// Shared real-DD-path catalog for the MDSplus (#15) and UDA (#25) breadth
// matrices (issue #46, TRACEABILITY.md Parts 4/5).
//
// Both real-path suites sweep the same {CHAR, INTEGER, DOUBLE, COMPLEX} x rank
// {0 .. MAXDIM} space against curated *real* DD-4.1.1 paths (a made-up field
// name has no node in the schema either backend validates against). The path
// for each (datatype, rank) cell -- the IDS, the struct_array chain to enter,
// the leaf, and whether DD 4.1.1 even has such a field -- is a fact about the
// DD, identical for both backends. This header is the single source of truth
// for that catalog so a path correction, a terminal-gap rationale, or a new
// (type, rank) cell is edited in exactly one place and the two matrices cannot
// silently diverge (issue #46).
//
// What stays OUT of the catalog, on purpose: backend-specific expected
// outcomes. A cell can be a plain pass on one backend and a storage-model
// divergence or a known xfail on the other (e.g. MDSplus's STR_0D dimension
// divergence vs UDA's dynamic-leaf-inside-AOS defect). Those verdicts live
// local to each backend suite; only the shared curated path lives here.
//
// Path curation method (carried over from test_mdsplus_real_paths.cpp's
// header): resolved by walking the actual baked-from artifact,
// build-mdsplus/_deps/data-dictionary-src/IDSDef.xml (the DD 4.1.1 source
// ALBuildDataDictionary.cmake downloads and the model tree is compiled from),
// to find, per (type, rank) cell, a real field of that data_type together with
// its struct_array ancestor chain -- empirical characterization against the
// real shipped artifact (TEST_STRATEGY D2). imas-dd MCP validation is used for
// catalog changes.
#ifndef AL_CONTRACT_REAL_DD_PATH_CATALOG_H
#define AL_CONTRACT_REAL_DD_PATH_CATALOG_H

#include <string>
#include <vector>

namespace al_contract {
namespace real_dd {

// The four C-ABI scalar datatypes the breadth matrices sweep.
enum class DType { Char, Int, Double, Complex };

// The DD version every curated path in this catalog was validated against.
inline constexpr const char* kCatalogDdVersion = "4.1.1";

// The swept space: kNumDTypes datatypes x ranks [kMinRank, kMaxRank].
inline constexpr int kNumDTypes = 4;
inline constexpr int kMinRank   = 0;
inline constexpr int kMaxRank   = 7;  // MAXDIM
inline constexpr int kNumRanks  = kMaxRank - kMinRank + 1;
inline constexpr int kNumCells  = kNumDTypes * kNumRanks;

// One curated (datatype, rank) cell -- the single source of truth for the DD
// path a backend breadth matrix stores at that cell.
//
// `terminal_gap_reason` non-null means DD kCatalogDdVersion has no field of
// this (dt, rank) at any nesting depth (a fact about the DD, shared by every
// backend); `ids`/`leaf` are then null and `aos_chain` empty. Otherwise the
// path fields name a real DD-conformant field: `aos_chain` lists the
// struct_array field(s) to enter in order (each may itself be a multi-segment
// path through intervening plain structures, e.g. "e_field/plus"); empty means
// the leaf hangs directly off the GLOBAL op.
//
// The CHAR rank axis is the raw C-ABI `dim` argument, one off from the DD's
// STR_ND suffix (IMAS convention: a *string* is CHAR dim=1, an *array of
// strings* is CHAR dim=2; dim=0 is a bare scalar char reusing the STR_0D node).
// INTEGER/DOUBLE/COMPLEX map dim directly to their _ND suffix.
struct CatalogEntry {
    DType                    dt;
    int                      rank;
    const char*              ids;                  // null iff terminal gap
    std::vector<const char*> aos_chain;            // struct_array chain to enter
    const char*              leaf;                 // null iff terminal gap
    const char*              terminal_gap_reason;  // null iff a real path exists
};

// The full curated catalog: exactly one entry per (dt, rank) cell, kNumCells
// entries, curated against DD kCatalogDdVersion.
const std::vector<CatalogEntry>& catalog();

// Canonical, stable parameter-name suffix for a cell, e.g. "DOUBLE_r3". Used
// verbatim as the INSTANTIATE_TEST_SUITE_P case-name in both backend suites, so
// existing test names / TRACEABILITY references stay stable across the #46
// extraction.
std::string case_name(DType dt, int rank);
inline std::string case_name(const CatalogEntry& e) {
    return case_name(e.dt, e.rank);
}

}  // namespace real_dd
}  // namespace al_contract

#endif  // AL_CONTRACT_REAL_DD_PATH_CATALOG_H
