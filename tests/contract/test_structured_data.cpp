// Structured-data contract tests (issue #4 / TEST_STRATEGY.md §4 step 2).
//
// Covers what the round-trip matrix (issue #3) and capability-gated suite
// (issue #6) deliberately left as gaps: arrays-of-structures (top-level +
// nested), al_delete_data at its three documented granularities,
// al_get_occurrences, and the realistic equilibrium-seed round trip.
//
// Same idioms as the existing suite: al_contract.h for the fixture/generator
// layer, a per-file backend-capability table + TEST_P (test_capability_gated.cpp),
// and local DISABLED_/tripwire pairs for genuine defects, never frozen as spec
// (decision D2).
//
// This is characterization work, not new product code: every "positive" test
// below asserts real, already-shipped C-ABI behavior; every DISABLED_/tripwire
// pair pins a real defect found by reading the backend sources directly
// (src/hdf5/, src/memory_backend.*, src/ascii_backend.*, src/flexbuffers_backend.*)
// and confirmed by running the test against the built library.

#include "al_contract.h"
#include "equilibrium_seed.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

using al_contract::BackendCase;
using al_contract::PulseId;

namespace {

constexpr const char* kIds = "magnetics";

// ===========================================================================
// Arrays of structures: top-level + nested, write -> iterate -> read.
// ===========================================================================
// Flexbuffers is excluded from the round-trip expectation, consistent with
// RoundTripMatrix's classification (test_roundtrip_matrix.cpp): it is a
// serializer, not a pulse store, and refuses reads within the same session
// for ordinary fields; AOS content is read via the same field read/write path
// so the same refusal applies.
// ASCII is a KnownDefect, not a plain refusal: al_begin_arraystruct_action's
// READ side always reports size 0, for any AOS, regardless of what was
// written — confirmed empirically and traced to AsciiBackend::beginAction
// (READ_OP) consuming the *entire* file into a random-access lookup map
// (`curcontent_map`, ascii_backend.cpp:213-235) before returning, which
// leaves the sequential stream position at EOF. beginReadArraystructAction's
// AOS-size lookup (ascii_backend.cpp:656-682) depends on that same sequential
// position via `getline`, so it always finds nothing. See AosKnownDefects
// below for the paired DISABLED_/tripwire.
// MDSplus is a Divergence, not a defect (issue #14): unlike every other
// backend here, it resolves paths against a real DD-baked model tree, so
// this fixture's synthetic field names ("elements"/"val", "outer"/"inner")
// have no corresponding node. Confirmed empirically: al_begin_arraystruct_action
// and the per-element al_write_data calls all report success (buffered in an
// in-memory MDSplus::Apd), but the AOS flush at al_end_action throws
// "%TREE-W-NNF, Node Not Found". Real DD-conformant AOS paths (e.g.
// equilibrium/time_slice/global_quantities/ip) round-trip through this exact
// same begin/write/iterate/end sequence without error — this is a structural
// requirement of MDSplus's storage model (issue #12 Q2), not a bug, so it is
// not paired with a DISABLED_/tripwire like KnownDefect. See TRACEABILITY.md
// Part 4.
enum class AosExpect { RoundTrip, Refused, KnownDefect, Divergence };

struct AosBackendCase {
    int         id;
    const char* name;
    bool        on_disk;
    AosExpect   expect;
};

inline void PrintTo(const AosBackendCase& b, std::ostream* os) { *os << b.name; }

const AosBackendCase kAosBackends[] = {
    {HDF5_BACKEND, "HDF5", /*on_disk=*/true, AosExpect::RoundTrip},
    {MEMORY_BACKEND, "Memory", /*on_disk=*/false, AosExpect::RoundTrip},
    {ASCII_BACKEND, "ASCII", /*on_disk=*/true, AosExpect::KnownDefect},
    {FLEXBUFFERS_BACKEND, "Flexbuffers", /*on_disk=*/true, AosExpect::Refused},
#ifdef AL_CONTRACT_HAVE_MDSPLUS
    {MDSPLUS_BACKEND, "Mdsplus", /*on_disk=*/true, AosExpect::Divergence},
#endif
};

// Shared by AosMatrix and DeleteMatrix below (their backend-descriptor
// structs differ, but each carries the same id/on_disk pair).
int open_fresh_pulse(int backend_id, bool on_disk, const al_contract::TempBase& base,
                     const PulseId& pulse) {
    if (on_disk) base.make_legacy_tree(pulse);
    const std::string uri = al_contract::build_uri(backend_id, base.str(), pulse);
    EXPECT_FALSE(uri.empty());
    int pctx = -1;
    EXPECT_EQ(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx).code, 0);
    return pctx;
}

class AosMatrix : public ::testing::TestWithParam<AosBackendCase> {
protected:
    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/12, /*run=*/0};
};

TEST_P(AosMatrix, TopLevelWriteIterateRead) {
    const AosBackendCase b = GetParam();
    if (b.expect == AosExpect::KnownDefect) {
        GTEST_SKIP() << "known defect for " << b.name
                     << " AOS read — see AosKnownDefects.*";
    }
    if (b.expect == AosExpect::Divergence) {
        GTEST_SKIP() << b.name << " requires real DD-conformant AOS paths "
                        "(issue #12 Q2) -- this fixture's synthetic field "
                        "names are refused, not a defect. See "
                        "TRACEABILITY.md Part 4.";
    }
    const int pctx = open_fresh_pulse(b.id, b.on_disk, base_, pulse_);
    constexpr int kN = 3;

    // --- write: 3-element top-level AOS, one INTEGER field per element -----
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op));
    int size = kN;
    int aos = -1;
    al_status_t begin_status =
        al_begin_arraystruct_action(op, "elements", "", &size, &aos);
    for (int i = 0; i < kN && begin_status.code == 0; ++i) {
        AL_EXPECT_OK(al_contract::write_data<int>(aos, "val", {}, {100 + i}));
        if (i + 1 < kN) AL_EXPECT_OK(al_iterate_over_arraystruct(aos, 1));
    }
    if (aos != 0) AL_EXPECT_OK(al_end_action(aos));
    AL_ASSERT_OK(al_end_action(op));

    // --- read: same shape back ----------------------------------------------
    int op2 = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", READ_OP, &op2));
    int rsize = 0;
    int raos = -1;
    al_status_t read_status =
        al_begin_arraystruct_action(op2, "elements", "", &rsize, &raos);

    if (b.expect == AosExpect::RoundTrip) {
        AL_EXPECT_OK(begin_status);
        AL_EXPECT_OK(read_status);
        ASSERT_EQ(rsize, kN) << "AOS size must round-trip";
        for (int i = 0; i < rsize; ++i) {
            std::vector<int> shape, data;
            AL_EXPECT_OK(al_contract::read_data<int>(raos, "val", 0, &shape, &data));
            ASSERT_EQ(data.size(), 1u);
            EXPECT_EQ(data[0], 100 + i) << "element " << i << " mismatch";
            if (i + 1 < rsize) AL_EXPECT_OK(al_iterate_over_arraystruct(raos, 1));
        }
    } else {
        EXPECT_TRUE(begin_status.code != 0 || read_status.code != 0 || rsize == 0)
            << "expected the serializer backend to refuse the AOS read-back "
               "within the same session";
    }
    if (raos != 0) al_end_action(raos);
    al_end_action(op2);
    al_close_pulse(pctx, CLOSE_PULSE);
}

TEST_P(AosMatrix, NestedWriteIterateRead) {
    const AosBackendCase b = GetParam();
    if (b.expect == AosExpect::KnownDefect) {
        GTEST_SKIP() << "known defect for " << b.name
                     << " AOS read — see AosKnownDefects.*";
    }
    if (b.expect == AosExpect::Divergence) {
        GTEST_SKIP() << b.name << " requires real DD-conformant AOS paths "
                        "(issue #12 Q2) -- this fixture's synthetic field "
                        "names are refused, not a defect. See "
                        "TRACEABILITY.md Part 4.";
    }
    const int pctx = open_fresh_pulse(b.id, b.on_disk, base_, pulse_);
    constexpr int kOuter = 2, kInner = 2;

    // --- write: outer AOS of size 2, each element holds an inner AOS of size 2
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op));
    int outer_size = kOuter;
    int outer = -1;
    al_status_t outer_status =
        al_begin_arraystruct_action(op, "outer", "", &outer_size, &outer);
    for (int oi = 0; oi < kOuter && outer_status.code == 0; ++oi) {
        int inner_size = kInner;
        int inner = -1;
        AL_EXPECT_OK(al_begin_arraystruct_action(outer, "inner", "", &inner_size, &inner));
        for (int ii = 0; ii < kInner; ++ii) {
            AL_EXPECT_OK(al_contract::write_data<int>(inner, "val", {}, {oi * 10 + ii}));
            if (ii + 1 < kInner) AL_EXPECT_OK(al_iterate_over_arraystruct(inner, 1));
        }
        if (inner != 0) AL_EXPECT_OK(al_end_action(inner));
        if (oi + 1 < kOuter) AL_EXPECT_OK(al_iterate_over_arraystruct(outer, 1));
    }
    if (outer != 0) AL_EXPECT_OK(al_end_action(outer));
    AL_ASSERT_OK(al_end_action(op));

    // --- read: same nested shape back --------------------------------------
    int op2 = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", READ_OP, &op2));
    int router_size = 0;
    int router = -1;
    al_status_t router_status =
        al_begin_arraystruct_action(op2, "outer", "", &router_size, &router);

    if (b.expect == AosExpect::RoundTrip) {
        AL_EXPECT_OK(outer_status);
        AL_EXPECT_OK(router_status);
        ASSERT_EQ(router_size, kOuter);
        for (int oi = 0; oi < router_size; ++oi) {
            int rinner_size = 0;
            int rinner = -1;
            AL_EXPECT_OK(
                al_begin_arraystruct_action(router, "inner", "", &rinner_size, &rinner));
            ASSERT_EQ(rinner_size, kInner) << "nested AOS size must round-trip";
            for (int ii = 0; ii < rinner_size; ++ii) {
                std::vector<int> shape, data;
                AL_EXPECT_OK(al_contract::read_data<int>(rinner, "val", 0, &shape, &data));
                ASSERT_EQ(data.size(), 1u);
                EXPECT_EQ(data[0], oi * 10 + ii)
                    << "outer=" << oi << " inner=" << ii << " mismatch";
                if (ii + 1 < rinner_size) AL_EXPECT_OK(al_iterate_over_arraystruct(rinner, 1));
            }
            if (rinner != 0) al_end_action(rinner);
            if (oi + 1 < router_size) AL_EXPECT_OK(al_iterate_over_arraystruct(router, 1));
        }
    } else {
        EXPECT_TRUE(outer_status.code != 0 || router_status.code != 0 || router_size == 0)
            << "expected the serializer backend to refuse the nested AOS "
               "read-back within the same session";
    }
    if (router != 0) al_end_action(router);
    al_end_action(op2);
    al_close_pulse(pctx, CLOSE_PULSE);
}

INSTANTIATE_TEST_SUITE_P(
    Backends, AosMatrix, ::testing::ValuesIn(kAosBackends),
    [](const ::testing::TestParamInfo<AosBackendCase>& info) {
        return std::string(info.param.name);
    });

// ---------------------------------------------------------------------------
// ASCII AOS-read known defect (decision D2): correct-contract DISABLED_ test
// paired with a current-behavior tripwire.
// ---------------------------------------------------------------------------
int ascii_aos_read_reported_size() {
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 12, 0};
    base.make_legacy_tree(pulse);
    const std::string uri = al_contract::build_uri(ASCII_BACKEND, base.str(), pulse);
    int pctx = -1;
    EXPECT_EQ(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx).code, 0);

    int op = -1;
    EXPECT_EQ(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op).code, 0);
    int size = 3;
    int aos = -1;
    EXPECT_EQ(al_begin_arraystruct_action(op, "elements", "", &size, &aos).code, 0);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(al_contract::write_data<int>(aos, "val", {}, {100 + i}).code, 0);
        if (i + 1 < 3) EXPECT_EQ(al_iterate_over_arraystruct(aos, 1).code, 0);
    }
    EXPECT_EQ(al_end_action(aos).code, 0);
    EXPECT_EQ(al_end_action(op).code, 0);

    int rop = -1;
    EXPECT_EQ(al_begin_global_action(pctx, kIds, "", READ_OP, &rop).code, 0);
    int rsize = -1;
    int raos = -1;
    al_begin_arraystruct_action(rop, "elements", "", &rsize, &raos);
    if (raos != 0) al_end_action(raos);
    al_end_action(rop);
    al_close_pulse(pctx, CLOSE_PULSE);
    return rsize;
}

TEST(AosKnownDefects, DISABLED_AsciiAosReadReportsWrittenSize) {
    EXPECT_EQ(ascii_aos_read_reported_size(), 3)
        << "reading an AOS back must report the size that was written";
}

TEST(AosKnownDefects, AsciiAosReadCurrentlyReportsZero) {
    EXPECT_EQ(ascii_aos_read_reported_size(), 0)
        << "AsciiBackend now reports the correct AOS size on read — enable "
           "AosKnownDefects.DISABLED_AsciiAosReadReportsWrittenSize "
           "(ascii_backend.cpp's READ_OP setup consumes the whole stream "
           "into curcontent_map before beginReadArraystructAction's "
           "sequential lookup ever runs)";
}

// ===========================================================================
// Equilibrium seed: realistic scalar + timebase-carrying 2-D array + AOS,
// oracle = content hash (decision D5). Flexbuffers is excluded for the same
// reason as the AOS matrix above (read refused within the session).
//
// MDSplus (issue #14) is excluded too, for a Divergence rather than a defect:
// unlike the always-on backends, it resolves every path against a real
// DD-baked model tree. Confirmed empirically -- the seed's SCALAR sub-shape
// ("vacuum_toroidal_field/r0") round-trips fine (that is exactly what the
// issue #13 tracer bullet, tests/contract/test_mdsplus.cpp, already pins),
// but the seed's other two sub-shapes don't match the real equilibrium DD
// layout MDSplus enforces: writing "profiles_1d/psi" as a flat tensorized
// 2-D array (the HDF5-tensorization convention this generator targets)
// throws "%TREE-W-NNF, Node Not Found" immediately, because MDSplus's real
// profiles_1d is a genuine dynamic array-of-structures, not a flat dataset at
// that path; and the "constraints" AOS's generic {measured, weight} fields
// don't exist in the real DD equilibrium/constraints structure (a fixed
// container of specific constraint-type sub-objects), so the AOS write
// buffers successfully but throws the same NNF when flushed at
// al_end_action. Real DD-conformant AOS/timed paths do round-trip on MDSplus
// through this exact write/read sequence -- the divergence is specific to
// this generator's DD-agnostic shape choices, not a general MDSplus
// limitation. See TRACEABILITY.md Part 4.
// ===========================================================================
class EquilibriumSeedMatrix : public ::testing::TestWithParam<BackendCase> {
protected:
    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/12, /*run=*/0};
};

const BackendCase kSeedBackends[] = {
    {HDF5_BACKEND, "HDF5", /*on_disk=*/true},
    {MEMORY_BACKEND, "Memory", /*on_disk=*/false},
    {ASCII_BACKEND, "ASCII", /*on_disk=*/true},
};

TEST_P(EquilibriumSeedMatrix, RoundTripHashMatches) {
    const BackendCase b = GetParam();
    if (b.on_disk) base_.make_legacy_tree(pulse_);
    const std::string uri = al_contract::build_uri(b.id, base_.str(), pulse_);
    ASSERT_FALSE(uri.empty());

    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx));
    AL_ASSERT_OK(equilibrium_seed::write(pctx));

    uint64_t hash = 0;
    AL_ASSERT_OK(equilibrium_seed::read_and_hash(pctx, &hash));
    EXPECT_EQ(hash, equilibrium_seed::expected_hash())
        << "equilibrium seed (scalar + timebase-carrying 2-D array + "
           "constraints AOS) must round-trip exactly";

    EXPECT_EQ(al_close_pulse(pctx, CLOSE_PULSE).code, 0);
}

INSTANTIATE_TEST_SUITE_P(
    Backends, EquilibriumSeedMatrix, ::testing::ValuesIn(kSeedBackends),
    [](const ::testing::TestParamInfo<BackendCase>& info) {
        return std::string(info.param.name);
    });

// ===========================================================================
// al_delete_data at signal / structure / DATAOBJECT-root granularity.
// ===========================================================================
// The three granularities land on wildly different, non-overlapping support
// across the always-on tier (verified by reading each backend's deleteData):
//   - HDF5: ignores `path` entirely — every call deletes the whole occurrence.
//     So *root* is the only granularity that matches the documented contract;
//     leaf/structure are genuine defects (a "leaf" delete takes everything
//     else with it). One tripwire (leaf) covers both defective granularities:
//     `HDF5Writer::deleteData` (src/hdf5/hdf5_writer.cpp) takes no path
//     parameter at all, so there is no leaf-vs-structure branch whose
//     behavior could differ — a structure-shaped call hits the exact same
//     unconditional occurrence wipe.
//   - Memory: the only backend that honors `path` — leaf and structure (an
//     AOS field) both work for real; there is no code path that special-cases
//     "delete the whole IDS", so root is the defect here.
//   - ASCII, Flexbuffers: deleteData is a literal no-op (empty body, doc
//     comment says so) — none of the three granularities do anything. One
//     tripwire (leaf) suffices: an empty function body has no path-dependent
//     branch to differ between granularities either.
//
// A "deleted" read is not a documented refusal: al_plugin_read_data returns
// status.code==0 with the datatype's sentinel value when the backend reports
// "no data" (src/al_lowlevel.cpp:1195-1199, Lowlevel::setDefaultValue) — so
// the oracle for "is it gone" is the sentinel, not an error code.
struct DeleteBackendCase {
    int         id;
    const char* name;
    bool        on_disk;
    bool        leaf_ok;
    bool        structure_ok;
    bool        root_ok;
};

inline void PrintTo(const DeleteBackendCase& b, std::ostream* os) { *os << b.name; }

const DeleteBackendCase kDeleteBackends[] = {
    {HDF5_BACKEND, "HDF5", /*on_disk=*/true, /*leaf_ok=*/false,
     /*structure_ok=*/false, /*root_ok=*/true},
    {MEMORY_BACKEND, "Memory", /*on_disk=*/false, /*leaf_ok=*/true,
     /*structure_ok=*/true, /*root_ok=*/false},
    {ASCII_BACKEND, "ASCII", /*on_disk=*/true, /*leaf_ok=*/false,
     /*structure_ok=*/false, /*root_ok=*/false},
    {FLEXBUFFERS_BACKEND, "Flexbuffers", /*on_disk=*/true, /*leaf_ok=*/false,
     /*structure_ok=*/false, /*root_ok=*/false},
};

class DeleteMatrix : public ::testing::TestWithParam<DeleteBackendCase> {
protected:
    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/12, /*run=*/0};
};

TEST_P(DeleteMatrix, LeafDeleteRemovesJustTheLeaf) {
    const DeleteBackendCase b = GetParam();
    if (!b.leaf_ok) {
        GTEST_SKIP() << "known defect for " << b.name
                     << " leaf delete — see DeleteKnownDefects.*";
    }
    const int pctx = open_fresh_pulse(b.id, b.on_disk, base_, pulse_);

    // Write and delete in the SAME WRITE_OP session: ASCII truncates its
    // pulsefile (std::ios::out|std::ios::trunc) every time a WRITE_OP begins
    // (ascii_backend.cpp:238), so a second, separate write-mode session would
    // wipe the data itself and make the delete look effective when it isn't.
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op));
    AL_EXPECT_OK(al_contract::write_data<double>(op, "leaf_a", {}, {11.0}));
    AL_EXPECT_OK(al_contract::write_data<double>(op, "leaf_b", {}, {22.0}));
    AL_EXPECT_OK(al_delete_data(op, "leaf_a"));
    AL_ASSERT_OK(al_end_action(op));

    int rop = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", READ_OP, &rop));
    std::vector<int> shape;
    std::vector<double> a, bb;
    AL_EXPECT_OK(al_contract::read_data<double>(rop, "leaf_a", 0, &shape, &a));
    AL_EXPECT_OK(al_contract::read_data<double>(rop, "leaf_b", 0, &shape, &bb));
    EXPECT_EQ(a.at(0), al_contract::kEmptyDouble) << "deleted leaf must read back absent";
    EXPECT_EQ(bb.at(0), 22.0) << "sibling leaf must survive a leaf-granularity delete";
    AL_ASSERT_OK(al_end_action(rop));

    al_close_pulse(pctx, CLOSE_PULSE);
}

TEST_P(DeleteMatrix, StructureDeleteRemovesWholeSubtree) {
    const DeleteBackendCase b = GetParam();
    if (!b.structure_ok) {
        GTEST_SKIP() << "known defect for " << b.name
                     << " structure delete — see DeleteKnownDefects.*";
    }
    const int pctx = open_fresh_pulse(b.id, b.on_disk, base_, pulse_);

    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op));
    int size = 2;
    int aos = -1;
    AL_ASSERT_OK(al_begin_arraystruct_action(op, "block", "", &size, &aos));
    AL_EXPECT_OK(al_contract::write_data<double>(aos, "x", {}, {1.0}));
    AL_EXPECT_OK(al_iterate_over_arraystruct(aos, 1));
    AL_EXPECT_OK(al_contract::write_data<double>(aos, "x", {}, {2.0}));
    AL_ASSERT_OK(al_end_action(aos));
    AL_EXPECT_OK(al_contract::write_data<double>(op, "sibling", {}, {99.0}));
    AL_EXPECT_OK(al_delete_data(op, "block"));
    AL_ASSERT_OK(al_end_action(op));

    int rop = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", READ_OP, &rop));
    int rsize = -1;
    int raos = -1;
    al_status_t s = al_begin_arraystruct_action(rop, "block", "", &rsize, &raos);
    std::vector<int> shape;
    std::vector<double> sibling;
    AL_EXPECT_OK(al_contract::read_data<double>(rop, "sibling", 0, &shape, &sibling));
    EXPECT_TRUE(s.code != 0 || rsize == 0) << "deleted structure must not report elements";
    EXPECT_EQ(sibling.at(0), 99.0) << "sibling field must survive a structure delete";
    if (raos != 0) al_end_action(raos);
    AL_ASSERT_OK(al_end_action(rop));

    al_close_pulse(pctx, CLOSE_PULSE);
}

TEST_P(DeleteMatrix, RootDeleteRemovesWholeOccurrence) {
    const DeleteBackendCase b = GetParam();
    if (!b.root_ok) {
        GTEST_SKIP() << "known defect for " << b.name
                     << " root delete — see DeleteKnownDefects.*";
    }
    const int pctx = open_fresh_pulse(b.id, b.on_disk, base_, pulse_);

    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op));
    AL_EXPECT_OK(al_contract::write_data<double>(op, "leaf_a", {}, {11.0}));
    AL_EXPECT_OK(al_contract::write_data<double>(op, "leaf_b", {}, {22.0}));
    AL_EXPECT_OK(al_delete_data(op, ""));
    AL_ASSERT_OK(al_end_action(op));

    // A root/occurrence delete may remove the container so thoroughly that
    // even opening a fresh READ action on it fails outright (verified: HDF5's
    // deleteData removes the external link and the on-disk IDS file, so
    // reopening "magnetics" for READ throws "Unable to open HDF5 group").
    // That failure is itself proof the occurrence is gone; if the backend
    // instead tolerates reopening, every leaf must read back absent.
    int rop = -1;
    al_status_t open_read = al_begin_global_action(pctx, kIds, "", READ_OP, &rop);
    if (open_read.code == 0) {
        std::vector<int> shape;
        std::vector<double> a, bb;
        AL_EXPECT_OK(al_contract::read_data<double>(rop, "leaf_a", 0, &shape, &a));
        AL_EXPECT_OK(al_contract::read_data<double>(rop, "leaf_b", 0, &shape, &bb));
        EXPECT_EQ(a.at(0), al_contract::kEmptyDouble) << "root delete must remove every leaf";
        EXPECT_EQ(bb.at(0), al_contract::kEmptyDouble) << "root delete must remove every leaf";
        AL_ASSERT_OK(al_end_action(rop));
    }

    al_close_pulse(pctx, CLOSE_PULSE);
}

INSTANTIATE_TEST_SUITE_P(
    Backends, DeleteMatrix, ::testing::ValuesIn(kDeleteBackends),
    [](const ::testing::TestParamInfo<DeleteBackendCase>& info) {
        return std::string(info.param.name);
    });

// ---------------------------------------------------------------------------
// Known defects for al_delete_data (decision D2): correct-contract DISABLED_
// tests, each paired with a current-behavior tripwire.
// ---------------------------------------------------------------------------
class DeleteKnownDefects : public ::testing::Test {
protected:
    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/12, /*run=*/0};

    int open(const BackendCase& b) {
        if (b.on_disk) base_.make_legacy_tree(pulse_);
        const std::string uri = al_contract::build_uri(b.id, base_.str(), pulse_);
        EXPECT_FALSE(uri.empty());
        int pctx = -1;
        EXPECT_EQ(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx).code, 0);
        return pctx;
    }
};

const BackendCase kHdf5{HDF5_BACKEND, "HDF5", /*on_disk=*/true};
const BackendCase kAscii{ASCII_BACKEND, "ASCII", /*on_disk=*/true};

// --- HDF5: leaf delete ignores `path`, wipes the whole occurrence ----------
double hdf5_leaf_delete_sibling_survives(al_contract::TempBase& base,
                                         const PulseId& pulse) {
    const std::string uri = al_contract::build_uri(HDF5_BACKEND, base.str(), pulse);
    int pctx = -1;
    EXPECT_EQ(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx).code, 0);
    int op = -1;
    EXPECT_EQ(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op).code, 0);
    EXPECT_EQ(al_contract::write_data<double>(op, "leaf_a", {}, {11.0}).code, 0);
    EXPECT_EQ(al_contract::write_data<double>(op, "leaf_b", {}, {22.0}).code, 0);
    EXPECT_EQ(al_delete_data(op, "leaf_a").code, 0);
    EXPECT_EQ(al_end_action(op).code, 0);

    // Reopening a READ on the occurrence may itself fail — HDF5's delete
    // removed the whole occurrence's group/link/file, so there may be
    // nothing left to open at all. That failure is just as much proof the
    // sibling is gone as reading back the sentinel would be.
    int rop = -1;
    al_status_t open_read = al_begin_global_action(pctx, kIds, "", READ_OP, &rop);
    double result = al_contract::kEmptyDouble;
    if (open_read.code == 0) {
        std::vector<int> shape;
        std::vector<double> bb;
        EXPECT_EQ(al_contract::read_data<double>(rop, "leaf_b", 0, &shape, &bb).code, 0);
        EXPECT_EQ(al_end_action(rop).code, 0);
        if (!bb.empty()) result = bb[0];
    }
    al_close_pulse(pctx, CLOSE_PULSE);
    return result;
}

TEST_F(DeleteKnownDefects, DISABLED_Hdf5LeafDeleteLeavesSiblingIntact) {
    EXPECT_EQ(hdf5_leaf_delete_sibling_survives(base_, pulse_), 22.0)
        << "a leaf-granularity delete must not remove sibling fields";
}

TEST_F(DeleteKnownDefects, Hdf5LeafDeleteCurrentlyWipesWholeOccurrence) {
    EXPECT_EQ(hdf5_leaf_delete_sibling_survives(base_, pulse_),
              al_contract::kEmptyDouble)
        << "HDF5 deleteData now honors `path` for leaf granularity — enable "
           "DeleteKnownDefects.DISABLED_Hdf5LeafDeleteLeavesSiblingIntact "
           "(src/hdf5/hdf5_writer.cpp deleteData ignores its caller's field)";
}

// --- Memory: no code path implements DATAOBJECT-root delete ----------------
bool memory_root_delete_clears_everything(al_contract::TempBase& base,
                                          const PulseId& pulse) {
    const std::string uri = al_contract::build_uri(MEMORY_BACKEND, base.str(), pulse);
    int pctx = -1;
    EXPECT_EQ(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx).code, 0);
    int op = -1;
    EXPECT_EQ(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op).code, 0);
    EXPECT_EQ(al_contract::write_data<double>(op, "leaf_a", {}, {11.0}).code, 0);
    EXPECT_EQ(al_contract::write_data<double>(op, "leaf_b", {}, {22.0}).code, 0);
    EXPECT_EQ(al_delete_data(op, "").code, 0);
    EXPECT_EQ(al_end_action(op).code, 0);
    int rop = -1;
    EXPECT_EQ(al_begin_global_action(pctx, kIds, "", READ_OP, &rop).code, 0);
    std::vector<int> shape;
    std::vector<double> a, bb;
    EXPECT_EQ(al_contract::read_data<double>(rop, "leaf_a", 0, &shape, &a).code, 0);
    EXPECT_EQ(al_contract::read_data<double>(rop, "leaf_b", 0, &shape, &bb).code, 0);
    EXPECT_EQ(al_end_action(rop).code, 0);
    al_close_pulse(pctx, CLOSE_PULSE);
    return !a.empty() && !bb.empty() && a[0] == al_contract::kEmptyDouble &&
           bb[0] == al_contract::kEmptyDouble;
}

TEST_F(DeleteKnownDefects, DISABLED_MemoryRootDeleteClearsWholeIds) {
    EXPECT_TRUE(memory_root_delete_clears_everything(base_, pulse_))
        << "a DATAOBJECT-root delete must clear every field of the IDS";
}

TEST_F(DeleteKnownDefects, MemoryRootDeleteCurrentlyDoesNothing) {
    EXPECT_FALSE(memory_root_delete_clears_everything(base_, pulse_))
        << "MemoryBackend::deleteData now supports root granularity — enable "
           "DeleteKnownDefects.DISABLED_MemoryRootDeleteClearsWholeIds "
           "(memory_backend.cpp deleteData has no root/occurrence case)";
}

// --- ASCII: deleteData is a literal no-op ----------------------------------
// Flexbuffers is deliberately NOT covered by this pair: it refuses every
// in-session read regardless of delete (RoundTripMatrix classifies the whole
// backend as a must-refuse column — "write accepted, read refused, every
// datatype x shape"), so a read-based oracle can't distinguish "delete did
// nothing" from "this backend never round-trips reads at all." Its
// deleteData is confirmed a no-op by reading the source directly
// (flexbuffers_backend.cpp: empty NOOP body) — noted in the file-header
// comment above, not re-asserted here since it isn't ABI-observable.
bool delete_is_a_noop(const BackendCase& b, al_contract::TempBase& base,
                      const PulseId& pulse) {
    const std::string uri = al_contract::build_uri(b.id, base.str(), pulse);
    int pctx = -1;
    EXPECT_EQ(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx).code, 0);
    int op = -1;
    EXPECT_EQ(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op).code, 0);
    EXPECT_EQ(al_contract::write_data<double>(op, "leaf_a", {}, {11.0}).code, 0);
    EXPECT_EQ(al_delete_data(op, "leaf_a").code, 0);
    EXPECT_EQ(al_end_action(op).code, 0);
    int rop = -1;
    EXPECT_EQ(al_begin_global_action(pctx, kIds, "", READ_OP, &rop).code, 0);
    std::vector<int> shape;
    std::vector<double> a;
    EXPECT_EQ(al_contract::read_data<double>(rop, "leaf_a", 0, &shape, &a).code, 0);
    EXPECT_EQ(al_end_action(rop).code, 0);
    al_close_pulse(pctx, CLOSE_PULSE);
    return !a.empty() && a[0] == 11.0;  // still there => delete did nothing
}

TEST_F(DeleteKnownDefects, DISABLED_AsciiDeleteRemovesLeaf) {
    EXPECT_FALSE(delete_is_a_noop(kAscii, base_, pulse_))
        << "al_delete_data must actually remove data in the ASCII backend";
}
TEST_F(DeleteKnownDefects, AsciiDeleteIsCurrentlyANoOp) {
    EXPECT_TRUE(delete_is_a_noop(kAscii, base_, pulse_))
        << "AsciiBackend::deleteData now has an effect — enable "
           "DeleteKnownDefects.DISABLED_AsciiDeleteRemovesLeaf "
           "(ascii_backend.cpp deleteData has an empty body)";
}


// ===========================================================================
// al_get_occurrences.
// ===========================================================================
// HDF5 and ASCII implement it for real, each against its own occurrence-naming
// convention (neither backend transforms `dataobjectname`, so the *caller*
// supplies the backend-native form): HDF5 external-links are named literally,
// so occurrence N is "<ids>_<N>" (src/hdf5/hdf5_reader.cpp:1550-1588, split on
// the last '_'); ASCII strips exactly one '/' character from `dataobjectname`
// (src/ascii_backend.cpp:194-197), so occurrence N must be passed as
// "<ids>/<N>" to produce the on-disk stem "<ids><N>" its own directory scan
// expects. Memory and Flexbuffers throw unconditionally (not implemented).
//
// MDSplus (issue #14) implements it for real too, resolved empirically as a
// THIRD convention: occurrence N must be passed as "<ids>/<N>", same slash
// form as ASCII (mdsconvertPath's SEPARATORS, src/mdsplus/mdsplus_backend.cpp:25,
// tokenizes on '/' and turns "equilibrium/2" into the real child node
// "EQUILIBRIUM:2" the baked model already contains -- occurrence subnodes are
// pre-baked structural slots, not dynamically created). get_occurrences
// (src/mdsplus/mdsplus_backend.cpp:4887-4954) walks ids_node's numeric-named
// children and reports an occurrence "filled" iff its
// ids_properties/homogeneous_time has nonzero length, so the write below must
// set homogeneous_time, not just the scalar. list_filled_paths is NOT
// real on MDSplus -- it throws unconditionally
// (src/mdsplus/mdsplus_backend.cpp:4956-4958, "not implemented in the MDSplus
// Backend"), confirmed via CapabilityMatrix below.
TEST(Occurrences, Hdf5ListsWrittenOccurrences) {
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 12, 0};
    base.make_legacy_tree(pulse);
    const std::string uri = al_contract::build_uri(HDF5_BACKEND, base.str(), pulse);
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx));

    for (const char* name : {"magnetics", "magnetics_2"}) {
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pctx, name, "", WRITE_OP, &op));
        AL_EXPECT_OK(al_contract::write_data<double>(op, "leaf", {}, {1.0}));
        AL_ASSERT_OK(al_end_action(op));
    }

    int* occ = nullptr;
    int  n   = -1;
    AL_ASSERT_OK(al_get_occurrences(pctx, "magnetics", &occ, &n));
    ASSERT_EQ(n, 2);
    EXPECT_EQ(occ[0], 0);
    EXPECT_EQ(occ[1], 2);
    free(occ);  // malloc'd (src/hdf5/hdf5_reader.cpp:1592) -> free() is correct.

    al_close_pulse(pctx, CLOSE_PULSE);
}

TEST(Occurrences, AsciiListsWrittenOccurrences) {
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 12, 0};
    base.make_legacy_tree(pulse);
    const std::string uri = al_contract::build_uri(ASCII_BACKEND, base.str(), pulse);
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx));

    for (const char* name : {"magnetics", "magnetics/2"}) {
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pctx, name, "", WRITE_OP, &op));
        AL_EXPECT_OK(al_contract::write_data<double>(op, "leaf", {}, {1.0}));
        AL_ASSERT_OK(al_end_action(op));
    }

    int* occ = nullptr;
    int  n   = -1;
    AL_ASSERT_OK(al_get_occurrences(pctx, "magnetics", &occ, &n));
    ASSERT_EQ(n, 2);
    EXPECT_EQ(occ[0], 0);
    EXPECT_EQ(occ[1], 2);
    free(occ);  // malloc'd (src/ascii_backend.cpp:706) -> free() is correct.

    al_close_pulse(pctx, CLOSE_PULSE);
}

TEST(Occurrences, MemoryRefuses) {
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 12, 0};
    const std::string uri = al_contract::build_uri(MEMORY_BACKEND, base.str(), pulse);
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx));

    int* occ = nullptr;
    int  n   = -1;
    EXPECT_NE(al_get_occurrences(pctx, "magnetics", &occ, &n).code, 0)
        << "get_occurrences is not implemented in MemoryBackend";
    al_close_pulse(pctx, CLOSE_PULSE);
}

TEST(Occurrences, FlexbuffersRefuses) {
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 12, 0};
    base.make_legacy_tree(pulse);
    const std::string uri = al_contract::build_uri(FLEXBUFFERS_BACKEND, base.str(), pulse);
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx));

    int* occ = nullptr;
    int  n   = -1;
    EXPECT_NE(al_get_occurrences(pctx, "magnetics", &occ, &n).code, 0)
        << "get_occurrences is not implemented in the Serialize (Flexbuffers) backend";
    al_close_pulse(pctx, CLOSE_PULSE);
}

#ifdef AL_CONTRACT_HAVE_MDSPLUS
// MDSplus resolves paths against a real DD-baked model tree, so this reuses
// equilibrium_seed's real DD path instead of the synthetic "magnetics"/"leaf"
// the other three Occurrences.* cases use (see the divergence notes on
// EquilibriumSeedMatrix/AosMatrix above for why a synthetic path can't
// transfer). homogeneous_time must be written for get_occurrences to see the
// occurrence as filled at all (see comment above).
TEST(Occurrences, MdsplusListsWrittenOccurrences) {
    const char* models_path = std::getenv("MDSPLUS_MODELS_PATH");
    if (!models_path || !*models_path) {
        GTEST_SKIP() << "MDSPLUS_MODELS_PATH is unset -- MDSplus "
                        "characterization tests are skipped, not failed "
                        "(TEST_STRATEGY.md D4).";
    }
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 12, 0};
    base.make_legacy_tree(pulse);
    const std::string uri = al_contract::build_uri(MDSPLUS_BACKEND, base.str(), pulse);
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx));

    for (const char* name : {equilibrium_seed::kIds, "equilibrium/2"}) {
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pctx, name, "", WRITE_OP, &op));
        AL_EXPECT_OK(al_contract::write_data<double>(op, equilibrium_seed::kScalar, {}, {6.2}));
        int ht = 1;
        AL_EXPECT_OK(al_write_data(op, "ids_properties/homogeneous_time", "", &ht,
                                   INTEGER_DATA, 0, nullptr));
        AL_ASSERT_OK(al_end_action(op));
    }

    int* occ = nullptr;
    int  n   = -1;
    AL_ASSERT_OK(al_get_occurrences(pctx, equilibrium_seed::kIds, &occ, &n));
    ASSERT_EQ(n, 2);
    EXPECT_EQ(occ[0], 0);
    EXPECT_EQ(occ[1], 2);
    free(occ);  // malloc'd (src/mdsplus/mdsplus_backend.cpp:4949) -> free() is correct.

    al_close_pulse(pctx, CLOSE_PULSE);
}
#endif  // AL_CONTRACT_HAVE_MDSPLUS

}  // namespace
