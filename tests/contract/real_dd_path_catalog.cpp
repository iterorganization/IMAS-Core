// The shared real-DD-path catalog (issue #46). See real_dd_path_catalog.h for
// what belongs here (curated DD paths + terminal-gap DD facts) and what does
// not (backend-specific verdicts). Curated against DD 4.1.1
// (build-mdsplus/_deps/data-dictionary-src/IDSDef.xml).

#include "real_dd_path_catalog.h"

namespace al_contract {
namespace real_dd {

const std::vector<CatalogEntry>& catalog() {
    // Exactly one entry per (dt, rank) cell, in (dt, rank) order. The
    // CoversEveryCellExactlyOnce meta-test (test_real_dd_path_catalog.cpp) is
    // the guard that this stays a complete, duplicate-free kNumCells table.
    static const std::vector<CatalogEntry> kCatalog = {
        // --- CHAR -----------------------------------------------------------
        {DType::Char, 0, "equilibrium", {}, "code/name", nullptr},
        {DType::Char, 1, "equilibrium", {}, "code/name", nullptr},
        {DType::Char, 2, "b_field_non_axisymmetric", {}, "control_surface_names",
         nullptr},
        {DType::Char, 3, nullptr, {}, nullptr,
         "DD 4.1.1 has no STR_2D+ at all (only STR_0D/STR_1D exist -- there is "
         "no DD concept a CHAR dim>=3 could model)"},
        {DType::Char, 4, nullptr, {}, nullptr, "see rank 3"},
        {DType::Char, 5, nullptr, {}, nullptr, "see rank 3"},
        {DType::Char, 6, nullptr, {}, nullptr, "see rank 3"},
        {DType::Char, 7, nullptr, {}, nullptr, "see rank 3"},

        // --- INTEGER --------------------------------------------------------
        {DType::Int, 0, "amns_data", {}, "z_n", nullptr},
        {DType::Int, 1, "magnetics", {}, "code/output_flag", nullptr},
        {DType::Int, 2, "magnetics", {}, "b_field_pol_probe_equivalent",
         nullptr},
        {DType::Int, 3, "temporary", {"constant_integer3d"}, "value", nullptr},
        {DType::Int, 4, nullptr, {}, nullptr,
         "DD 4.1.1 has no INT_4D+ anywhere (max is INT_3D, itself only inside a "
         "struct_array)"},
        {DType::Int, 5, nullptr, {}, nullptr, "see rank 4"},
        {DType::Int, 6, nullptr, {}, nullptr, "see rank 4"},
        {DType::Int, 7, nullptr, {}, nullptr, "see rank 4"},

        // --- DOUBLE ---------------------------------------------------------
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

        // --- COMPLEX --------------------------------------------------------
        {DType::Complex, 0, nullptr, {}, nullptr,
         "DD 4.1.1 has no CPX_0D -- no scalar complex type exists at all"},
        {DType::Complex, 1, "waves",
         {"coherent_wave", "full_wave", "e_field/plus"}, "values", nullptr},
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
    return kCatalog;
}

std::string case_name(DType dt, int rank) {
    static const char* const kNames[] = {"CHAR", "INTEGER", "DOUBLE",
                                          "COMPLEX"};
    return std::string(kNames[static_cast<int>(dt)]) + "_r" +
           std::to_string(rank);
}

}  // namespace real_dd
}  // namespace al_contract
