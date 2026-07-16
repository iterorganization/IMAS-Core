// UDA real-DD-path datatype x rank breadth (issue #25, TRACEABILITY.md Part 5,
// acceptance criterion 1/2).
//
// This is the UDA analogue of test_mdsplus_real_paths.cpp (issue #15): the
// same curated real-DD-4.1.1 path set, reused as-is (identical kPathSpecs
// paths/ranks/aos_chains), because UDA hits the identical structural wall
// MDSplus does -- RoundTripMatrix's opaque synthetic paths have no node in
// the schema either backend validates against (MDSplus: a baked binary model
// tree; UDA: the DD XML walked at runtime, src/uda/uda_xml.cpp find_node()).
// See test_mdsplus_real_paths.cpp's header for the curation method.
//
// Shape adaptation for UDA (PRD #21 / issue #24): UDA has no remote write, so
// each cell is seed(HDF5)-then-reopen(UDA) instead of MDSplus's direct
// write->read against its own backend --
//   * seed: FORCE_CREATE_PULSE via the plain HDF5 backend, write
//     ids_properties/homogeneous_time=1 (UDA's cache_mode=none read path
//     always resolves it first and throws if absent -- see test_uda.cpp),
//     then the leaf (descending spec.aos_chain via
//     al_begin_arraystruct_action, exactly like the MDSplus fixture);
//   * reopen: OPEN_PULSE via the UDA backend in remote mode
//     (backend=hdf5&cache_mode=none), descend the same aos_chain, read the
//     leaf back, assert self-consistency against what was written.
//
// Oracle: per-backend write->read self-consistency (same as MDSplus's
// version of this matrix) -- only the field *path* is real DD; the *content*
// stays opaque synthetic data (al_contract::synth_buffer).
#ifdef AL_CONTRACT_HAVE_UDA

#include "al_contract.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <complex>
#include <filesystem>
#include <string>
#include <vector>

namespace {

enum class DType { Char, Int, Double, Complex };

// Same shape as test_mdsplus_real_paths.cpp's PathSpec. `terminal_gap_reason`
// is shared verbatim with the MDSplus matrix (it is a fact about DD 4.1.1
// itself, not about either backend). `divergence_reason` is UDA-specific --
// re-derived empirically against the reference stack, not copied from
// MDSplus's own divergence rows, since the two backends can diverge for
// different reasons on the same cell.
struct PathSpec {
    DType                    dt;
    int                      rank;
    const char*              ids;
    std::vector<const char*> aos_chain;
    const char*              leaf;
    const char*              terminal_gap_reason;
    const char*              divergence_reason = nullptr;
    // Non-null iff the cell hits the genuine, newly-discovered defect pinned
    // by UdaAosKnownDefects (test_uda_breadth.cpp): a *dynamic* (time-varying)
    // leaf nested inside a struct_array silently comes back empty/absent
    // through UDA remote mode + backend=hdf5, instead of what was written.
    // Distinct from divergence_reason -- this is a real bug (xfail), not a
    // legitimate storage-model difference; the cell is skipped in the breadth
    // matrix (not run into the known-wrong assertion) and pinned exactly by
    // UdaRealPathMatrixKnownDefects's DISABLED_/tripwire pair below.
    const char*              known_defect_reason = nullptr;
};

// Keep the three exact known-defect cells as named specs so the breadth matrix
// and the DISABLED_/tripwire pair below exercise identical IDS paths, ranks,
// and AOS nesting without duplicating that configuration.
const PathSpec kComplexR1DynamicAosSpec{
    DType::Complex, 1, "waves", {"coherent_wave", "full_wave", "e_field/plus"},
    "values", nullptr, nullptr,
    "dynamic leaf nested in a struct_array (3 AOS levels)"};
const PathSpec kComplexR3DynamicAosSpec{
    DType::Complex, 3, "runaway_electrons", {"distribution/markers"},
    "orbit_integrals_instant/values", nullptr, nullptr,
    "dynamic leaf nested in a struct_array (1 AOS level)"};
const PathSpec kComplexR5DynamicAosSpec{
    DType::Complex, 5, "runaway_electrons", {"distribution/markers"},
    "orbit_integrals/values", nullptr, nullptr,
    "dynamic leaf nested in a struct_array (1 AOS level)"};

// Identical path/rank/aos_chain curation to test_mdsplus_real_paths.cpp's
// kPathSpecs (issue #25 acceptance criterion 2: "curated real-path set reused
// as-is"). terminal_gap_reason carries over unchanged (a DD fact). CHAR r0's
// divergence_reason is UDA-specific -- see below.
const PathSpec kPathSpecs[] = {
    // --- CHAR ---------------------------------------------------------------
    // Skipped WITHOUT attempting the seed write: seeding this cell means
    // writing a dim=0 (bare scalar) CHAR value through the plain HDF5
    // backend used for fixture setup, which is the pre-existing,
    // independently-pinned HDF5 CHAR-scalar crash
    // (RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips / tripwire
    // …CurrentlyCrashes, TRACEABILITY.md Part 1) -- confirmed to reproduce on
    // a real DD leaf too (equilibrium's code/name, STR_0D), not just the
    // synthetic path that test already pins. This is a fixture-setup-backend
    // defect, not a UDA behavior, so it is recorded as a divergence (not
    // exercised) rather than run into the crash.
    {DType::Char, 0, "equilibrium", {}, "code/name", nullptr,
     "seeding this cell needs a dim=0 CHAR write through the plain HDF5 "
     "backend (fixture setup) -- the pre-existing, independently-pinned "
     "HDF5 CHAR-scalar crash (RoundTripKnownDefects.DISABLED_"
     "Hdf5CharScalarRoundTrips / tripwire …CurrentlyCrashes, TRACEABILITY.md "
     "Part 1) reproduces on this real DD leaf too; not run to avoid crashing "
     "the seed step. A fixture-backend defect, not a UDA behavior"},
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
    // Known defect (xfail), not attempted: waves/coherent_wave/full_wave/
    // e_field/plus/values is a DD *dynamic* leaf 3 levels deep in nested
    // struct_arrays -- confirmed to hit UdaAosKnownDefects's dynamic-leaf-
    // inside-AOS defect (test_uda_breadth.cpp): the field comes back
    // empty/absent through UDA remote mode + backend=hdf5 instead of what
    // was written.
    kComplexR1DynamicAosSpec,
    {DType::Complex, 2, "gyrokinetics_local", {},
     "non_linear/fields_zonal_2d/phi_potential_perturbed_norm", nullptr},
    // Same known defect as COMPLEX r1 above: orbit_integrals_instant/values
    // is a dynamic leaf nested one level inside runaway_electrons/
    // distribution/markers.
    kComplexR3DynamicAosSpec,
    {DType::Complex, 4, "gyrokinetics_local", {},
     "non_linear/fields_4d/phi_potential_perturbed_norm", nullptr},
    // Same known defect as COMPLEX r1/r3 above.
    kComplexR5DynamicAosSpec,
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

// One real-path round trip: seed a fresh pulse dir through the plain HDF5
// backend (descending spec.aos_chain, one al_begin_arraystruct_action per
// segment, a single element each, mirroring test_mdsplus_real_paths.cpp's
// run_cell), then reopen the identical pulse dir through the UDA backend in
// remote mode and read the same path back.
template <class T, class ReadExpectation>
void run_cell_with_read_expectation(const PathSpec& spec,
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
void run_cell(const PathSpec& spec) {
    run_cell_with_read_expectation<T>(
        spec, [](int ctx, const PathSpec& read_spec,
                 const std::vector<int>& shape,
                 const std::vector<T>& written) {
            read_leaf_and_expect<T>(ctx, read_spec.leaf, read_spec.rank, shape,
                                    written);
        });
}

class UdaRealPathMatrix : public ::testing::TestWithParam<PathSpec> {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_UDA_UNCONFIGURED(); }
};

TEST_P(UdaRealPathMatrix, RealPathRoundTrip) {
    const PathSpec spec = GetParam();
    if (spec.terminal_gap_reason != nullptr) {
        GTEST_SKIP() << "terminal-gap: " << spec.terminal_gap_reason;
    }
    if (spec.divergence_reason != nullptr) {
        GTEST_SKIP() << "divergence: " << spec.divergence_reason;
    }
    if (spec.known_defect_reason != nullptr) {
        GTEST_SKIP() << "known-defect (xfail): " << spec.known_defect_reason;
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
std::string NameUdaRealPathMatrixCase(
    const ::testing::TestParamInfo<PathSpec>& info) {
    static const char* const kNames[] = {"CHAR", "INTEGER", "DOUBLE",
                                         "COMPLEX"};
    return std::string(kNames[static_cast<int>(info.param.dt)]) + "_r" +
           std::to_string(info.param.rank);
}

INSTANTIATE_TEST_SUITE_P(Uda, UdaRealPathMatrix, ::testing::ValuesIn(kPathSpecs),
                         NameUdaRealPathMatrixCase);

// Exact D2 pins for the three COMPLEX matrix cells affected by the dynamic-
// leaf-inside-AOS defect. The disabled correct-contract test and enabled
// current-behavior tripwire deliberately share run_cell_with_read_expectation,
// so both traverse the same public C ABI and the same real DD path. A targeted
// fix to any one rank/path makes that case's tripwire fail independently.
class UdaRealPathMatrixKnownDefects
    : public ::testing::TestWithParam<PathSpec> {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_UDA_UNCONFIGURED(); }
};

TEST_P(UdaRealPathMatrixKnownDefects,
       DISABLED_DynamicComplexLeafInsideAosRoundTrips) {
    run_cell<std::complex<double>>(GetParam());
}

TEST_P(UdaRealPathMatrixKnownDefects,
       DynamicComplexLeafInsideAosCurrentlyReadsEmpty) {
    run_cell_with_read_expectation<std::complex<double>>(
        GetParam(), [](int ctx, const PathSpec& spec,
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

const PathSpec kComplexDynamicAosKnownDefectSpecs[] = {
    kComplexR1DynamicAosSpec,
    kComplexR3DynamicAosSpec,
    kComplexR5DynamicAosSpec,
};

INSTANTIATE_TEST_SUITE_P(
    Uda, UdaRealPathMatrixKnownDefects,
    ::testing::ValuesIn(kComplexDynamicAosKnownDefectSpecs),
    NameUdaRealPathMatrixCase);

}  // namespace

#endif  // AL_CONTRACT_HAVE_UDA
