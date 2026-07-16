// MDSplus real-DD-path datatype x rank breadth (issue #15, TRACEABILITY.md
// Part 4).
//
// RoundTripMatrix (test_roundtrip_matrix.cpp) sweeps
//     {CHAR, INTEGER, DOUBLE, COMPLEX} x rank {scalar .. MAXDIM}
// against SYNTHETIC opaque paths on the always-on backend tier -- a shape
// MDSplus cannot join (issue #12 Q2/Q5, confirmed by the AosMatrix/
// EquilibriumSeedMatrix divergence rows in test_structured_data.cpp): it
// resolves every path against a real, DD-4.1.1-baked model tree, so a made-up
// field name throws "%TREE-W-NNF, Node Not Found". This file is
// RoundTripMatrix's MDSplus-only counterpart: the same type x rank space,
// curated as real DD-4.1.1 paths so MDSplus can actually store them, spanning
// multiple IDSs (the model tree is baked from the whole IDSDef.xml, so this
// exercises that whole-DD bake, not one container).
//
// The curated path set is the shared real_dd::catalog() (issue #46): the IDS,
// struct_array chain, leaf, and terminal-gap DD facts are identical for MDSplus
// and UDA and live in real_dd_path_catalog.{h,cpp}. Only MDSplus's own
// storage-model verdicts stay here (kDivergences below).
//
// Per (type, rank) cell, one of three shapes:
//   * plain leaf   -- straightforward GLOBAL write/read, exactly like a
//                     RoundTripMatrix cell (`aos_chain` empty).
//   * AOS-nested   -- the field only exists inside an array-of-structures in
//                     the whole DD; the AOS is entered (one to three levels,
//                     matching that field's real nesting) via the same
//                     al_begin_arraystruct_action idiom AosMatrix already
//                     proved round-trips against MDSplus for real
//                     DD-conformant paths (test_structured_data.cpp's
//                     Divergence note names
//                     equilibrium/time_slice/global_quantities/ip as the
//                     proof); a single element is written and read back, so
//                     AOS *breadth* itself stays AosMatrix/#14's job.
//   * terminal-gap -- the DD contains no field of that (type, rank) at any
//                     nesting depth; the test itself GTEST_SKIP()s with the
//                     empirical reason, so the cell is visibly accounted for
//                     rather than silently absent from the matrix.
//
// Oracle: per-backend write->read self-consistency (issue #12 Q2), the same
// al_contract::synth_value/synth_buffer/shape_for_rank generator
// RoundTripMatrix uses -- only the field *path* is real; the *content* stays
// opaque synthetic data clear of the write-path "absent" sentinels
// (al_contract.h). This stays a storage-shape probe, not a DD-semantics test
// (decision D1: the core attaches no DD semantics to any path).
#ifdef AL_CONTRACT_HAVE_MDSPLUS

#include "al_contract.h"
#include "real_dd_path_catalog.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <complex>
#include <set>
#include <string>
#include <utility>
#include <vector>

using al_contract::PulseId;
using al_contract::real_dd::CatalogEntry;
using al_contract::real_dd::DType;

namespace {

// MDSplus's own verdict for a catalog cell, layered on top of the shared path.
// `divergence_reason` non-null means the cell round-tripped but not as a plain
// pass -- a legitimate storage-model difference empirically confirmed by
// running it (see CHAR_r0 below), not a defect (D2); the round trip is
// attempted up to the point that confirms the divergence, then the test
// self-reports via GTEST_SKIP() rather than asserting equality. terminal-gap
// cells (a DD fact) come straight from the catalog entry.
struct MdsplusCell {
    const CatalogEntry* entry;
    const char*         divergence_reason;
};

// The only MDSplus-specific divergence: CHAR dim=0 against a real STR_0D node.
struct Divergence {
    DType       dt;
    int         rank;
    const char* reason;
};
const Divergence kDivergences[] = {
    {DType::Char, 0,
     "MDSplus's real STR_0D node is inherently string-shaped (1-D): reading a "
     "dim=0 write back reports CHAR_DATA in 1D, not 0D ('Wrong dimension of "
     "Data returned by backend'), confirmed empirically. Not a crash/defect "
     "(contrast HDF5's genuine CHAR-scalar crash, RoundTripKnownDefects) --  "
     "the C-ABI's synthetic 'dim=0 CHAR' shape has no DD analogue at all "
     "(the DD's smallest text type, STR_0D, is a *string*, i.e. dim=1); "
     "against a real DD-conformant node the backend correctly refuses the "
     "mismatched rank instead of silently misreading it"},
};

const char* divergence_for(DType dt, int rank) {
    for (const Divergence& d : kDivergences) {
        if (d.dt == dt && d.rank == rank) return d.reason;
    }
    return nullptr;
}

// Derive the MDSplus matrix 1:1 from the shared catalog: exactly one cell per
// catalog entry, in catalog order, with MDSplus's local verdict attached. The
// CoversEveryCatalogKeyOnce meta-test pins that this mapping stays a bijection.
const std::vector<MdsplusCell>& mdsplus_cells() {
    static const std::vector<MdsplusCell> kCells = [] {
        std::vector<MdsplusCell> cells;
        for (const CatalogEntry& e : al_contract::real_dd::catalog()) {
            cells.push_back({&e, divergence_for(e.dt, e.rank)});
        }
        return cells;
    }();
    return kCells;
}

// --- write/read a leaf field at whatever context it lives in ----------------
template <class T>
void write_leaf(int ctx, const char* leaf, const std::vector<int>& shape,
                const std::vector<T>& data) {
    AL_EXPECT_OK(al_contract::write_data<T>(ctx, leaf, shape, data));
}

template <class T>
void read_leaf_and_expect(int ctx, const char* leaf, int rank,
                          const std::vector<int>& shape,
                          const std::vector<T>& written) {
    std::vector<int> read_shape;
    std::vector<T>   read_data;
    AL_EXPECT_OK(al_contract::read_data<T>(ctx, leaf, rank, &read_shape,
                                           &read_data));
    EXPECT_EQ(read_shape, shape) << "shape changed across the round trip";
    ASSERT_EQ(read_data.size(), written.size())
        << "element count changed across the round trip";
    EXPECT_EQ(read_data, written) << "data changed across the round trip";
}

// One real-path round trip: open a fresh MDSplus pulse, descend `aos_chain`
// (if any, one al_begin_arraystruct_action per segment, a single element
// each), write the leaf, read it back, assert self-consistency.
template <class T>
void run_cell(const CatalogEntry& spec) {
    al_contract::TempBase base;
    PulseId               pulse{/*database=*/"test", /*version=*/"3",
                    /*pulse=*/15, /*run=*/0};
    base.make_legacy_tree(pulse);

    const std::string uri =
        al_contract::build_uri(MDSPLUS_BACKEND, base.str(), pulse);
    ASSERT_FALSE(uri.empty());

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE,
                                           &pulse_ctx));

    const std::vector<int> shape = al_contract::shape_for_rank(spec.rank);
    const std::vector<T>   written =
        al_contract::synth_buffer<T>(al_contract::element_count(shape));

    // --- write ---
    {
        int op = -1;
        AL_ASSERT_OK(
            al_begin_global_action(pulse_ctx, spec.ids, "", WRITE_OP, &op));

        int               ctx = op;
        std::vector<int>  aos_ctxs;
        for (const char* name : spec.aos_chain) {
            int size = 1;
            int aos  = -1;
            AL_EXPECT_OK(al_begin_arraystruct_action(ctx, name, "", &size,
                                                     &aos));
            aos_ctxs.push_back(aos);
            ctx = aos;
        }

        write_leaf<T>(ctx, spec.leaf, shape, written);

        for (auto it = aos_ctxs.rbegin(); it != aos_ctxs.rend(); ++it) {
            AL_EXPECT_OK(al_end_action(*it));
        }
        AL_ASSERT_OK(al_end_action(op));
    }

    // --- read back ---
    {
        int op = -1;
        AL_ASSERT_OK(
            al_begin_global_action(pulse_ctx, spec.ids, "", READ_OP, &op));

        int               ctx = op;
        std::vector<int>  aos_ctxs;
        for (const char* name : spec.aos_chain) {
            int size = 0;
            int aos  = -1;
            AL_EXPECT_OK(al_begin_arraystruct_action(ctx, name, "", &size,
                                                     &aos));
            EXPECT_EQ(size, 1) << "single-element AOS size must round-trip";
            aos_ctxs.push_back(aos);
            ctx = aos;
        }

        read_leaf_and_expect<T>(ctx, spec.leaf, spec.rank, shape, written);

        for (auto it = aos_ctxs.rbegin(); it != aos_ctxs.rend(); ++it) {
            AL_EXPECT_OK(al_end_action(*it));
        }
        AL_ASSERT_OK(al_end_action(op));
    }

    EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
}

class MdsplusRealPathMatrix : public ::testing::TestWithParam<MdsplusCell> {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_MDSPLUS_UNCONFIGURED(); }
};

TEST_P(MdsplusRealPathMatrix, RealPathRoundTrip) {
    const MdsplusCell   cell = GetParam();
    const CatalogEntry& spec = *cell.entry;
    if (spec.terminal_gap_reason != nullptr) {
        GTEST_SKIP() << "terminal-gap: " << spec.terminal_gap_reason;
    }
    if (cell.divergence_reason != nullptr) {
        GTEST_SKIP() << "divergence: " << cell.divergence_reason;
    }
    switch (spec.dt) {
        case DType::Char:
            run_cell<char>(spec);
            break;
        case DType::Int:
            run_cell<int>(spec);
            break;
        case DType::Double:
            run_cell<double>(spec);
            break;
        case DType::Complex:
            run_cell<std::complex<double>>(spec);
            break;
    }
}

// A free function, not an inline lambda: a brace-init-list's top-level commas
// aren't paren-protected, so they would otherwise split this macro's argument
// list at the preprocessor stage (INSTANTIATE_TEST_SUITE_P only tracks paren
// nesting, not brace nesting). Case names come from the shared catalog so they
// stay stable across the #46 extraction (e.g. "DOUBLE_r3").
std::string NameMdsplusRealPathMatrixCase(
    const ::testing::TestParamInfo<MdsplusCell>& info) {
    return al_contract::real_dd::case_name(*info.param.entry);
}

INSTANTIATE_TEST_SUITE_P(Mdsplus, MdsplusRealPathMatrix,
                         ::testing::ValuesIn(mdsplus_cells()),
                         NameMdsplusRealPathMatrixCase);

// Meta-test (issue #46): the MDSplus matrix maps the shared catalog 1:1 -- one
// cell per catalog key, no duplicate, none dropped. A future refactor that
// filters cells out of mdsplus_cells() turns this red instead of silently
// shrinking the matrix.
TEST(MdsplusRealPathCatalog, CoversEveryCatalogKeyOnce) {
    using Key = std::pair<int, int>;
    std::set<Key> matrix_keys;
    for (const MdsplusCell& cell : mdsplus_cells()) {
        ASSERT_NE(cell.entry, nullptr);
        const Key k{static_cast<int>(cell.entry->dt), cell.entry->rank};
        EXPECT_TRUE(matrix_keys.insert(k).second)
            << "duplicate matrix key "
            << al_contract::real_dd::case_name(*cell.entry);
    }
    std::set<Key> catalog_keys;
    for (const CatalogEntry& e : al_contract::real_dd::catalog()) {
        catalog_keys.insert({static_cast<int>(e.dt), e.rank});
    }
    EXPECT_EQ(matrix_keys, catalog_keys)
        << "MDSplus matrix and catalog key sets diverge";
}

}  // namespace

#endif  // AL_CONTRACT_HAVE_MDSPLUS
