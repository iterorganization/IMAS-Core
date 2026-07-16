// UDA real-DD-path datatype x rank breadth (issue #25, TRACEABILITY.md Part 5,
// acceptance criterion 1/2).
//
// This is the UDA analogue of test_mdsplus_real_paths.cpp (issue #15): the same
// curated real-DD-4.1.1 path set, now the shared real_dd::catalog() (issue
// #46). UDA hits the identical structural wall MDSplus does -- RoundTripMatrix's
// opaque synthetic paths have no node in the schema either backend validates
// against (MDSplus: a baked binary model tree; UDA: the DD XML walked at
// runtime, src/uda/uda_xml.cpp find_node()). See real_dd_path_catalog.h for the
// curation method.
//
// Shape adaptation for UDA (PRD #21 / issue #24): UDA has no remote write, so
// each cell is seed(HDF5)-then-reopen(UDA) instead of MDSplus's direct
// write->read against its own backend --
//   * seed: FORCE_CREATE_PULSE via the plain HDF5 backend, write
//     ids_properties/homogeneous_time=1 (UDA's cache_mode=none read path
//     always resolves it first and throws if absent -- see test_uda.cpp),
//     then the leaf (descending the catalog aos_chain via
//     al_begin_arraystruct_action, exactly like the MDSplus fixture);
//   * reopen: OPEN_PULSE via the UDA backend in remote mode
//     (backend=hdf5&cache_mode=none), descend the same aos_chain, read the
//     leaf back, assert self-consistency against what was written.
//
// The path set is shared with MDSplus; only UDA's own verdicts stay here:
// kDivergences (CHAR r0's fixture-backend limitation) and kKnownDefects (the
// dynamic-leaf-inside-AOS defect pinned by UdaRealPathMatrixKnownDefects).
//
// Oracle: per-backend write->read self-consistency (same as MDSplus's version
// of this matrix) -- only the field *path* is real DD; the *content* stays
// opaque synthetic data (al_contract::synth_buffer).
#ifdef AL_CONTRACT_HAVE_UDA

#include "al_contract.h"
#include "real_dd_path_catalog.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <complex>
#include <filesystem>
#include <set>
#include <string>
#include <utility>
#include <vector>

using al_contract::real_dd::CatalogEntry;
using al_contract::real_dd::DType;

namespace {

// UDA's own verdict for a catalog cell, layered on the shared path.
// `divergence_reason` non-null: a legitimate storage/fixture-model difference,
// skipped (not asserted). `known_defect_reason` non-null: the genuine,
// newly-discovered defect pinned by UdaAosKnownDefects (test_uda_breadth.cpp)
// -- a *dynamic* (time-varying) leaf nested inside a struct_array silently
// comes back empty/absent through UDA remote mode + backend=hdf5. Distinct from
// a divergence: this is a real bug (xfail), skipped in the breadth matrix and
// pinned exactly by UdaRealPathMatrixKnownDefects's DISABLED_/tripwire pair.
struct UdaCell {
    const CatalogEntry* entry;
    const char*         divergence_reason;
    const char*         known_defect_reason;
};

// UDA-specific verdicts keyed by (dtype, rank). Distinct from MDSplus's:
// the two backends diverge for different reasons on the same cell.
struct Verdict {
    DType       dt;
    int         rank;
    const char* divergence_reason;
    const char* known_defect_reason;
};
const Verdict kVerdicts[] = {
    // CHAR r0: seeding needs a dim=0 (bare scalar) CHAR write through the plain
    // HDF5 backend used for fixture setup -- the pre-existing, independently-
    // pinned HDF5 CHAR-scalar crash (RoundTripKnownDefects.DISABLED_
    // Hdf5CharScalarRoundTrips / tripwire …CurrentlyCrashes, TRACEABILITY.md
    // Part 1), confirmed to reproduce on this real DD leaf (equilibrium's
    // code/name, STR_0D) too. A fixture-backend defect, not a UDA behavior, so
    // it is recorded as a divergence (not exercised) rather than run into the
    // crash.
    {DType::Char, 0,
     "seeding this cell needs a dim=0 CHAR write through the plain HDF5 "
     "backend (fixture setup) -- the pre-existing, independently-pinned "
     "HDF5 CHAR-scalar crash (RoundTripKnownDefects.DISABLED_"
     "Hdf5CharScalarRoundTrips / tripwire …CurrentlyCrashes, TRACEABILITY.md "
     "Part 1) reproduces on this real DD leaf too; not run to avoid crashing "
     "the seed step. A fixture-backend defect, not a UDA behavior",
     nullptr},
    // The three dynamic-leaf-inside-AOS known defects (xfail): the field comes
    // back empty/absent through UDA remote mode + backend=hdf5 instead of what
    // was written. Pinned by UdaRealPathMatrixKnownDefects below.
    {DType::Complex, 1, nullptr,
     "dynamic leaf nested in a struct_array (3 AOS levels)"},
    {DType::Complex, 3, nullptr,
     "dynamic leaf nested in a struct_array (1 AOS level)"},
    {DType::Complex, 5, nullptr,
     "dynamic leaf nested in a struct_array (1 AOS level)"},
};

const Verdict* verdict_for(DType dt, int rank) {
    for (const Verdict& v : kVerdicts) {
        if (v.dt == dt && v.rank == rank) return &v;
    }
    return nullptr;
}

// Derive the UDA matrix 1:1 from the shared catalog: one cell per catalog
// entry, in catalog order, with UDA's local verdict attached. The
// CoversEveryCatalogKeyOnce meta-test pins that this stays a bijection.
const std::vector<UdaCell>& uda_cells() {
    static const std::vector<UdaCell> kCells = [] {
        std::vector<UdaCell> cells;
        for (const CatalogEntry& e : al_contract::real_dd::catalog()) {
            const Verdict* v = verdict_for(e.dt, e.rank);
            cells.push_back({&e, v ? v->divergence_reason : nullptr,
                             v ? v->known_defect_reason : nullptr});
        }
        return cells;
    }();
    return kCells;
}

// The subset of cells carrying the dynamic-leaf-inside-AOS known defect, for
// the DISABLED_/tripwire pair below. Derived from the same uda_cells() so the
// matrix and the pins exercise identical IDS paths, ranks, and AOS nesting.
const std::vector<UdaCell>& uda_known_defect_cells() {
    static const std::vector<UdaCell> kCells = [] {
        std::vector<UdaCell> cells;
        for (const UdaCell& c : uda_cells()) {
            if (c.known_defect_reason != nullptr) cells.push_back(c);
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

// One real-path round trip: seed a fresh pulse dir through the plain HDF5
// backend (descending spec.aos_chain, one al_begin_arraystruct_action per
// segment, a single element each, mirroring test_mdsplus_real_paths.cpp's
// run_cell), then reopen the identical pulse dir through the UDA backend in
// remote mode and read the same path back.
template <class T, class ReadExpectation>
void run_cell_with_read_expectation(const CatalogEntry& spec,
                                    ReadExpectation&& expect_read) {
    al_contract::TempBase base;
    const std::string pulse_dir = base.str() + "/pulse";
    std::error_code ec;
    std::filesystem::create_directories(pulse_dir, ec);

    const std::vector<int> shape = al_contract::shape_for_rank(spec.rank);
    const std::vector<T>   written =
        al_contract::synth_buffer<T>(al_contract::element_count(shape));

    // --- seed via the plain HDF5 backend (fixture setup) -------------------
    {
        const std::string hdf5_uri = al_contract::hdf5_uri_for(pulse_dir);
        int pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(hdf5_uri.c_str(),
                                               FORCE_CREATE_PULSE, &pulse_ctx));

        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, spec.ids, "", WRITE_OP, &op));
        // UDA's cache_mode=none read path always resolves
        // ids_properties/homogeneous_time first and throws if absent (see
        // test_uda.cpp's characterization-discovered facts) -- seed it ahead
        // of the field under test so the round trip exercises the leaf, not
        // that precondition.
        AL_EXPECT_OK(al_contract::write_data<int>(
            op, "ids_properties/homogeneous_time", {}, {1}));

        int              ctx = op;
        std::vector<int> aos_ctxs;
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
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    // --- reopen through UDA, remote mode ------------------------------------
    {
        const std::string uda_uri = al_contract::uda_hdf5_uri_for(pulse_dir);
        int pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE,
                                               &pulse_ctx));

        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, spec.ids, "", READ_OP, &op));

        int              ctx = op;
        std::vector<int> aos_ctxs;
        for (const char* name : spec.aos_chain) {
            int size = 0;
            int aos  = -1;
            AL_EXPECT_OK(al_begin_arraystruct_action(ctx, name, "", &size,
                                                     &aos));
            EXPECT_EQ(size, 1) << "single-element AOS size must round-trip";
            aos_ctxs.push_back(aos);
            ctx = aos;
        }

        expect_read(ctx, spec, shape, written);

        for (auto it = aos_ctxs.rbegin(); it != aos_ctxs.rend(); ++it) {
            AL_EXPECT_OK(al_end_action(*it));
        }
        AL_ASSERT_OK(al_end_action(op));
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }
}

template <class T>
void run_cell(const CatalogEntry& spec) {
    run_cell_with_read_expectation<T>(
        spec, [](int ctx, const CatalogEntry& read_spec,
                 const std::vector<int>& shape,
                 const std::vector<T>& written) {
            read_leaf_and_expect<T>(ctx, read_spec.leaf, read_spec.rank, shape,
                                    written);
        });
}

class UdaRealPathMatrix : public ::testing::TestWithParam<UdaCell> {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_UDA_UNCONFIGURED(); }
};

TEST_P(UdaRealPathMatrix, RealPathRoundTrip) {
    const UdaCell       cell = GetParam();
    const CatalogEntry& spec = *cell.entry;
    if (spec.terminal_gap_reason != nullptr) {
        GTEST_SKIP() << "terminal-gap: " << spec.terminal_gap_reason;
    }
    if (cell.divergence_reason != nullptr) {
        GTEST_SKIP() << "divergence: " << cell.divergence_reason;
    }
    if (cell.known_defect_reason != nullptr) {
        GTEST_SKIP() << "known-defect (xfail): " << cell.known_defect_reason;
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
// stay stable across the #46 extraction (e.g. "COMPLEX_r1").
std::string NameUdaRealPathMatrixCase(
    const ::testing::TestParamInfo<UdaCell>& info) {
    return al_contract::real_dd::case_name(*info.param.entry);
}

INSTANTIATE_TEST_SUITE_P(Uda, UdaRealPathMatrix,
                         ::testing::ValuesIn(uda_cells()),
                         NameUdaRealPathMatrixCase);

// Meta-test (issue #46): the UDA matrix maps the shared catalog 1:1 -- one cell
// per catalog key, no duplicate, none dropped. A future refactor that filters
// cells out of uda_cells() turns this red instead of silently shrinking the
// matrix.
TEST(UdaRealPathCatalog, CoversEveryCatalogKeyOnce) {
    using Key = std::pair<int, int>;
    std::set<Key> matrix_keys;
    for (const UdaCell& cell : uda_cells()) {
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
        << "UDA matrix and catalog key sets diverge";
}

// Exact D2 pins for the COMPLEX matrix cells affected by the dynamic-leaf-
// inside-AOS defect. The disabled correct-contract test and enabled
// current-behavior tripwire deliberately share run_cell_with_read_expectation,
// so both traverse the same public C ABI and the same real DD path. A targeted
// fix to any one rank/path makes that case's tripwire fail independently.
class UdaRealPathMatrixKnownDefects
    : public ::testing::TestWithParam<UdaCell> {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_UDA_UNCONFIGURED(); }
};

TEST_P(UdaRealPathMatrixKnownDefects,
       DISABLED_DynamicComplexLeafInsideAosRoundTrips) {
    run_cell<std::complex<double>>(*GetParam().entry);
}

TEST_P(UdaRealPathMatrixKnownDefects,
       DynamicComplexLeafInsideAosCurrentlyReadsEmpty) {
    run_cell_with_read_expectation<std::complex<double>>(
        *GetParam().entry, [](int ctx, const CatalogEntry& spec,
                              const std::vector<int>&,
                              const std::vector<std::complex<double>>&) {
            std::vector<int>                  read_shape;
            std::vector<std::complex<double>> read_data;
            AL_EXPECT_OK(al_contract::read_data<std::complex<double>>(
                ctx, spec.leaf, spec.rank, &read_shape, &read_data));
            EXPECT_TRUE(read_shape.empty() && read_data.empty())
                << "this exact COMPLEX r" << spec.rank
                << " dynamic leaf now returns data through UDA remote mode -- "
                   "enable its paired DISABLED_DynamicComplexLeafInsideAosRoundTrips "
                   "case";
        });
}

INSTANTIATE_TEST_SUITE_P(Uda, UdaRealPathMatrixKnownDefects,
                         ::testing::ValuesIn(uda_known_defect_cells()),
                         NameUdaRealPathMatrixCase);

}  // namespace

#endif  // AL_CONTRACT_HAVE_UDA
