// Always-on meta-tests for the shared real-DD-path catalog (issue #46).
//
// These guard the catalog's own invariants (one verdict per cell, no duplicate
// keys) independently of any backend. The per-backend "the matrix covers every
// catalog key" meta-tests live in test_mdsplus_real_paths.cpp and
// test_uda_real_paths.cpp behind their compile gates, so a matrix cannot drop a
// cell without a red test on that backend.

#include "real_dd_path_catalog.h"

#include <gtest/gtest.h>

#include <set>
#include <utility>

namespace {

using al_contract::real_dd::catalog;
using al_contract::real_dd::CatalogEntry;
using al_contract::real_dd::DType;
using al_contract::real_dd::kMaxRank;
using al_contract::real_dd::kMinRank;
using al_contract::real_dd::kNumCells;
using al_contract::real_dd::kNumDTypes;

using Key = std::pair<int, int>;  // (datatype ordinal, rank)

Key key_of(const CatalogEntry& e) {
    return {static_cast<int>(e.dt), e.rank};
}

// The catalog defines exactly one verdict for every (datatype, rank) cell in
// the swept space, with no duplicate and no missing key. This is the shared
// half of issue #46's "exactly one verdict for every catalog key, no
// duplicates" acceptance criterion; the per-backend halves pin that each
// matrix maps this same key set 1:1.
TEST(RealDdPathCatalog, CoversEveryCellExactlyOnce) {
    std::set<Key> seen;
    for (const CatalogEntry& e : catalog()) {
        EXPECT_TRUE(seen.insert(key_of(e)).second)
            << "duplicate catalog key: dtype=" << static_cast<int>(e.dt)
            << " rank=" << e.rank;
    }
    EXPECT_EQ(catalog().size(), static_cast<std::size_t>(kNumCells));

    // Every cell in the swept space is present.
    for (int d = 0; d < kNumDTypes; ++d) {
        for (int r = kMinRank; r <= kMaxRank; ++r) {
            EXPECT_TRUE(seen.count({d, r}) == 1)
                << "missing catalog key: dtype=" << d << " rank=" << r;
        }
    }
}

// A terminal-gap cell carries its DD-fact reason and no path; a real-path cell
// carries a path and no gap reason. The two are mutually exclusive, so neither
// backend suite can misread a half-populated entry.
TEST(RealDdPathCatalog, PathAndTerminalGapAreMutuallyExclusive) {
    for (const CatalogEntry& e : catalog()) {
        const bool is_gap = e.terminal_gap_reason != nullptr;
        if (is_gap) {
            EXPECT_EQ(e.ids, nullptr)
                << "terminal-gap cell " << al_contract::real_dd::case_name(e)
                << " must not name an IDS";
            EXPECT_EQ(e.leaf, nullptr)
                << "terminal-gap cell " << al_contract::real_dd::case_name(e)
                << " must not name a leaf";
            EXPECT_TRUE(e.aos_chain.empty())
                << "terminal-gap cell " << al_contract::real_dd::case_name(e)
                << " must not carry an AOS chain";
        } else {
            EXPECT_NE(e.ids, nullptr)
                << "real-path cell " << al_contract::real_dd::case_name(e)
                << " must name an IDS";
            EXPECT_NE(e.leaf, nullptr)
                << "real-path cell " << al_contract::real_dd::case_name(e)
                << " must name a leaf";
        }
    }
}

}  // namespace
