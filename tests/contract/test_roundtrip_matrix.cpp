// Synthetic write->read round-trip matrix (issue #3).
//
// TEST_STRATEGY.md §4 step 2: the bulk of the storage contract. One round-trip
// test is value-parametrized over the always-on backend tier (decision D4)
//     {HDF5, Memory, ASCII, Flexbuffers} x {CHAR, INTEGER, DOUBLE, COMPLEX}
//     x {scalar, 1-D, ... 7-D}
// (D4's columns x D5's synthetic data). Each cell is its own ctest entry, so
// one failing datatype/shape/backend is pinpointed by name.
//
// Oracle = round-trip self-consistency (decision D5): the ABI is handed a
// deterministic synthetic buffer and must return exactly those bytes with the
// shape it was given. No Data Dictionary artifacts are required — itself the
// standing proof of the version-free storage property the migration protects.
//
// Not every (backend, datatype, shape) cell is a plain round trip. Following
// decision D2 (assert the intended contract; never freeze a bug) and D4
// (capability-gating with paired negatives), each cell is classified once:
//   * RoundTrip   - the backend stores and returns the data; assert equality.
//   * Refused     - the backend documents the operation as unsupported and must
//                   refuse it; assert the refusal (D4's paired-negative). Two
//                   cases: CHAR > 2D on the tensorizing/text backends (IMAS
//                   models CHAR as string / array-of-strings), and every cell
//                   of Flexbuffers, which is a serializer, not a pulse store —
//                   it refuses a read while writing (src/flexbuffers_backend.cpp
//                   :358) and never persists (:148). Its full serialize ->
//                   deserialize round trip needs the HLI "<buffer>" protocol and
//                   belongs to the serialize seam; here it is the must-refuse
//                   column D4 prescribes.
//   * KnownDefect - the backend crashes or corrupts on a shape it ought to
//                   handle; the cell is skipped here and the *correct* contract
//                   is asserted (expected-fail) in the RoundTripKnownDefects.*
//                   tests below, so a fix flips them to pass.

#include "al_contract.h"

#include <gtest/gtest.h>

#include <complex>
#include <cstdlib>
#include <string>
#include <tuple>
#include <vector>

using al_contract::BackendCase;
using al_contract::PulseId;

namespace {

// A magnetics IDS is used purely as an opaque container; the core attaches no
// DD semantics to it (DD paths are opaque strings through the whole C ABI).
constexpr const char* kIds = "magnetics";

// The always-on backend tier (decision D4). MDSplus/UDA are compile-guarded and
// covered elsewhere; they are deliberately absent here, not failing.
const BackendCase kBackends[] = {
    {HDF5_BACKEND, "HDF5", /*on_disk=*/true},
    {MEMORY_BACKEND, "Memory", /*on_disk=*/false},
    {ASCII_BACKEND, "ASCII", /*on_disk=*/true},
    {FLEXBUFFERS_BACKEND, "Flexbuffers", /*on_disk=*/true},
};

// Datatype axis. The enum indexes a switch that recovers the C++ element type,
// so the matrix stays a single flat Combine() rather than four near-copies.
enum class DType { Char, Int, Double, Complex };
struct DataTypeCase {
    DType       dt;
    const char* name;
};
const DataTypeCase kDataTypes[] = {
    {DType::Char, "CHAR"},
    {DType::Int, "INTEGER"},
    {DType::Double, "DOUBLE"},
    {DType::Complex, "COMPLEX"},
};

// Per-cell classification (see file header). Everything not named here is a
// plain round trip; the exceptions are the backends' real capability edges,
// each observed empirically and cross-checked against the source.
enum class Expect { RoundTrip, Refused, KnownDefect };

Expect classify(int backend_id, DType dt, int rank) {
    // Flexbuffers is a serializer, not a pulse store: it accepts the write but
    // refuses the read-back within the session, for every datatype and shape.
    if (backend_id == FLEXBUFFERS_BACKEND) {
        return Expect::Refused;
    }

    const bool is_char = (dt == DType::Char);

    // ASCII corrupts / crashes numeric data at the MAXDIM boundary (rank 7):
    // INTEGER aborts, DOUBLE/COMPLEX read back wrong. HDF5/Memory are fine at
    // rank 7, so this is an ASCII-specific defect, not a shape limit.
    if (!is_char && rank == MAXDIM && backend_id == ASCII_BACKEND) {
        return Expect::KnownDefect;
    }

    if (is_char) {
        // CHAR > 2D is documented-unsupported on the tensorizing/text backends
        // ("CHAR data > 2D is not implemented yet" on ASCII; a dimension-
        // mismatch error on HDF5). Memory keeps raw bytes and round-trips.
        if (rank >= 3 &&
            (backend_id == HDF5_BACKEND || backend_id == ASCII_BACKEND)) {
            return Expect::Refused;
        }
        // A CHAR scalar (dim 0) crashes the HDF5 backend; Memory/ASCII handle
        // it. Tracked as a defect, not frozen as "char-0D unsupported".
        if (rank == 0 && backend_id == HDF5_BACKEND) {
            return Expect::KnownDefect;
        }
    }
    return Expect::RoundTrip;
}

// A distinct opaque leaf per (datatype, rank) so cells never collide on a path.
template <class T>
std::string field_for(int rank) {
    return std::string("ids_properties/synthetic_") + al_contract::DType<T>::name +
           "_r" + std::to_string(rank);
}

// The inputs one cell operates on: the opened-entry URI plus the synthetic
// buffer, its shape, and the leaf path to store it at. Bundled so the identical
// setup preamble isn't copy-pasted between the round-trip and refusal helpers.
template <class T>
struct Cell {
    std::string      uri;
    std::vector<int> shape;
    std::vector<T>   written;
    std::string      field;
};

template <class T>
Cell<T> make_cell(const BackendCase& backend, const al_contract::TempBase& base,
                  const PulseId& pulse, int rank) {
    if (backend.on_disk) {
        base.make_legacy_tree(pulse);
    }
    Cell<T> c;
    c.uri     = al_contract::build_uri(backend.id, base.str(), pulse);
    c.shape   = al_contract::shape_for_rank(rank);
    c.written = al_contract::synth_buffer<T>(al_contract::element_count(c.shape));
    c.field   = field_for<T>(rank);
    return c;
}

// Write one field into an already-open pulse, in its own WRITE global action.
template <class T>
void write_field(int pulse_ctx, const std::string& field,
                 const std::vector<int>& shape, const std::vector<T>& data) {
    int op_ctx = -1;
    AL_ASSERT_OK(al_begin_global_action(pulse_ctx, kIds, "", WRITE_OP, &op_ctx));
    AL_EXPECT_OK(al_contract::write_data<T>(op_ctx, field.c_str(), shape, data));
    AL_ASSERT_OK(al_end_action(op_ctx));
}

// Read one field back and assert it equals what was written, byte-for-byte
// (the round-trip oracle, decision D5): same shape, same count, same values.
template <class T>
void read_field_and_expect(int pulse_ctx, const std::string& field, int rank,
                           const std::vector<int>& shape,
                           const std::vector<T>& written) {
    int op_ctx = -1;
    AL_ASSERT_OK(al_begin_global_action(pulse_ctx, kIds, "", READ_OP, &op_ctx));

    std::vector<int> read_shape;
    std::vector<T>   read_data;
    AL_EXPECT_OK(al_contract::read_data<T>(op_ctx, field.c_str(), rank,
                                           &read_shape, &read_data));

    EXPECT_EQ(read_shape, shape) << "shape changed across the round trip";
    ASSERT_EQ(read_data.size(), written.size())
        << "element count changed across the round trip";
    EXPECT_EQ(read_data, written) << "data changed across the round trip";

    AL_ASSERT_OK(al_end_action(op_ctx));
}

// One typed round trip: generate -> write -> read -> assert byte-exact equal.
// Write and read share one open data entry (mandatory for the transient Memory
// backend; fine for the persistent ones). Fatal ASSERTs return from this
// helper, which is the whole cell body, so nothing runs on a broken setup.
template <class T>
void run_round_trip(const BackendCase& backend, const al_contract::TempBase& base,
                    const PulseId& pulse, int rank) {
    const Cell<T> c = make_cell<T>(backend, base, pulse, rank);
    ASSERT_FALSE(c.uri.empty());

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(c.uri.c_str(), FORCE_CREATE_PULSE,
                                           &pulse_ctx));
    write_field<T>(pulse_ctx, c.field, c.shape, c.written);
    read_field_and_expect<T>(pulse_ctx, c.field, rank, c.shape, c.written);
    EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
}

// Paired-negative (decision D4): the operation is documented-unsupported, so
// the round-trip attempt must be refused. The refusal may surface on write
// (ASCII CHAR>2D) or on read (HDF5 CHAR>2D dimension mismatch; Flexbuffers read-
// while-serializing), so we accept either.
template <class T>
void expect_refused(const BackendCase& backend, const al_contract::TempBase& base,
                    const PulseId& pulse, int rank) {
    const Cell<T> c = make_cell<T>(backend, base, pulse, rank);
    ASSERT_FALSE(c.uri.empty());

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(c.uri.c_str(), FORCE_CREATE_PULSE,
                                           &pulse_ctx));

    al_status_t ws, rs;
    ws.code = 0;
    rs.code = 0;
    {
        int op_ctx = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, kIds, "", WRITE_OP,
                                            &op_ctx));
        ws = al_contract::write_data<T>(op_ctx, c.field.c_str(), c.shape,
                                        c.written);
        AL_EXPECT_OK(al_end_action(op_ctx));
    }
    if (ws.code == 0) {  // write was accepted; the refusal must come on read
        int op_ctx = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, kIds, "", READ_OP,
                                            &op_ctx));
        std::vector<int> sh;
        std::vector<T>   rd;
        rs = al_contract::read_data<T>(op_ctx, c.field.c_str(), rank, &sh, &rd);
        AL_EXPECT_OK(al_end_action(op_ctx));
    }
    EXPECT_TRUE(ws.code != 0 || rs.code != 0)
        << "expected the backend to refuse this datatype/shape (a documented "
           "limitation — CHAR data > 2D, or a read while the serializer is "
           "writing), but the round trip was accepted";

    EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
}

template <class T>
void dispatch(const BackendCase& backend, const al_contract::TempBase& base,
              const PulseId& pulse, int rank, Expect expect) {
    if (expect == Expect::Refused) {
        expect_refused<T>(backend, base, pulse, rank);
    } else {
        run_round_trip<T>(backend, base, pulse, rank);
    }
}

using MatrixParam = std::tuple<BackendCase, DataTypeCase, int /*rank*/>;

class RoundTripMatrix : public ::testing::TestWithParam<MatrixParam> {
protected:
    al_contract::TempBase base_;
    // One fixed pulse address; each parametrized instance has its own base_
    // dir, so on-disk trees can never alias despite the shared address.
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/12,
                   /*run=*/0};
};

TEST_P(RoundTripMatrix, ReadEqualsWrite) {
    const BackendCase  backend = std::get<0>(GetParam());
    const DataTypeCase dcase   = std::get<1>(GetParam());
    const int          rank    = std::get<2>(GetParam());

    const Expect expect = classify(backend.id, dcase.dt, rank);
    if (expect == Expect::KnownDefect) {
        GTEST_SKIP() << "known defect for " << backend.name << "/" << dcase.name
                     << "/rank " << rank
                     << " — correct contract asserted (expected-fail) in "
                        "RoundTripKnownDefects.*";
    }

    switch (dcase.dt) {
        case DType::Char:
            dispatch<char>(backend, base_, pulse_, rank, expect);
            break;
        case DType::Int:
            dispatch<int>(backend, base_, pulse_, rank, expect);
            break;
        case DType::Double:
            dispatch<double>(backend, base_, pulse_, rank, expect);
            break;
        case DType::Complex:
            dispatch<std::complex<double>>(backend, base_, pulse_, rank, expect);
            break;
    }
}

INSTANTIATE_TEST_SUITE_P(
    Backends, RoundTripMatrix,
    ::testing::Combine(::testing::ValuesIn(kBackends),
                       ::testing::ValuesIn(kDataTypes),
                       ::testing::Range(0, MAXDIM + 1)),
    [](const ::testing::TestParamInfo<MatrixParam>& info) {
        // ctest-name suffix: <Backend>_<Type>_r<rank>, e.g. HDF5_DOUBLE_r3.
        return std::string(std::get<0>(info.param).name) + "_" +
               std::get<1>(info.param).name + "_r" +
               std::to_string(std::get<2>(info.param));
    });

// ===========================================================================
// Known defects surfaced by the matrix above (decision D2).
// ===========================================================================
// The correct contract is asserted and marked expected-fail (DISABLED_ + a
// paired death test for the crash, mirroring test_known_defects.cpp). The day a
// backend is fixed, the DISABLED_ test flips to pass — a signal, not a
// regression — and its death-test twin starts failing, forcing whoever fixed it
// to enable the correct-contract test.

const BackendCase kHdf5{HDF5_BACKEND, "HDF5", /*on_disk=*/true};
const BackendCase kAscii{ASCII_BACKEND, "ASCII", /*on_disk=*/true};

// Run a round trip against a freshly-created, self-cleaning data entry — the
// standalone form used by the (non-parametrized) known-defect tests.
template <class T>
void run_round_trip_here(const BackendCase& backend, int rank) {
    al_contract::TempBase base;
    PulseId               pulse{"test", "3", 12, 0};
    run_round_trip<T>(backend, base, pulse, rank);
}

// Observe a write->read round trip WITHOUT asserting equality: returns true iff
// the data survived intact (shape + values), after asserting each op itself
// succeeded (so a "did not survive" result means genuine corruption, not a
// refusal or a broken setup). The paired DISABLED_ tests above assert survival
// as the correct contract; the current-behavior tripwires below use this to pin
// today's *silent corruption* (the op returns success but the bytes come back
// wrong), so a fix that restores the data flips the tripwire red.
template <class T>
bool maxdim_round_trip_survives(const BackendCase& backend, int rank) {
    al_contract::TempBase base;
    PulseId               pulse{"test", "3", 12, 0};
    const Cell<T>         c = make_cell<T>(backend, base, pulse, rank);
    EXPECT_FALSE(c.uri.empty());

    int pulse_ctx = -1;
    EXPECT_EQ(
        al_begin_dataentry_action(c.uri.c_str(), FORCE_CREATE_PULSE, &pulse_ctx)
            .code,
        0);
    {
        int op_ctx = -1;
        EXPECT_EQ(
            al_begin_global_action(pulse_ctx, kIds, "", WRITE_OP, &op_ctx).code,
            0);
        EXPECT_EQ(al_contract::write_data<T>(op_ctx, c.field.c_str(), c.shape,
                                             c.written)
                      .code,
                  0);
        EXPECT_EQ(al_end_action(op_ctx).code, 0);
    }
    std::vector<int> read_shape;
    std::vector<T>   read_data;
    {
        int op_ctx = -1;
        EXPECT_EQ(
            al_begin_global_action(pulse_ctx, kIds, "", READ_OP, &op_ctx).code,
            0);
        EXPECT_EQ(al_contract::read_data<T>(op_ctx, c.field.c_str(), rank,
                                            &read_shape, &read_data)
                      .code,
                  0);
        EXPECT_EQ(al_end_action(op_ctx).code, 0);
    }
    al_close_pulse(pulse_ctx, CLOSE_PULSE);
    return read_shape == c.shape && read_data == c.written;
}

// --- HDF5 crashes writing/reading a CHAR scalar (dim 0) --------------------
TEST(RoundTripKnownDefects, DISABLED_Hdf5CharScalarRoundTrips) {
    run_round_trip_here<char>(kHdf5, /*rank=*/0);
}

TEST(RoundTripKnownDefectsDeath, Hdf5CharScalarCurrentlyCrashes) {
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    // ASSERT_DEATH's argument must be a single call: a brace block would expose
    // the commas in the initializers as extra macro arguments.
    ASSERT_DEATH(run_round_trip_here<char>(kHdf5, /*rank=*/0), ".*")
        << "expected the HDF5 CHAR-scalar defect to crash. If it no longer "
           "does, enable RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips.";
}

// --- ASCII corrupts/crashes numeric data at the MAXDIM boundary (rank 7) ----
TEST(RoundTripKnownDefects, DISABLED_AsciiIntegerMaxdimRoundTrips) {
    run_round_trip_here<int>(kAscii, /*rank=*/MAXDIM);
}
TEST(RoundTripKnownDefects, DISABLED_AsciiDoubleMaxdimRoundTrips) {
    run_round_trip_here<double>(kAscii, /*rank=*/MAXDIM);
}
TEST(RoundTripKnownDefects, DISABLED_AsciiComplexMaxdimRoundTrips) {
    run_round_trip_here<std::complex<double>>(kAscii, /*rank=*/MAXDIM);
}

// Current-behavior tripwires for the ASCII rank-7 defects, so the DISABLED_
// correct-contract tests above can't rot. INTEGER reliably aborts the process
// (a plain death test). DOUBLE and COMPLEX overrun the heap (issue #9): most
// runs complete the op and return corrupted bytes, but the overrun is UB and
// intermittently faults instead (observed: occasional SIGBUS on macOS). Each
// of those tripwires therefore runs the round trip in a death-test child that
// exits kMaxdimSurvivedExit only on an intact round trip — corruption and
// crash both keep the tripwire green; only a clean round trip turns it red,
// forcing whoever fixed ASCII to enable the paired DISABLED_ test.
//
// NB: under AddressSanitizer the corruption is masked (the data round-trips
// intact there), so the two `…CurrentlyCorrupts` tripwires are excluded from
// the sanitizer CI leg (.github/workflows/sanitizers.yml,
// `-E CurrentlyCorrupts`). Their rot-guard job runs in the blocking functional
// leg (.github/workflows/contract-tests.yml) and in local `ctest` runs.
constexpr int kMaxdimSurvivedExit = 77;

template <typename T>
void exit_with_maxdim_survival(const BackendCase& backend) {
    const bool survives =
        maxdim_round_trip_survives<T>(backend, /*rank=*/MAXDIM);
    std::_Exit(survives ? kMaxdimSurvivedExit : 0);
}

TEST(RoundTripKnownDefectsDeath, AsciiIntegerMaxdimCurrentlyAborts) {
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    ASSERT_DEATH(run_round_trip_here<int>(kAscii, /*rank=*/MAXDIM), ".*")
        << "expected ASCII rank-7 INTEGER to abort. If it no longer does, the "
           "defect was likely fixed — enable "
           "RoundTripKnownDefects.DISABLED_AsciiIntegerMaxdimRoundTrips.";
}

TEST(RoundTripKnownDefectsDeath, AsciiDoubleMaxdimCurrentlyCorrupts) {
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    const ::testing::ExitedWithCode survived(kMaxdimSurvivedExit);
    EXPECT_EXIT(
        exit_with_maxdim_survival<double>(kAscii),
        [&](int status) { return !survived(status); }, "")
        << "ASCII rank-7 DOUBLE now round-trips intact — the defect is fixed; "
           "enable RoundTripKnownDefects.DISABLED_AsciiDoubleMaxdimRoundTrips.";
}

TEST(RoundTripKnownDefectsDeath, AsciiComplexMaxdimCurrentlyCorrupts) {
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    const ::testing::ExitedWithCode survived(kMaxdimSurvivedExit);
    EXPECT_EXIT(
        exit_with_maxdim_survival<std::complex<double>>(kAscii),
        [&](int status) { return !survived(status); }, "")
        << "ASCII rank-7 COMPLEX now round-trips intact — the defect is fixed; "
           "enable RoundTripKnownDefects.DISABLED_AsciiComplexMaxdimRoundTrips.";
}

// --- al_build_uri_from_legacy_parameters can't address FLEXBUFFERS ----------
// getURIBackend (src/al_context.cpp:280) has no FLEXBUFFERS_BACKEND case and
// throws, even though the parse side accepts the "flexbuffers" scheme — so the
// always-on serializer backend has no legacy-URI builder (the matrix works
// around it by hand-building the URI, see al_contract.h::build_uri). Correct
// contract: the builder should support it, like the other always-on backends.
TEST(RoundTripKnownDefects, DISABLED_BuildUriSupportsFlexbuffers) {
    char*       uri = nullptr;
    al_status_t s   = al_build_uri_from_legacy_parameters(
        FLEXBUFFERS_BACKEND, 12, 0, "/tmp/al_contract_flexbuffers", "test", "3",
        "", &uri);
    EXPECT_EQ(s.code, 0)
        << "al_build_uri_from_legacy_parameters should support the always-on "
           "FLEXBUFFERS backend (getURIBackend, src/al_context.cpp:280)";
    free(uri);
}

// Current-behavior tripwire: today the builder errors for FLEXBUFFERS (code -4),
// so al_contract.h::build_uri hand-builds the URI to keep the matrix running.
// When the builder is fixed this goes red, forcing whoever fixed it to enable
// DISABLED_BuildUriSupportsFlexbuffers and drop the hand-built workaround.
TEST(RoundTripKnownDefects, BuildUriFlexbuffersCurrentlyFails) {
    char*       uri = nullptr;
    al_status_t s   = al_build_uri_from_legacy_parameters(
        FLEXBUFFERS_BACKEND, 12, 0, "/tmp/al_contract_flexbuffers", "test", "3",
        "", &uri);
    EXPECT_NE(s.code, 0)
        << "al_build_uri_from_legacy_parameters now supports FLEXBUFFERS — "
           "enable RoundTripKnownDefects.DISABLED_BuildUriSupportsFlexbuffers "
           "and drop the hand-built URI workaround in al_contract.h::build_uri.";
    free(uri);
}

}  // namespace
