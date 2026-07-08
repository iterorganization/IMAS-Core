// Capability-gated operation contract tests (issue #6 / TEST_STRATEGY.md §4 step 4).
//
// Some operations are only meaningful on backends that advertise the matching
// Cluster-E capability (FUNCTIONALITY_INVENTORY.md — supportsTimeDataInterpolation
// / supportsTimeRangeOperation). Per decision D4 each such op is tested two ways:
//   * POSITIVE where the backend supports it — the op runs and, wherever there is
//     an oracle, its returned data is asserted exactly (not merely code==0).
//   * PAIRED-NEGATIVE where it does not — the op must produce the *documented
//     refusal* (a specific al_status_t error code), never a crash or a silent
//     success. This puts the capability matrix itself under test at the C ABI
//     boundary (decision D1: we cannot link the C++ Backend flags directly, so we
//     assert their only ABI-observable consequence).
//
// The support columns below were established empirically against the current
// library (each value has a verified round trip), because two of them contradict
// the issue's stated assumptions and must follow D2 ("assert the real contract,
// tag defects as xfail — never freeze folklore"):
//
//   op / backend        HDF5   Memory   ASCII   Flexbuffers
//   slice   (R/W)         y       y*       n         n
//   timerange (R)         y       n        n         n
//   list_filled_paths     y       n        n         n
//
//   * Memory genuinely supports slice storage + closest/previous/linear slice
//     reads (src/memory_backend.cpp has full SLICE_OP handling); it is NOT a
//     paired-negative for slice, even though supportsTimeRangeOperation()==false.
//     What Memory lacks is time-RANGE operations, gated separately at
//     src/al_lowlevel.cpp:1053.
//
// Refusal codes are the documented, stable part of the contract (D4 "specific
// error code"):
//   * timerange on an unsupporting backend  -> LOWLEVEL_ERR
//     (src/al_lowlevel.cpp:1053-1055, ALLowlevelException)
//   * slice / list_filled_paths unsupported -> BACKEND_ERR
//     (backend beginAction / list_filled_paths throw ALBackendException)

#include "al_contract.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

using al_contract::PulseId;

namespace {

// --- opaque paths (the core attaches no DD semantics; strings are opaque) -----
constexpr const char* kIds       = "magnetics";
constexpr const char* kHomogTime = "ids_properties/homogeneous_time";
constexpr const char* kTimebase  = "time";  // homogeneous shared timebase
constexpr const char* kSignal    = "ip";    // an opaque dynamic leaf

// The seeded dynamic signal: ip = f(time), a 2-point time axis. Chosen so slice
// interpolation has a non-trivial, exactly-representable oracle
// (LINEAR at the midpoint = 15.0, not a round-off).
constexpr double kT0 = 1.0, kT1 = 2.0;
constexpr double kV0 = 10.0, kV1 = 20.0;

// ---------------------------------------------------------------------------
// Backend capability descriptor for the always-on tier (decision D4).
// ---------------------------------------------------------------------------
struct CapBackend {
    int         id;
    const char* name;
    bool        on_disk;    // needs the legacy <base>/<db>/<ver>/<pulse>/<run> tree
    bool        slice;      // al_begin_slice_action accepted
    bool        timerange;  // supportsTimeRangeOperation() (time-range ops accepted)
    bool        listpaths;  // al_list_filled_paths implemented
};

// GoogleTest prints this wherever a CapBackend is shown (failure diagnostics and
// the discovered test-name suffix), so both read "HDF5"/"Memory"/… not a byte dump.
inline void PrintTo(const CapBackend& b, std::ostream* os) { *os << b.name; }

const CapBackend kBackends[] = {
    {HDF5_BACKEND, "HDF5", /*on_disk=*/true, /*slice=*/true,
     /*timerange=*/true, /*listpaths=*/true},
    {MEMORY_BACKEND, "Memory", /*on_disk=*/false, /*slice=*/true,
     /*timerange=*/false, /*listpaths=*/false},
    {ASCII_BACKEND, "ASCII", /*on_disk=*/true, /*slice=*/false,
     /*timerange=*/false, /*listpaths=*/false},
    {FLEXBUFFERS_BACKEND, "Flexbuffers", /*on_disk=*/true,
     /*slice=*/false, /*timerange=*/false, /*listpaths=*/false},
};

// Write the 2-point dynamic signal through an ordinary GLOBAL write. Slice/
// timerange are READ-side capabilities here, so the fixture is seeded with a
// plain global write (already proven by the round-trip matrix on every backend),
// and multi-point time selection is exercised on read. Returns true iff every
// write succeeded, so a downstream refusal can never be blamed on a missing seed.
bool seed_signal(int pctx) {
    int op = -1;
    if (al_begin_global_action(pctx, kIds, "", WRITE_OP, &op).code != 0) {
        return false;
    }
    int    ht      = 1;
    double t[2]    = {kT0, kT1};
    double v[2]    = {kV0, kV1};
    int    shape[1] = {2};
    bool   ok = true;
    ok &= al_write_data(op, kHomogTime, "", &ht, INTEGER_DATA, 0, nullptr).code == 0;
    ok &= al_write_data(op, kTimebase, kTimebase, t, DOUBLE_DATA, 1, shape).code == 0;
    ok &= al_write_data(op, kSignal, kTimebase, v, DOUBLE_DATA, 1, shape).code == 0;
    ok &= al_end_action(op).code == 0;
    return ok;
}

// Read the single scalar a slice read collapses the signal to.
al_status_t read_slice_value(int op, double* out) {
    void* buf = nullptr;
    int   size[MAXDIM] = {0};
    al_status_t s = al_read_data(op, kSignal, kTimebase, &buf, DOUBLE_DATA, 1, size);
    if (s.code == 0 && buf != nullptr && size[0] >= 1) {
        *out = static_cast<double*>(buf)[0];
    }
    free(buf);
    return s;
}

// ===========================================================================
// Parametrized capability matrix: one instance per always-on backend.
// ===========================================================================
class CapabilityMatrix : public ::testing::TestWithParam<CapBackend> {
protected:
    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/12, /*run=*/0};

    // Open a fresh, seeded data entry. Fails the test fatally on any setup error
    // (so a later refusal assertion cannot be a false positive from bad setup).
    int open_seeded() {
        const CapBackend& b = GetParam();
        if (b.on_disk) base_.make_legacy_tree(pulse_);
        const std::string uri = al_contract::build_uri(b.id, base_.str(), pulse_);
        EXPECT_FALSE(uri.empty());
        int pctx = -1;
        EXPECT_EQ(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx).code, 0);
        EXPECT_TRUE(seed_signal(pctx)) << "seed write must succeed on every backend";
        return pctx;
    }
};

// --- timerange: positive on HDF5, paired-negative (LOWLEVEL_ERR) elsewhere ---
// Directly asserts supportsTimeRangeOperation() through its sole ABI
// manifestation (the guard at src/al_lowlevel.cpp:1053).
TEST_P(CapabilityMatrix, TimeRangeReadPositiveOrRefused) {
    const CapBackend b = GetParam();
    int pctx = open_seeded();

    int    op = -1;
    double dtime = 0.0;
    int    dtime_shape = 0;  // 0 => no resampling: return the raw stored slices
    al_status_t s = al_begin_timerange_action(pctx, kIds, READ_OP, kT0, kT1,
                                              &dtime, &dtime_shape,
                                              UNDEFINED_INTERP, &op);
    if (b.timerange) {
        AL_EXPECT_OK(s);
        void* buf = nullptr;
        int   size[MAXDIM] = {0};
        AL_EXPECT_OK(al_read_data(op, kSignal, kTimebase, &buf, DOUBLE_DATA, 1, size));
        ASSERT_EQ(size[0], 2) << "no-resample timerange returns both stored slices";
        EXPECT_EQ(static_cast<double*>(buf)[0], kV0);
        EXPECT_EQ(static_cast<double*>(buf)[1], kV1);
        free(buf);
        EXPECT_EQ(al_end_action(op).code, 0);
    } else {
        EXPECT_NE(s.code, 0) << "unsupporting backend must refuse timerange";
        EXPECT_EQ(s.code, LOWLEVEL_ERR)
            << "documented refusal is LOWLEVEL_ERR (al_lowlevel.cpp:1053)";
    }
    al_close_pulse(pctx, CLOSE_PULSE);
}

// --- slice: positive on HDF5 + Memory, paired-negative (BACKEND_ERR) elsewhere -
TEST_P(CapabilityMatrix, SliceReadPositiveOrRefused) {
    const CapBackend b = GetParam();
    int pctx = open_seeded();

    int op = -1;
    // A time between the two slices; CLOSEST resolves to kT0 -> kV0.
    al_status_t s = al_begin_slice_action(pctx, kIds, READ_OP, 1.4, CLOSEST_INTERP, &op);
    if (b.slice) {
        AL_EXPECT_OK(s);
        double val = -1.0;
        AL_EXPECT_OK(read_slice_value(op, &val));
        EXPECT_EQ(val, kV0) << "CLOSEST slice to t=1.4 is t=1.0 -> " << kV0;
        EXPECT_EQ(al_end_action(op).code, 0);
    } else {
        EXPECT_NE(s.code, 0) << "unsupporting backend must refuse slice";
        EXPECT_EQ(s.code, BACKEND_ERR)
            << "documented refusal is BACKEND_ERR (backend beginAction throws)";
    }
    al_close_pulse(pctx, CLOSE_PULSE);
}

// --- slice WRITE (append via UNDEFINED_TIME): accepted iff the backend slices --
// The issue calls out append/replace via UNDEFINED_TIME specifically; this pins
// that begin-slice-for-write is accepted where supported and refused where not.
TEST_P(CapabilityMatrix, SliceWriteBeginPositiveOrRefused) {
    const CapBackend b = GetParam();
    int pctx = open_seeded();

    int op = -1;
    al_status_t s = al_begin_slice_action(pctx, kIds, WRITE_OP, UNDEFINED_TIME,
                                          UNDEFINED_INTERP, &op);
    if (b.slice) {
        AL_EXPECT_OK(s);
        EXPECT_EQ(al_end_action(op).code, 0);
    } else {
        EXPECT_EQ(s.code, BACKEND_ERR) << "slice write must be refused (BACKEND_ERR)";
    }
    al_close_pulse(pctx, CLOSE_PULSE);
}

// --- list_filled_paths: positive on HDF5, paired-negative (BACKEND_ERR) else ---
TEST_P(CapabilityMatrix, ListFilledPathsPositiveOrRefused) {
    const CapBackend b = GetParam();
    int pctx = open_seeded();

    char**      paths = nullptr;
    int         n     = -1;
    al_status_t s     = al_list_filled_paths(pctx, kIds, &paths, &n);
    if (b.listpaths) {
        AL_EXPECT_OK(s);
        ASSERT_GE(n, 0);
        // The caller owns the list and every string (al_lowlevel.h:499).
        std::set<std::string> got;
        for (int i = 0; i < n; ++i) {
            got.insert(paths[i]);
            free(paths[i]);
        }
        free(paths);
        // Every leaf we seeded must be discoverable (order is unspecified).
        EXPECT_TRUE(got.count(kHomogTime)) << "seeded leaf missing from listing";
        EXPECT_TRUE(got.count(kTimebase)) << "seeded timebase missing from listing";
        EXPECT_TRUE(got.count(kSignal)) << "seeded signal missing from listing";
    } else {
        EXPECT_NE(s.code, 0) << "unsupporting backend must refuse list_filled_paths";
        EXPECT_EQ(s.code, BACKEND_ERR)
            << "documented refusal is BACKEND_ERR (not implemented in this backend)";
    }
    al_close_pulse(pctx, CLOSE_PULSE);
}

INSTANTIATE_TEST_SUITE_P(
    Backends, CapabilityMatrix, ::testing::ValuesIn(kBackends),
    [](const ::testing::TestParamInfo<CapBackend>& info) {
        return std::string(info.param.name);
    });

// ===========================================================================
// HDF5-only positive detail: slice interpolation modes + timerange resampling.
// ===========================================================================
// These carry the real interpolation oracle (CLOSEST / PREVIOUS / LINEAR and the
// with/without-dtime timerange forms the issue enumerates). They run only on the
// one always-on backend that supports interpolation; the paired-negative half is
// already covered by CapabilityMatrix above.
class Hdf5TimeDependent : public ::testing::Test {
protected:
    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/12, /*run=*/0};
    int     pctx_ = -1;

    void SetUp() override {
        base_.make_legacy_tree(pulse_);
        const std::string uri = al_contract::build_uri(HDF5_BACKEND, base_.str(), pulse_);
        ASSERT_FALSE(uri.empty());
        ASSERT_EQ(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx_).code, 0);
        ASSERT_TRUE(seed_signal(pctx_));
    }
    void TearDown() override {
        if (pctx_ >= 0) al_close_pulse(pctx_, CLOSE_PULSE);
    }

    double slice_read(double time, int interp) {
        int op = -1;
        EXPECT_EQ(al_begin_slice_action(pctx_, kIds, READ_OP, time, interp, &op).code, 0);
        double val = -1.0;
        EXPECT_EQ(read_slice_value(op, &val).code, 0);
        EXPECT_EQ(al_end_action(op).code, 0);
        return val;
    }
};

TEST_F(Hdf5TimeDependent, SliceInterpolationModes) {
    // CLOSEST: nearest sample time.
    EXPECT_EQ(slice_read(1.4, CLOSEST_INTERP), kV0);  // 1.4 nearer 1.0
    EXPECT_EQ(slice_read(1.6, CLOSEST_INTERP), kV1);  // 1.6 nearer 2.0
    // PREVIOUS: last sample at/below the requested time.
    EXPECT_EQ(slice_read(1.9, PREVIOUS_INTERP), kV0);
    // LINEAR: interpolate between bracketing samples (midpoint -> mean).
    EXPECT_EQ(slice_read(1.5, LINEAR_INTERP), (kV0 + kV1) / 2.0);
}

TEST_F(Hdf5TimeDependent, TimeRangeReadWithoutResampling) {
    int         op = -1;
    double      dtime = 0.0;
    int         dtime_shape = 0;  // no resampling
    ASSERT_EQ(al_begin_timerange_action(pctx_, kIds, READ_OP, kT0, kT1, &dtime,
                                        &dtime_shape, UNDEFINED_INTERP, &op).code, 0);
    void* buf = nullptr;
    int   size[MAXDIM] = {0};
    ASSERT_EQ(al_read_data(op, kSignal, kTimebase, &buf, DOUBLE_DATA, 1, size).code, 0);
    ASSERT_EQ(size[0], 2);
    EXPECT_EQ(static_cast<double*>(buf)[0], kV0);
    EXPECT_EQ(static_cast<double*>(buf)[1], kV1);
    free(buf);
    EXPECT_EQ(al_end_action(op).code, 0);
}

TEST_F(Hdf5TimeDependent, TimeRangeReadWithResampling) {
    // Resample onto an explicit 3-point grid; LINEAR fills the new midpoint.
    int    op = -1;
    double grid[3]  = {1.0, 1.5, 2.0};
    int    grid_len = 3;
    ASSERT_EQ(al_begin_timerange_action(pctx_, kIds, READ_OP, kT0, kT1, grid,
                                        &grid_len, LINEAR_INTERP, &op).code, 0);
    void* buf = nullptr;
    int   size[MAXDIM] = {0};
    ASSERT_EQ(al_read_data(op, kSignal, kTimebase, &buf, DOUBLE_DATA, 1, size).code, 0);
    ASSERT_EQ(size[0], 3) << "resampled onto the 3-point target grid";
    EXPECT_EQ(static_cast<double*>(buf)[0], kV0);
    EXPECT_EQ(static_cast<double*>(buf)[1], (kV0 + kV1) / 2.0);
    EXPECT_EQ(static_cast<double*>(buf)[2], kV1);
    free(buf);
    EXPECT_EQ(al_end_action(op).code, 0);
}

// A slice READ with UNDEFINED_INTERP is a documented refusal (an interp mode is
// required for reads). Pins the CONTEXT_ERR contract (src/al_context.cpp:347).
TEST_F(Hdf5TimeDependent, SliceReadWithoutInterpModeIsRejected) {
    int op = -1;
    al_status_t s = al_begin_slice_action(pctx_, kIds, READ_OP, 1.5, UNDEFINED_INTERP, &op);
    EXPECT_NE(s.code, 0) << "a slice read without an interp mode must be refused";
    EXPECT_EQ(s.code, CONTEXT_ERR) << "documented refusal: 'Missing interpmode'";
}

// ===========================================================================
// Slice append via UNDEFINED_TIME — KNOWN DEFECT (decision D2).
// ===========================================================================
// The issue calls for slice "append/replace via UNDEFINED_TIME". A correct
// slicing backend must let repeated WRITE+UNDEFINED_TIME slices *accumulate*
// (the header documents UNDEFINED_TIME as "append a slice"). Empirically, in
// this build each append overwrites the previous — only the last-written slice
// survives — regardless of UNDEFINED_TIME vs a concrete time, or where
// homogeneous_time is written. Per D2 the correct contract is asserted (as an
// expected-fail), never frozen as folklore; a current-behavior tripwire pins
// today's overwrite so the xfail can't rot. Write-lifecycle ownership is
// issue #3; the slice *contract* is pinned here.
class Hdf5SliceAppend : public ::testing::Test {
protected:
    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/12, /*run=*/0};
    int     pctx_ = -1;

    void SetUp() override {
        base_.make_legacy_tree(pulse_);
        const std::string uri = al_contract::build_uri(HDF5_BACKEND, base_.str(), pulse_);
        ASSERT_FALSE(uri.empty());
        ASSERT_EQ(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx_).code, 0);
        // Append two slices: (t=1.0 -> 10.0) then (t=2.0 -> 20.0).
        const double t[2] = {kT0, kT1};
        const double v[2] = {kV0, kV1};
        for (int k = 0; k < 2; ++k) {
            int op = -1;
            EXPECT_EQ(al_begin_slice_action(pctx_, kIds, WRITE_OP, UNDEFINED_TIME,
                                            UNDEFINED_INTERP, &op).code, 0);
            if (k == 0) {
                int ht = 1;
                al_write_data(op, kHomogTime, "", &ht, INTEGER_DATA, 0, nullptr);
            }
            int one[1] = {1};
            al_write_data(op, kTimebase, kTimebase, const_cast<double*>(&t[k]),
                          DOUBLE_DATA, 1, one);
            al_write_data(op, kSignal, kTimebase, const_cast<double*>(&v[k]),
                          DOUBLE_DATA, 1, one);
            EXPECT_EQ(al_end_action(op).code, 0);
        }
    }
    void TearDown() override {
        if (pctx_ >= 0) al_close_pulse(pctx_, CLOSE_PULSE);
    }

    // Number of stored slices a no-resample timerange read finds.
    int stored_slice_count() {
        int op = -1;
        double dtime = 0.0;
        int    dtime_shape = 0;
        EXPECT_EQ(al_begin_timerange_action(pctx_, kIds, READ_OP, 0.0, 10.0, &dtime,
                                            &dtime_shape, UNDEFINED_INTERP, &op).code, 0);
        void* buf = nullptr;
        int   size[MAXDIM] = {0};
        EXPECT_EQ(al_read_data(op, kSignal, kTimebase, &buf, DOUBLE_DATA, 1, size).code, 0);
        free(buf);
        EXPECT_EQ(al_end_action(op).code, 0);
        return size[0];
    }
};

// CORRECT-CONTRACT, expected-fail (DISABLED_): both appended slices must persist.
// Drop the DISABLED_ prefix the day append works — the suite then documents it.
TEST_F(Hdf5SliceAppend, DISABLED_AppendedSlicesAllPersist) {
    EXPECT_EQ(stored_slice_count(), 2)
        << "two WRITE+UNDEFINED_TIME slices must both be readable";
}

// CURRENT-BEHAVIOR tripwire: today only the last-written slice survives. When
// append is fixed this goes red, forcing whoever fixed it to enable the
// correct-contract test above.
TEST_F(Hdf5SliceAppend, OnlyLastAppendedSlicePersists_CurrentBehavior) {
    EXPECT_EQ(stored_slice_count(), 1)
        << "append currently overwrites; if this is now 2 the defect is fixed — "
           "enable Hdf5SliceAppend.AppendedSlicesAllPersist";
}

}  // namespace
