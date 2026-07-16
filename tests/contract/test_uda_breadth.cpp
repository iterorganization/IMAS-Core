// UDA parity breadth beyond the real-DD-path matrix (issue #25,
// TRACEABILITY.md Part 5): AOS traversal, occurrences, list_filled_paths,
// pulse open modes / error behavior, and slice/time-range reads -- every
// remaining C-ABI-reachable read-side capability from
// FUNCTIONALITY_INVENTORY.md Part 1 that test_uda.cpp / test_uda_real_paths.cpp
// do not already cover.
//
// Same seed(HDF5)-then-reopen(UDA) shape throughout (issue #24): the plain
// HDF5 backend seeds a pulse dir, the UDA backend in remote mode
// (backend=hdf5&cache_mode=none) reopens the identical path and the test
// asserts what comes back through the public C ABI. Real DD-4.1.1 paths only
// (UDA validates every path against the schema it loads at startup,
// src/uda/uda_xml.cpp) -- see test_uda.cpp's characterization-discovered
// facts for the ids_properties/homogeneous_time precondition every remote
// read needs first.
#ifdef AL_CONTRACT_HAVE_UDA

#include "al_contract.h"
#include "equilibrium_seed.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace {

// Build the UDA remote-mode URI for a seeded HDF5 pulse dir.
std::string uda_uri_for(const std::string& pulse_dir) {
    return al_contract::uda_uri_base() + "?backend=hdf5&cache_mode=none&path=" +
           pulse_dir;
}

// Exercise ERASE_PULSE entirely through the public C ABI, then report whether
// the same remote pulse remains openable. Shared by the disabled correct-
// contract test and its current-behavior tripwire below.
struct UdaEraseResult {
    int create_code;
    int erase_code;
    int reopen_code;
};

UdaEraseResult uda_remote_reopen_after_erase() {
    al_contract::TempBase base;
    const std::string pulse_dir = base.str() + "/pulse";
    std::error_code    ec;
    std::filesystem::create_directories(pulse_dir, ec);
    const std::string uda_uri = uda_uri_for(pulse_dir);

    int         pulse_ctx = -1;
    al_status_t created = al_begin_dataentry_action(
        uda_uri.c_str(), FORCE_CREATE_PULSE, &pulse_ctx);
    if (created.code != 0) return {created.code, -1, -1};

    al_status_t erased = al_close_pulse(pulse_ctx, ERASE_PULSE);
    if (erased.code != 0) return {created.code, erased.code, -1};

    int         reopened_ctx = -1;
    al_status_t reopened =
        al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE, &reopened_ctx);
    if (reopened.code == 0) al_close_pulse(reopened_ctx, CLOSE_PULSE);
    return {created.code, erased.code, reopened.code};
}

class UdaBreadthTest : public ::testing::Test {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_UDA_UNCONFIGURED(); }

    al_contract::TempBase base_;

    std::string fresh_pulse_dir() const {
        const std::string dir = base_.str() + "/pulse";
        std::error_code    ec;
        std::filesystem::create_directories(dir, ec);
        return dir;
    }
};

// ===========================================================================
// AOS traversal (top-level): equilibrium/time_slice is a real DD struct_array
// (timebasepath="time"), each element carrying global_quantities/ip -- the
// same real DD AOS path MDSplus's own unique-surface/parity proof uses
// (TRACEABILITY.md Part 4). Seeded with 3 elements through HDF5, reopened
// through UDA: al_begin_arraystruct_action correctly reports size 3 and
// iterates (confirmed via the debug trace: three distinct
// "time_slice[0..2]/global_quantities/ip" directives are sent), but the
// VALUE that comes back for every element is the HDF5 "absent" sentinel
// (-9.0e40, al_contract::kEmptyDouble), not what was written -- a genuine
// NEW DEFECT, not a legitimate storage difference, pinned per D2 below
// (UdaAosKnownDefects). Root cause traced empirically: `global_quantities/ip`
// is a DD *dynamic* (time-varying) leaf (confirmed via the request trace's
// `dynamic_flags=1`), and every dynamic leaf nested inside a struct_array
// fails identically through UDA remote mode + backend=hdf5 -- see
// UdaRealPathMatrix's COMPLEX_r1/r3/r5 rows (test_uda_real_paths.cpp), which
// hit the exact same wall on a different datatype/AOS-depth (their
// `known_defect_reason`, not `divergence_reason`, cites this test). Static
// (non-dynamic) leaves nested in a struct_array -- e.g. `temporary`'s
// self-test IDS fields -- are unaffected (UdaRealPathMatrix's INTEGER_r3/
// DOUBLE_r6 pass cleanly).
// ===========================================================================

// Seed 3 elements of equilibrium/time_slice via HDF5 (global_quantities/ip =
// 100+i per element), reopen through UDA remote mode, and return what comes
// back for each element (rather than asserting inline) so the correct-
// contract test and its tripwire can share one seed/read path (mirrors
// test_structured_data.cpp's ascii_aos_read_reported_size() pattern).
std::vector<double> dynamic_leaf_inside_aos_values_via_uda() {
    al_contract::TempBase base;
    const std::string pulse_dir = base.str() + "/pulse";
    std::error_code    ec;
    std::filesystem::create_directories(pulse_dir, ec);
    constexpr int kN = 3;

    // --- seed via HDF5 -------------------------------------------------------
    {
        const std::string hdf5_uri = "imas:hdf5?path=" + pulse_dir;
        int                pulse_ctx = -1;
        EXPECT_EQ(al_begin_dataentry_action(hdf5_uri.c_str(), FORCE_CREATE_PULSE,
                                            &pulse_ctx)
                      .code,
                  0);

        int op = -1;
        EXPECT_EQ(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                         WRITE_OP, &op)
                      .code,
                  0);
        EXPECT_EQ(al_contract::write_data<int>(
                      op, "ids_properties/homogeneous_time", {}, {1})
                      .code,
                  0);

        int size = kN;
        int aos  = -1;
        EXPECT_EQ(al_begin_arraystruct_action(op, "time_slice", "", &size, &aos)
                      .code,
                  0);
        for (int i = 0; i < kN; ++i) {
            EXPECT_EQ(al_contract::write_data<double>(aos, "global_quantities/ip",
                                                       {}, {100.0 + i})
                          .code,
                      0);
            if (i + 1 < kN) EXPECT_EQ(al_iterate_over_arraystruct(aos, 1).code, 0);
        }
        EXPECT_EQ(al_end_action(aos).code, 0);
        EXPECT_EQ(al_end_action(op).code, 0);
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    // --- reopen through UDA, remote mode ------------------------------------
    std::vector<double> values;
    {
        const std::string uda_uri = uda_uri_for(pulse_dir);
        int                pulse_ctx = -1;
        EXPECT_EQ(
            al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE, &pulse_ctx).code,
            0);

        int op = -1;
        EXPECT_EQ(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                         READ_OP, &op)
                      .code,
                  0);

        int rsize = 0;
        int raos  = -1;
        EXPECT_EQ(
            al_begin_arraystruct_action(op, "time_slice", "", &rsize, &raos).code,
            0);
        EXPECT_EQ(rsize, kN) << "AOS size must round-trip through UDA";
        for (int i = 0; i < rsize; ++i) {
            std::vector<int>    shape;
            std::vector<double> data;
            EXPECT_EQ(al_contract::read_data<double>(raos, "global_quantities/ip",
                                                      0, &shape, &data)
                          .code,
                      0);
            values.push_back(data.empty() ? al_contract::kEmptyDouble : data[0]);
            if (i + 1 < rsize) EXPECT_EQ(al_iterate_over_arraystruct(raos, 1).code, 0);
        }
        EXPECT_EQ(al_end_action(raos).code, 0);
        EXPECT_EQ(al_end_action(op).code, 0);
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }
    return values;
}

// CORRECT-CONTRACT, expected-fail (DISABLED_): every element's written value
// must round-trip.
TEST(UdaAosKnownDefects, DISABLED_DynamicLeafInsideAosRoundTrips) {
    AL_CONTRACT_SKIP_IF_UDA_UNCONFIGURED();
    const std::vector<double> values = dynamic_leaf_inside_aos_values_via_uda();
    ASSERT_EQ(values.size(), 3u);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(values[static_cast<size_t>(i)], 100.0 + i)
            << "element " << i << " must round-trip through UDA remote mode";
    }
}

// CURRENT-BEHAVIOR tripwire: every element currently comes back as the HDF5
// "absent" sentinel instead of what was written. When this is fixed, this
// test goes red, forcing whoever fixed it to enable the correct-contract
// test above.
TEST(UdaAosKnownDefects, DynamicLeafInsideAosCurrentlyReturnsSentinel) {
    AL_CONTRACT_SKIP_IF_UDA_UNCONFIGURED();
    const std::vector<double> values = dynamic_leaf_inside_aos_values_via_uda();
    ASSERT_EQ(values.size(), 3u);
    for (double v : values) {
        EXPECT_EQ(v, al_contract::kEmptyDouble)
            << "a dynamic leaf nested in an AOS now returns real data through "
               "UDA remote mode -- enable "
               "UdaAosKnownDefects.DISABLED_DynamicLeafInsideAosRoundTrips "
               "(and update UdaRealPathMatrix's COMPLEX_r1/r3/r5 "
               "known_defect_reason rows in test_uda_real_paths.cpp)";
    }
}

// ===========================================================================
// Occurrences (al_get_occurrences): the reference IMAS server plugin
// implements getOccurrences (PRD #21's static finding, confirmed empirically
// here). Seeded exactly like Occurrences.Hdf5ListsWrittenOccurrences
// (test_structured_data.cpp) -- occurrence 0 as "equilibrium", occurrence 2
// as HDF5's own "<ids>_<N>" naming convention "equilibrium_2" -- then listed
// through UDA remote mode.
// ===========================================================================
TEST_F(UdaBreadthTest, OccurrencesListsWrittenOccurrencesThroughReopen) {
    const std::string pulse_dir = fresh_pulse_dir();

    // --- seed via HDF5: occurrence 0 and occurrence 2 -----------------------
    {
        const std::string hdf5_uri = "imas:hdf5?path=" + pulse_dir;
        int                pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(hdf5_uri.c_str(),
                                               FORCE_CREATE_PULSE, &pulse_ctx));

        for (const char* name : {"equilibrium", "equilibrium_2"}) {
            int op = -1;
            AL_ASSERT_OK(
                al_begin_global_action(pulse_ctx, name, "", WRITE_OP, &op));
            AL_EXPECT_OK(al_contract::write_data<int>(
                op, "ids_properties/homogeneous_time", {}, {1}));
            AL_EXPECT_OK(al_contract::write_data<double>(
                op, equilibrium_seed::kScalar, {}, {6.2}));
            AL_ASSERT_OK(al_end_action(op));
        }
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    // --- list occurrences through UDA, remote mode --------------------------
    {
        const std::string uda_uri = uda_uri_for(pulse_dir);
        int                pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE,
                                               &pulse_ctx));

        int* occ = nullptr;
        int  n   = -1;
        AL_ASSERT_OK(
            al_get_occurrences(pulse_ctx, equilibrium_seed::kIds, &occ, &n));
        ASSERT_EQ(n, 2);
        EXPECT_EQ(occ[0], 0);
        EXPECT_EQ(occ[1], 2);
        free(occ);  // malloc'd by UDABackend::get_occurrences.

        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }
}

// ===========================================================================
// al_list_filled_paths: the reference IMAS server plugin implements
// listFilledPaths (PRD #21's static finding); UDABackend::list_filled_paths
// restricts itself to backend=hdf5 (src/uda/uda_backend.cpp), which this
// fixture already uses throughout.
// ===========================================================================
TEST_F(UdaBreadthTest, ListFilledPathsThroughReopen) {
    const std::string pulse_dir = fresh_pulse_dir();

    // --- seed via HDF5 -------------------------------------------------------
    {
        const std::string hdf5_uri = "imas:hdf5?path=" + pulse_dir;
        int                pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(hdf5_uri.c_str(),
                                               FORCE_CREATE_PULSE, &pulse_ctx));
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds,
                                            "", WRITE_OP, &op));
        AL_EXPECT_OK(al_contract::write_data<int>(
            op, "ids_properties/homogeneous_time", {}, {1}));
        AL_EXPECT_OK(
            al_contract::write_data<double>(op, equilibrium_seed::kScalar, {}, {6.2}));
        AL_ASSERT_OK(al_end_action(op));
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    // --- list_filled_paths through UDA, remote mode -------------------------
    {
        const std::string uda_uri = uda_uri_for(pulse_dir);
        int                pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE,
                                               &pulse_ctx));

        char**      paths = nullptr;
        int         n     = -1;
        al_status_t s = al_list_filled_paths(pulse_ctx, equilibrium_seed::kIds,
                                             &paths, &n);
        AL_EXPECT_OK(s);
        ASSERT_GE(n, 0);
        std::set<std::string> got;
        for (int i = 0; i < n; ++i) {
            got.insert(paths[i]);
            free(paths[i]);
        }
        free(paths);
        EXPECT_TRUE(got.count("ids_properties/homogeneous_time"))
            << "seeded leaf missing from listing";
        EXPECT_TRUE(got.count(equilibrium_seed::kScalar))
            << "seeded leaf missing from listing";

        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }
}

// ===========================================================================
// Pulse open modes and error behavior on absent paths / absent IDSs.
// ===========================================================================

// OPEN_PULSE against a UDA URI whose remote pulse dir does not exist at all.
// UDA's `openPulse` genuinely round-trips to the server at open time (unlike
// the initial assumption this test started from) -- the debug trace shows
// `al_begin_dataentry_action` itself issuing `IMAS::open(...)`, and the
// server-side HDF5 backend it opens on the plugin's behalf refuses
// immediately ("HDF5 master file not found"), which the UDA client re-throws
// through the C ABI as a non-zero al_status_t. This is genuine **parity**
// with the on-disk backends' own OPEN_PULSE-fails-when-absent contract
// (DataEntryModes.OpenPulseFailsWhenAbsent, test_pulse_lifecycle.cpp) -- UDA
// remote mode does not weaken it.
TEST_F(UdaBreadthTest, OpenPulseFailsWhenRemotePathAbsent) {
    const std::string absent_dir = base_.str() + "/never-seeded";
    const std::string uda_uri    = uda_uri_for(absent_dir);
    int                pulse_ctx  = -1;
    al_status_t        s = al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE,
                                               &pulse_ctx);
    EXPECT_NE(s.code, 0) << "OPEN_PULSE must fail when the remote pulse does "
                            "not exist, matching the on-disk backends' "
                            "parity contract";
    if (s.code == 0) al_close_pulse(pulse_ctx, CLOSE_PULSE);
}

TEST_F(UdaBreadthTest, ForceOpenPulseCreatesRemotePulseWhenAbsent) {
    const std::string pulse_dir = fresh_pulse_dir();
    const std::string uda_uri   = uda_uri_for(pulse_dir);

    int pulse_ctx = -1;
    AL_ASSERT_OK(
        al_begin_dataentry_action(uda_uri.c_str(), FORCE_OPEN_PULSE, &pulse_ctx));
    AL_ASSERT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));

    int reopened_ctx = -1;
    al_status_t reopened =
        al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE, &reopened_ctx);
    AL_EXPECT_OK(reopened);
    if (reopened.code == 0) {
        EXPECT_EQ(al_close_pulse(reopened_ctx, CLOSE_PULSE).code, 0);
    }
}

TEST_F(UdaBreadthTest, CreatePulseRefusesExistingRemotePulse) {
    const std::string pulse_dir = fresh_pulse_dir();
    const std::string uda_uri   = uda_uri_for(pulse_dir);

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uda_uri.c_str(), FORCE_CREATE_PULSE,
                                           &pulse_ctx));
    AL_ASSERT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));

    int         create_ctx = -1;
    al_status_t s = al_begin_dataentry_action(uda_uri.c_str(), CREATE_PULSE,
                                               &create_ctx);
    EXPECT_NE(s.code, 0)
        << "CREATE_PULSE must refuse an existing remote pulse";
    if (s.code == 0) al_close_pulse(create_ctx, CLOSE_PULSE);
}

TEST_F(UdaBreadthTest, ForceCreatePulseAcceptsExistingRemotePulse) {
    const std::string pulse_dir = fresh_pulse_dir();
    const std::string uda_uri   = uda_uri_for(pulse_dir);

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uda_uri.c_str(), FORCE_CREATE_PULSE,
                                           &pulse_ctx));
    AL_ASSERT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));

    int replaced_ctx = -1;
    al_status_t replaced = al_begin_dataentry_action(
        uda_uri.c_str(), FORCE_CREATE_PULSE, &replaced_ctx);
    AL_EXPECT_OK(replaced);
    if (replaced.code == 0) {
        EXPECT_EQ(al_close_pulse(replaced_ctx, CLOSE_PULSE).code, 0);
    }
}

// CORRECT-CONTRACT, expected-fail (DISABLED_): ERASE_PULSE promises to close
// and remove the pulse, so a later OPEN_PULSE must fail.
TEST_F(UdaBreadthTest, DISABLED_ErasePulseMakesRemotePulseUnopenable) {
    const UdaEraseResult result = uda_remote_reopen_after_erase();
    ASSERT_EQ(result.create_code, 0) << "test setup must create the remote pulse";
    ASSERT_EQ(result.erase_code, 0) << "ERASE_PULSE itself must report success";
    EXPECT_NE(result.reopen_code, 0)
        << "ERASE_PULSE must remove the remote pulse";
}

// CURRENT-BEHAVIOR tripwire: the reference stack's server-side HDF5 backend
// accepts ERASE_PULSE but treats it like CLOSE_PULSE, leaving the pulse
// openable. When this changes, this test goes red and the correct-contract
// test above must be enabled.
TEST_F(UdaBreadthTest, ErasePulseCurrentlyLeavesRemotePulseOpenable) {
    const UdaEraseResult result = uda_remote_reopen_after_erase();
    ASSERT_EQ(result.create_code, 0) << "test setup must create the remote pulse";
    ASSERT_EQ(result.erase_code, 0) << "ERASE_PULSE itself must report success";
    EXPECT_EQ(result.reopen_code, 0)
        << "ERASE_PULSE now removes the remote pulse -- enable "
           "UdaBreadthTest.DISABLED_ErasePulseMakesRemotePulseUnopenable";
}

// A read against a real DD leaf that was never written (the IDS/occurrence
// itself was seeded, but this particular field was not): confirmed
// empirically that this succeeds (code==0) and returns the same HDF5
// "absent leaf" sentinel value (al_contract::kEmptyDouble) the local HDF5
// backend itself returns for an unwritten scalar read (RoundTripMatrix's
// generator carefully avoids ever *writing* this sentinel, precisely because
// reading an absent scalar returns it rather than erroring -- al_lowlevel.cpp
// Lowlevel::setDefaultValue). This is transparent parity, not a UDA-specific
// behavior: the remote get() forwards straight through to the server-side
// HDF5 backend's own semantics, unchanged.
TEST_F(UdaBreadthTest, ReadOfUnwrittenLeafOnSeededIdsReturnsHdf5AbsentSentinel) {
    const std::string pulse_dir = fresh_pulse_dir();
    {
        const std::string hdf5_uri = "imas:hdf5?path=" + pulse_dir;
        int                pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(hdf5_uri.c_str(),
                                               FORCE_CREATE_PULSE, &pulse_ctx));
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds,
                                            "", WRITE_OP, &op));
        AL_EXPECT_OK(al_contract::write_data<int>(
            op, "ids_properties/homogeneous_time", {}, {1}));
        // Deliberately do NOT write equilibrium_seed::kScalar.
        AL_ASSERT_OK(al_end_action(op));
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    const std::string uda_uri = uda_uri_for(pulse_dir);
    int                pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE, &pulse_ctx));
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                        READ_OP, &op));
    std::vector<int>    shape;
    std::vector<double> data;
    al_status_t s = al_contract::read_data<double>(op, equilibrium_seed::kScalar,
                                                    0, &shape, &data);
    AL_EXPECT_OK(s) << "reading an unwritten leaf on a seeded IDS reports "
                       "success (matching the local HDF5 backend's own "
                       "absent-scalar contract), not an error";
    ASSERT_EQ(data.size(), 1u);
    EXPECT_EQ(data[0], al_contract::kEmptyDouble)
        << "unwritten leaf must read back as the HDF5 absent-scalar "
           "sentinel, forwarded transparently through UDA";
    al_end_action(op);
    al_close_pulse(pulse_ctx, CLOSE_PULSE);
}

// A read against a real-DD-conformant but never-written IDS (the pulse dir
// exists and one IDS was seeded, but a completely different, never-touched
// IDS is requested) must also fail cleanly.
TEST_F(UdaBreadthTest, ReadFailsForNeverWrittenIds) {
    const std::string pulse_dir = fresh_pulse_dir();
    {
        const std::string hdf5_uri = "imas:hdf5?path=" + pulse_dir;
        int                pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(hdf5_uri.c_str(),
                                               FORCE_CREATE_PULSE, &pulse_ctx));
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds,
                                            "", WRITE_OP, &op));
        AL_EXPECT_OK(al_contract::write_data<int>(
            op, "ids_properties/homogeneous_time", {}, {1}));
        AL_ASSERT_OK(al_end_action(op));
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    const std::string uda_uri = uda_uri_for(pulse_dir);
    int                pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE, &pulse_ctx));
    // "magnetics" is a real DD 4.1.1 IDS never written to this pulse dir.
    int op = -1;
    AL_ASSERT_OK(
        al_begin_global_action(pulse_ctx, "magnetics", "", READ_OP, &op));
    std::vector<int> shape;
    std::vector<int> data;
    al_status_t s = al_contract::read_data<int>(
        op, "code/output_flag", 1, &shape, &data);
    EXPECT_NE(s.code, 0) << "reading a never-written IDS must fail, not "
                            "return stale/default data";
    al_end_action(op);
    al_close_pulse(pulse_ctx, CLOSE_PULSE);
}

// ===========================================================================
// Slice and time-range reads (where existing fixtures cover them for other
// backends -- CapabilityMatrix, test_capability_gated.cpp). Real DD dynamic
// leaf: equilibrium's vacuum_toroidal_field/b0 (FLT_1D, coordinate1 "time"),
// paired with the IDS's own top-level "time" timebase -- both real DD 4.1.1
// paths, the same shape CapabilityMatrix's seed_signal() uses with opaque
// names.
// ===========================================================================
constexpr double kUdaTimeT0 = 1.0, kUdaTimeT1 = 2.0;
constexpr double kUdaTimeV0 = 10.0, kUdaTimeV1 = 20.0;

class UdaSliceAndTimeRange : public UdaBreadthTest {
public:
    // Seed the 2-point dynamic signal through HDF5, return the pulse dir.
    // Public: the timerange-known-defect helper below (a free function, so
    // the DISABLED_/tripwire pair can share one seed/read path) calls it on a
    // fixture instance passed in by the TEST_F body.
    std::string seed_dynamic_signal() {
        const std::string pulse_dir = fresh_pulse_dir();
        const std::string hdf5_uri  = "imas:hdf5?path=" + pulse_dir;
        int                pulse_ctx = -1;
        EXPECT_EQ(al_begin_dataentry_action(hdf5_uri.c_str(), FORCE_CREATE_PULSE,
                                            &pulse_ctx)
                      .code,
                  0);
        int op = -1;
        EXPECT_EQ(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                         WRITE_OP, &op)
                      .code,
                  0);
        int    ht      = 1;
        double t[2]    = {kUdaTimeT0, kUdaTimeT1};
        double v[2]    = {kUdaTimeV0, kUdaTimeV1};
        int    shape[1] = {2};
        EXPECT_EQ(al_write_data(op, "ids_properties/homogeneous_time", "", &ht,
                                INTEGER_DATA, 0, nullptr)
                      .code,
                  0);
        EXPECT_EQ(al_write_data(op, "time", "time", t, DOUBLE_DATA, 1, shape).code, 0);
        EXPECT_EQ(al_write_data(op, "vacuum_toroidal_field/b0", "time", v,
                                DOUBLE_DATA, 1, shape)
                      .code,
                  0);
        EXPECT_EQ(al_end_action(op).code, 0);
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
        return pulse_dir;
    }
};

TEST_F(UdaSliceAndTimeRange, SliceReadThroughReopen) {
    const std::string pulse_dir = seed_dynamic_signal();
    const std::string uda_uri   = uda_uri_for(pulse_dir);
    int                pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE, &pulse_ctx));

    int op = -1;
    // A time between the two samples; CLOSEST resolves to kT0 -> kV0.
    al_status_t s = al_begin_slice_action(pulse_ctx, equilibrium_seed::kIds,
                                          READ_OP, 1.4, CLOSEST_INTERP, &op);
    AL_ASSERT_OK(s);
    void* buf         = nullptr;
    int   size[MAXDIM] = {0};
    AL_EXPECT_OK(al_read_data(op, "vacuum_toroidal_field/b0", "time", &buf,
                              DOUBLE_DATA, 1, size));
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(static_cast<double*>(buf)[0], kUdaTimeV0)
        << "CLOSEST slice to t=1.4 is t=1.0 -> " << kUdaTimeV0;
    free(buf);
    EXPECT_EQ(al_end_action(op).code, 0);
    EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
}

// ===========================================================================
// Time-range read -- NEW DEFECT (not UDA-specific in origin, but only
// observable through UDA's C-ABI surface): al_begin_timerange_action's
// OperationContext constructor (src/al_context.cpp, the
// `(ctx, dataobject, access, range, tmin, tmax, dtime, interp)` overload)
// never assigns the base `interpmode` member -- only
// `time_range.interpolation_method` -- unlike the GLOBAL_OP and SLICE_OP
// constructors, which both set it. Every always-on backend's readData()
// ignores `getInterpmode()` for a TIMERANGE_OP context (they consult
// `time_range` instead), so the uninitialized member is silently never
// observed -- until UDA's remote readData() directive-builder
// (src/uda/uda_backend.cpp) calls `op_ctx->getInterpmode()`
// *unconditionally*, regardless of rangemode, to fill the directive's
// `interp=` field. Confirmed empirically: the garbage value trips
// `uda_utilities.hpp`'s `InterpMode` convertor's default case, throwing
// `std::runtime_error("unknown interp mode: <garbage int>")`, surfaced
// through the C ABI as `al_plugin_read_data: unknown interp mode: ...`. Pinned
// per D2: correct-contract (DISABLED_) + current-behavior tripwire, sharing
// one read attempt (mirrors UdaAosKnownDefects above).
// ===========================================================================
al_status_t timerange_read_without_resampling_via_uda(
    UdaSliceAndTimeRange* fixture, void** buf, int* size) {
    const std::string pulse_dir = fixture->seed_dynamic_signal();
    const std::string uda_uri   = uda_uri_for(pulse_dir);
    int                pulse_ctx = -1;
    EXPECT_EQ(
        al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE, &pulse_ctx).code, 0);

    int    op          = -1;
    double dtime       = 0.0;
    int    dtime_shape = 0;  // no resampling: return the raw stored slices
    EXPECT_EQ(al_begin_timerange_action(pulse_ctx, equilibrium_seed::kIds,
                                        READ_OP, kUdaTimeT0,
                                        kUdaTimeT1, &dtime,
                                        &dtime_shape, UNDEFINED_INTERP, &op)
                  .code,
              0);
    al_status_t s = al_read_data(op, "vacuum_toroidal_field/b0", "time", buf,
                                 DOUBLE_DATA, 1, size);
    al_end_action(op);
    al_close_pulse(pulse_ctx, CLOSE_PULSE);
    return s;
}

// CORRECT-CONTRACT, expected-fail (DISABLED_): a no-resample timerange read
// through UDA must return both stored slices, matching every other backend's
// CapabilityMatrix.TimeRangeReadPositiveOrRefused/HDF5 contract.
TEST_F(UdaSliceAndTimeRange, DISABLED_TimeRangeReadWithoutResamplingThroughReopenSucceeds) {
    void* buf          = nullptr;
    int   size[MAXDIM] = {0};
    al_status_t s = timerange_read_without_resampling_via_uda(this, &buf, size);
    AL_ASSERT_OK(s);
    ASSERT_EQ(size[0], 2) << "no-resample timerange returns both stored slices";
    EXPECT_EQ(static_cast<double*>(buf)[0], kUdaTimeV0);
    EXPECT_EQ(static_cast<double*>(buf)[1], kUdaTimeV1);
    free(buf);
}

// CURRENT-BEHAVIOR tripwire: today's read fails with the uninitialized-
// interpmode-derived "unknown interp mode" exception every time. When fixed,
// this test goes red, forcing whoever fixed it to enable the correct-
// contract test above.
TEST_F(UdaSliceAndTimeRange, TimeRangeReadWithoutResamplingCurrentlyFailsWithUnknownInterpMode) {
    void* buf          = nullptr;
    int   size[MAXDIM] = {0};
    al_status_t s = timerange_read_without_resampling_via_uda(this, &buf, size);
    free(buf);
    EXPECT_NE(s.code, 0)
        << "a UDA time-range read now succeeds -- enable UdaSliceAndTimeRange."
           "DISABLED_TimeRangeReadWithoutResamplingThroughReopenSucceeds "
           "(OperationContext's TIMERANGE_OP constructor, src/al_context.cpp, "
           "now initializes the base interpmode member)";
    EXPECT_NE(std::string(s.message).find("unknown interp mode"),
              std::string::npos)
        << "expected the documented uninitialized-interpmode failure signature, "
           "got a different error instead: " << s.message;
}

}  // namespace

#endif  // AL_CONTRACT_HAVE_UDA
