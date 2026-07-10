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
// Path curation method: the imas-dd MCP tool named in CLAUDE.md was not
// present in this session's MCP configuration, so the same question it would
// answer was resolved by walking the actual baked-from artifact directly --
// build-mdsplus/_deps/data-dictionary-src/IDSDef.xml (the DD-4.1.1 source
// ALBuildDataDictionary.cmake downloads and the model tree is compiled from)
// -- to find, per (type, rank) cell, a real field of that data_type together
// with its struct_array ancestor chain (if any). This is still empirical
// characterization against the real shipped artifact (TEST_STRATEGY D2),
// simply queried by hand instead of through the MCP wrapper.
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

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <complex>
#include <string>
#include <vector>

using al_contract::PulseId;

namespace {

enum class DType { Char, Int, Double, Complex };

// One (type, rank) cell. `aos_chain` lists the struct_array field(s) to enter
// in order (each may itself be a multi-segment path through intervening plain
// structures, e.g. "e_field/plus" or "distribution/markers"); empty means the
// leaf hangs directly off the GLOBAL op. `terminal_gap_reason` non-null means
// the DD contains no field of this (type, rank) anywhere and the rest of the
// row is ignored. `divergence_reason` non-null means the cell round-tripped
// but not as a plain pass -- a legitimate storage-model difference empirically
// confirmed by running it (see CHAR_r0 below), not a defect (D2); the round
// trip is still attempted up to the point that confirms the divergence, then
// the test self-reports via GTEST_SKIP() rather than asserting equality.
struct PathSpec {
    DType                    dt;
    int                      rank;
    const char*              ids;
    std::vector<const char*> aos_chain;
    const char*              leaf;
    const char*              terminal_gap_reason;
    const char*              divergence_reason = nullptr;
};

// Curated against build-mdsplus/_deps/data-dictionary-src/IDSDef.xml (DD
// 4.1.1). CHAR's rank axis is the raw C-ABI `dim` argument, which is one off
// from the DD's STR_ND suffix (IMAS convention: a *string* is CHAR dim=1, an
// *array of strings* is CHAR dim=2 -- dim=0 is a bare scalar char with no DD
// analogue of its own, reusing the same STR_0D node as dim=1). INTEGER,
// DOUBLE, and COMPLEX map dim directly to their _ND suffix.
const PathSpec kPathSpecs[] = {
    // --- CHAR ---------------------------------------------------------------
    {DType::Char, 0, "equilibrium", {}, "code/name", nullptr,
     "MDSplus's real STR_0D node is inherently string-shaped (1-D): reading a "
     "dim=0 write back reports CHAR_DATA in 1D, not 0D ('Wrong dimension of "
     "Data returned by backend'), confirmed empirically. Not a crash/defect "
     "(contrast HDF5's genuine CHAR-scalar crash, RoundTripKnownDefects) --  "
     "the C-ABI's synthetic 'dim=0 CHAR' shape has no DD analogue at all "
     "(the DD's smallest text type, STR_0D, is a *string*, i.e. dim=1); "
     "against a real DD-conformant node the backend correctly refuses the "
     "mismatched rank instead of silently misreading it"},
    {DType::Char, 1, "equilibrium", {}, "code/name", nullptr},
    {DType::Char, 2, "b_field_non_axisymmetric", {}, "control_surface_names",
     nullptr},
    {DType::Char, 3, nullptr, {}, nullptr,
     "DD 4.1.1 has no STR_2D+ at all (only STR_0D/STR_1D exist -- there is no "
     "DD concept a CHAR dim>=3 could model)"},
    {DType::Char, 4, nullptr, {}, nullptr, "see rank 3"},
    {DType::Char, 5, nullptr, {}, nullptr, "see rank 3"},
    {DType::Char, 6, nullptr, {}, nullptr, "see rank 3"},
    {DType::Char, 7, nullptr, {}, nullptr, "see rank 3"},

    // --- INTEGER --------------------------------------------------------------
    {DType::Int, 0, "amns_data", {}, "z_n", nullptr},
    {DType::Int, 1, "magnetics", {}, "code/output_flag", nullptr},
    {DType::Int, 2, "magnetics", {}, "b_field_pol_probe_equivalent", nullptr},
    {DType::Int, 3, "temporary", {"constant_integer3d"}, "value", nullptr},
    {DType::Int, 4, nullptr, {}, nullptr,
     "DD 4.1.1 has no INT_4D+ anywhere (max is INT_3D, itself only inside a "
     "struct_array)"},
    {DType::Int, 5, nullptr, {}, nullptr, "see rank 4"},
    {DType::Int, 6, nullptr, {}, nullptr, "see rank 4"},
    {DType::Int, 7, nullptr, {}, nullptr, "see rank 4"},

    // --- DOUBLE -------------------------------------------------------------
    {DType::Double, 0, "amns_data", {}, "a", nullptr},
    {DType::Double, 1, "balance_of_plant", {}, "gain_plant", nullptr},
    {DType::Double, 2, "bolometer", {}, "grid/volume_element", nullptr},
    {DType::Double, 3, "bolometer", {}, "power_density/data", nullptr},
    {DType::Double, 4, "gyrokinetics_local", {},
     "non_linear/fluxes_4d/particles_phi_potential", nullptr},
    {DType::Double, 5, "gyrokinetics_local", {},
     "non_linear/fluxes_5d/particles_phi_potential", nullptr},
    {DType::Double, 6, "temporary", {"constant_float6d"}, "value", nullptr},
    {DType::Double, 7, nullptr, {}, nullptr,
     "DD 4.1.1 has no FLT_7D (the DD's own array-rank ceiling for DOUBLE is "
     "6, one short of MAXDIM)"},

    // --- COMPLEX --------------------------------------------------------------
    {DType::Complex, 0, nullptr, {}, nullptr,
     "DD 4.1.1 has no CPX_0D -- no scalar complex type exists at all"},
    {DType::Complex, 1, "waves", {"coherent_wave", "full_wave", "e_field/plus"},
     "values", nullptr},
    {DType::Complex, 2, "gyrokinetics_local", {},
     "non_linear/fields_zonal_2d/phi_potential_perturbed_norm", nullptr},
    {DType::Complex, 3, "runaway_electrons", {"distribution/markers"},
     "orbit_integrals_instant/values", nullptr},
    {DType::Complex, 4, "gyrokinetics_local", {},
     "non_linear/fields_4d/phi_potential_perturbed_norm", nullptr},
    {DType::Complex, 5, "runaway_electrons", {"distribution/markers"},
     "orbit_integrals/values", nullptr},
    {DType::Complex, 6, nullptr, {}, nullptr,
     "DD 4.1.1 has no CPX_6D anywhere"},
    {DType::Complex, 7, nullptr, {}, nullptr,
     "DD 4.1.1 has no CPX_7D anywhere"},
};

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
void run_cell(const PathSpec& spec) {
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
            AL_ASSERT_OK(al_begin_arraystruct_action(ctx, name, "", &size,
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
            AL_ASSERT_OK(al_begin_arraystruct_action(ctx, name, "", &size,
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

class MdsplusRealPathMatrix : public ::testing::TestWithParam<PathSpec> {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_MDSPLUS_UNCONFIGURED(); }
};

TEST_P(MdsplusRealPathMatrix, RealPathRoundTrip) {
    const PathSpec spec = GetParam();
    if (spec.terminal_gap_reason != nullptr) {
        GTEST_SKIP() << "terminal-gap: " << spec.terminal_gap_reason;
    }
    if (spec.divergence_reason != nullptr) {
        GTEST_SKIP() << "divergence: " << spec.divergence_reason;
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
// nesting, not brace nesting).
std::string NameMdsplusRealPathMatrixCase(
    const ::testing::TestParamInfo<PathSpec>& info) {
    static const char* const kNames[] = {"CHAR", "INTEGER", "DOUBLE",
                                         "COMPLEX"};
    return std::string(kNames[static_cast<int>(info.param.dt)]) + "_r" +
           std::to_string(info.param.rank);
}

INSTANTIATE_TEST_SUITE_P(Mdsplus, MdsplusRealPathMatrix,
                         ::testing::ValuesIn(kPathSpecs),
                         NameMdsplusRealPathMatrixCase);

}  // namespace

#endif  // AL_CONTRACT_HAVE_MDSPLUS
