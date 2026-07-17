// MDSplus real-DD-path delete semantics (issue #34, TRACEABILITY.md Part 4
// DeleteMatrix row).
//
// The always-on DeleteMatrix (test_structured_data.cpp) exercises
// al_delete_data at leaf / structure / DATAOBJECT-root granularity against the
// synthetic fixture paths (leaf_a, block, "") on HDF5/Memory/ASCII/Flexbuffers.
// MDSplus is absent from kDeleteBackends because those synthetic paths have no
// node in its DD-baked model tree -- the same wall RoundTripMatrix/AosMatrix
// hit (issue #12 Q2/Q5). TRACEABILITY.md long marked the whole MDSplus
// DeleteMatrix row a `terminal-gap`, even though MDSplusBackend::deleteData
// (src/mdsplus/mdsplus_backend.cpp:2136) genuinely implements path-scoped
// deletion. This file closes that gap the same way MdsplusRealPathMatrix
// (issue #15) closed the RoundTrip gap: with real DD-4.1.1 paths MDSplus can
// resolve, and one verdict per granularity.
//
// Real DD-4.1.1 paths (validated against the baked-from artifact
// build-mdsplus/_deps/data-dictionary-src/IDSDef.xml, the same method
// real_dd_path_catalog.h documents -- the imas-dd MCP tool CLAUDE.md names is
// not in this session's MCP config):
//   * equilibrium/code is a plain structure holding several STR_0D leaves
//     (name, version, description, ...) -- the leaf and structure cases.
//   * equilibrium/vacuum_toroidal_field/r0 (FLT_0D) is an unrelated sibling
//     leaf that must survive a code-scoped or leaf-scoped delete.
//
// Observed verdicts (characterized empirically through the public C ABI):
//   * LEAF  (al_delete_data("code/name")) -> COVERED. Succeeds; the target's
//     data is cleared (reads back empty, no longer its seeded value) while
//     every sibling keeps its value.
//   * STRUCTURE (al_delete_data("code")) -> XFAIL. deleteData's STRUCTURE
//     branch (mdsplus_backend.cpp:2147) assumes array-of-structures storage --
//     it looks up the `:static`/`.timed_aos` AOS-layout children, which a
//     plain DD structure has no node for, so the call throws
//     `%TREE-W-NNF, Node Not Found` and nothing is deleted. Pinned as a
//     DISABLED_ correct-contract test + an active current-behavior tripwire
//     (decision D2).
//   * ROOT  (al_delete_data("")) -> XFAIL, same %TREE-W-NNF cause (the
//     DATAOBJECT root is itself a STRUCTURE-usage node). Explicit observed
//     verdict via the same DISABLED_/tripwire pair.
//
// Oracle: seed distinguishable values through the public C ABI, al_delete_data
// at a granularity, reopen the pulse, read every path back. Only the *paths*
// are real DD; the deletion contract itself is backend behavior (decision D1:
// the core attaches no DD semantics to any path).
#ifdef AL_CONTRACT_HAVE_MDSPLUS

#include "al_contract.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using al_contract::PulseId;

namespace {

constexpr const char* kIds = "equilibrium";

// A real DD-4.1.1 STR_0D leaf under equilibrium/code and the value seeded into
// it. All static (non-dynamic), so one GLOBAL write seeds them.
struct SeedLeaf {
    const char* path;
    const char* value;
};
constexpr SeedLeaf kCodeName{"code/name", "alpha-name"};
constexpr SeedLeaf kCodeVersion{"code/version", "beta-version"};
constexpr SeedLeaf kCodeDescription{"code/description", "gamma-description"};

// An unrelated sibling leaf outside `code`: FLT_0D, must survive a code-scoped
// (structure) or code/name-scoped (leaf) delete.
constexpr const char* kSiblingPath  = "vacuum_toroidal_field/r0";
constexpr double       kSiblingValue = 6.2;

PulseId pulse_id() {
    return PulseId{/*database=*/"test", /*version=*/"3", /*pulse=*/15,
                   /*run=*/0};
}

void write_str(int op, const char* path, const std::string& value) {
    std::vector<char> buf(value.begin(), value.end());
    AL_EXPECT_OK(al_contract::write_data<char>(
        op, path, {static_cast<int>(buf.size())}, buf));
}

// Read a STR_0D/1D leaf. Returns true with `out` set on a clean read; false if
// the read reported an error. After a delete clears a node, the read succeeds
// but returns empty -- so callers distinguish "gone" via emptiness, not error.
bool try_read_str(int op, const char* path, std::string* out) {
    std::vector<int>  shape;
    std::vector<char> data;
    al_status_t       s = al_contract::read_data<char>(op, path, /*rank=*/1,
                                                       &shape, &data);
    if (s.code != 0) return false;
    out->assign(data.begin(), data.end());
    return true;
}

bool try_read_double(int op, const char* path, double* out) {
    std::vector<int>    shape;
    std::vector<double> data;
    al_status_t s = al_contract::read_data<double>(op, path, /*rank=*/0, &shape,
                                                   &data);
    if (s.code != 0 || data.empty()) return false;
    *out = data.front();
    return true;
}

// True iff a STR leaf reads back its exact seeded value.
bool str_survives(int op, const SeedLeaf& leaf) {
    std::string v;
    return try_read_str(op, leaf.path, &v) && v == leaf.value;
}

// True iff a STR leaf's data is gone (read fails, or reads back empty rather
// than its seeded value).
bool str_gone(int op, const SeedLeaf& leaf) {
    std::string v;
    if (!try_read_str(op, leaf.path, &v)) return true;
    return v != leaf.value;
}

bool sibling_survives(int op) {
    double d = 0;
    return try_read_double(op, kSiblingPath, &d) && d == kSiblingValue;
}

class MdsplusRealPathDelete : public ::testing::Test {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_MDSPLUS_UNCONFIGURED(); }

    // Create a fresh MDSplus pulse and seed the three code/* leaves plus the
    // unrelated vacuum_toroidal_field/r0 sibling. Leaves `base` owning the tree
    // dir so the caller can reopen the same pulse.
    void seed(al_contract::TempBase& base) {
        base.make_legacy_tree(pulse_id());
        const std::string uri =
            al_contract::build_uri(MDSPLUS_BACKEND, base.str(), pulse_id());
        ASSERT_FALSE(uri.empty());
        int pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE,
                                               &pulse_ctx));
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, kIds, "", WRITE_OP, &op));
        write_str(op, kCodeName.path, kCodeName.value);
        write_str(op, kCodeVersion.path, kCodeVersion.value);
        write_str(op, kCodeDescription.path, kCodeDescription.value);
        AL_EXPECT_OK(al_contract::write_data<double>(op, kSiblingPath, {},
                                                     {kSiblingValue}));
        AL_ASSERT_OK(al_end_action(op));
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    // Delete `path` in a fresh WRITE action on the (reopened) pulse; returns the
    // al_delete_data status. Opens/closes its own pulse ctx.
    al_status_t delete_path(const al_contract::TempBase& base,
                            const char* path) {
        const std::string uri =
            al_contract::build_uri(MDSPLUS_BACKEND, base.str(), pulse_id());
        int pulse_ctx = -1;
        EXPECT_EQ(
            al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx).code,
            0);
        int op = -1;
        EXPECT_EQ(al_begin_global_action(pulse_ctx, kIds, "", WRITE_OP, &op).code,
                  0);
        al_status_t s = al_delete_data(op, path);
        al_end_action(op);
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
        return s;
    }

    // Reopen the pulse for READ and hand the op ctx to `check`; tidies up after.
    template <class Fn>
    void reopen_and_read(const al_contract::TempBase& base, Fn&& check) {
        const std::string uri =
            al_contract::build_uri(MDSPLUS_BACKEND, base.str(), pulse_id());
        int pulse_ctx = -1;
        AL_ASSERT_OK(
            al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, kIds, "", READ_OP, &op));
        check(op);
        al_end_action(op);
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }
};

// --- LEAF: covered ----------------------------------------------------------
// Deleting a single STR_0D leaf clears exactly that leaf; every sibling (two
// other code/* leaves and the unrelated vacuum_toroidal_field/r0) survives.
TEST_F(MdsplusRealPathDelete, LeafDeleteRemovesJustTheLeaf) {
    al_contract::TempBase base;
    seed(base);

    AL_EXPECT_OK(delete_path(base, kCodeName.path));

    reopen_and_read(base, [](int op) {
        EXPECT_TRUE(str_gone(op, kCodeName))
            << "deleted leaf code/name still reads back its seeded value";
        EXPECT_TRUE(str_survives(op, kCodeVersion))
            << "sibling code/version must survive a code/name delete";
        EXPECT_TRUE(str_survives(op, kCodeDescription))
            << "sibling code/description must survive a code/name delete";
        EXPECT_TRUE(sibling_survives(op))
            << "unrelated vacuum_toroidal_field/r0 must survive";
    });
}

// --- STRUCTURE: xfail -------------------------------------------------------
// Correct contract (disabled): deleting the `code` structure removes its whole
// subtree (all code/* leaves) and nothing else. Enable when deleteData learns
// to delete a plain (non-AOS) structure.
TEST_F(MdsplusRealPathDelete, DISABLED_StructureDeleteRemovesWholeSubtree) {
    al_contract::TempBase base;
    seed(base);

    AL_EXPECT_OK(delete_path(base, "code"));

    reopen_and_read(base, [](int op) {
        EXPECT_TRUE(str_gone(op, kCodeName));
        EXPECT_TRUE(str_gone(op, kCodeVersion));
        EXPECT_TRUE(str_gone(op, kCodeDescription));
        EXPECT_TRUE(sibling_survives(op))
            << "a code-scoped delete must not touch vacuum_toroidal_field/r0";
    });
}

// Current behavior (active tripwire): al_delete_data("code") throws
// %TREE-W-NNF because deleteData's STRUCTURE branch looks up the AOS-only
// :static/.timed_aos children, which a plain structure lacks. The delete is a
// no-op: every seeded leaf survives. When deleteData is fixed, this tripwire
// fails and its paired DISABLED_ test above should be enabled.
TEST_F(MdsplusRealPathDelete, StructureDeleteCurrentlyThrowsNodeNotFound) {
    al_contract::TempBase base;
    seed(base);

    al_status_t s = delete_path(base, "code");
    EXPECT_NE(s.code, 0)
        << "al_delete_data(\"code\") now succeeds -- enable "
           "DISABLED_StructureDeleteRemovesWholeSubtree";
    EXPECT_NE(std::string(s.message).find("Node Not Found"), std::string::npos)
        << "expected %TREE-W-NNF, got: " << s.message;

    reopen_and_read(base, [](int op) {
        EXPECT_TRUE(str_survives(op, kCodeName))
            << "failed structure delete must leave code/name intact";
        EXPECT_TRUE(str_survives(op, kCodeVersion));
        EXPECT_TRUE(str_survives(op, kCodeDescription));
        EXPECT_TRUE(sibling_survives(op));
    });
}

// --- ROOT: xfail ------------------------------------------------------------
// Correct contract (disabled): deleting the DATAOBJECT root ("") removes the
// whole occurrence -- every path is gone. Enable when deleteData supports root
// granularity for MDSplus.
TEST_F(MdsplusRealPathDelete, DISABLED_RootDeleteRemovesWholeOccurrence) {
    al_contract::TempBase base;
    seed(base);

    AL_EXPECT_OK(delete_path(base, ""));

    reopen_and_read(base, [](int op) {
        EXPECT_TRUE(str_gone(op, kCodeName));
        EXPECT_TRUE(str_gone(op, kCodeVersion));
        EXPECT_TRUE(str_gone(op, kCodeDescription));
        double d = 0;
        EXPECT_FALSE(try_read_double(op, kSiblingPath, &d))
            << "root delete must remove vacuum_toroidal_field/r0 too";
    });
}

// Current behavior (active tripwire): al_delete_data("") throws %TREE-W-NNF
// (the DATAOBJECT root is a STRUCTURE-usage node, same AOS-layout assumption as
// the structure case). Explicit observed verdict; the occurrence is untouched.
TEST_F(MdsplusRealPathDelete, RootDeleteCurrentlyThrowsNodeNotFound) {
    al_contract::TempBase base;
    seed(base);

    al_status_t s = delete_path(base, "");
    EXPECT_NE(s.code, 0)
        << "al_delete_data(\"\") now succeeds -- enable "
           "DISABLED_RootDeleteRemovesWholeOccurrence";
    EXPECT_NE(std::string(s.message).find("Node Not Found"), std::string::npos)
        << "expected %TREE-W-NNF, got: " << s.message;

    reopen_and_read(base, [](int op) {
        EXPECT_TRUE(str_survives(op, kCodeName))
            << "failed root delete must leave the occurrence intact";
        EXPECT_TRUE(sibling_survives(op));
    });
}

}  // namespace

#endif  // AL_CONTRACT_HAVE_MDSPLUS
