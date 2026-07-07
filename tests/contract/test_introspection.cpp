// Introspection / diagnostics contract tests — the thin unit tier.
//
// TEST_STRATEGY.md §4 step 3 + issue #5: the pure-lookup surface of the C ABI
// (FUNCTIONALITY_INVENTORY.md "Cluster 4 — Introspection / diagnostics",
// lines 434-459). These four functions take no context and touch no backend,
// so they are the genuinely-unit part of a C-ABI-only layer (decision D6) and
// need zero DD artifacts (decision D5).
//
// Every assertion here is *intended contract* (decision D2), asserted
// directly — including the silent "" for unmapped ids, which the inventory
// documents as deliberate (al_const.cpp:4-11), not a defect. Nothing in this
// file is expected-fail; the crash-class defects live in test_known_defects.cpp.

#include <al_const.h>
#include <al_defs.h>   // AL_VERSION, DD_VERSION — the compiled-in macros we
                       // check getALVersion()/getDDVersion() actually return.

#include <gtest/gtest.h>

#include <string>

namespace {

// ---------------------------------------------------------------------------
// const2str — maps a vocabulary constant to its symbolic name.
// ---------------------------------------------------------------------------

// Mapped ids: a representative spread across the constmap categories (backend,
// operation, access mode, pulse action, datatype). Table-driven so a missing
// or renamed entry names itself in the failure.
TEST(Introspection, Const2StrMapsKnownConstantsToTheirNames) {
    struct Case { int id; const char* name; };
    const Case cases[] = {
        {NO_BACKEND, "NO_BACKEND"},
        {ASCII_BACKEND, "ASCII_BACKEND"},
        {MDSPLUS_BACKEND, "MDSPLUS_BACKEND"},
        {HDF5_BACKEND, "HDF5_BACKEND"},
        {MEMORY_BACKEND, "MEMORY_BACKEND"},
        {UDA_BACKEND, "UDA_BACKEND"},
        {GLOBAL_OP, "GLOBAL_OP"},
        {SLICE_OP, "SLICE_OP"},
        {READ_OP, "READ_OP"},
        {WRITE_OP, "WRITE_OP"},
        {OPEN_PULSE, "OPEN_PULSE"},
        {FORCE_CREATE_PULSE, "FORCE_CREATE_PULSE"},
        {CLOSE_PULSE, "CLOSE_PULSE"},
        {ERASE_PULSE, "ERASE_PULSE"},
        {CHAR_DATA, "CHAR_DATA"},
        {INTEGER_DATA, "INTEGER_DATA"},
        {DOUBLE_DATA, "DOUBLE_DATA"},
        {COMPLEX_DATA, "COMPLEX_DATA"},
    };
    for (const Case& c : cases) {
        const char* got = const2str(c.id);
        ASSERT_NE(got, nullptr) << "const2str must never return NULL (id=" << c.id << ")";
        EXPECT_EQ(std::string(got), c.name) << "const2str(" << c.id << ")";
    }
}

// Unmapped id: an out-of-range value is not in constmap, so the documented
// contract is a silent empty string (NOT NULL, NOT an error). al_const.cpp:7-8.
TEST(Introspection, Const2StrReturnsEmptyForUnmappedId) {
    const char* got = const2str(999999);
    ASSERT_NE(got, nullptr) << "unmapped id must yield \"\", never NULL";
    EXPECT_STREQ(got, "") << "unmapped id must silently return an empty string";
}

// Known map-coverage gaps, asserted as the *intended* silent-"" contract:
// FUNCTIONALITY_INVENTORY.md:75-79 & 451-454 record that TIMERANGE_OP and
// FLEXBUFFERS_BACKEND are defined constants deliberately absent from
// alconst::constmap, so const2str returns "" for them rather than erroring.
// This pins that documented quirk so a future map change is a visible event.
TEST(Introspection, Const2StrReturnsEmptyForDefinedButUnmappedConstants) {
    EXPECT_STREQ(const2str(TIMERANGE_OP), "")
        << "TIMERANGE_OP is a defined constant absent from constmap "
           "(FUNCTIONALITY_INVENTORY.md:451-454)";
    EXPECT_STREQ(const2str(FLEXBUFFERS_BACKEND), "")
        << "FLEXBUFFERS_BACKEND is a defined constant absent from constmap "
           "(FUNCTIONALITY_INVENTORY.md:75-79)";
}

// ---------------------------------------------------------------------------
// err2str — maps an error code to its name (a separate map from const2str).
// ---------------------------------------------------------------------------

TEST(Introspection, Err2StrMapsKnownErrorCodesToTheirNames) {
    struct Case { int id; const char* name; };
    const Case cases[] = {
        {UNKNOWN_ERR, "UNKNOWN_ERR"},
        {CONTEXT_ERR, "CONTEXT_ERR"},
        {BACKEND_ERR, "BACKEND_ERR"},
        {LOWLEVEL_ERR, "LOWLEVEL_ERR"},
    };
    for (const Case& c : cases) {
        const char* got = err2str(c.id);
        ASSERT_NE(got, nullptr) << "err2str must never return NULL (id=" << c.id << ")";
        EXPECT_EQ(std::string(got), c.name) << "err2str(" << c.id << ")";
    }
}

TEST(Introspection, Err2StrReturnsEmptyForUnmappedId) {
    const char* got = err2str(999999);
    ASSERT_NE(got, nullptr) << "unmapped error id must yield \"\", never NULL";
    EXPECT_STREQ(got, "") << "unmapped error id must silently return an empty string";
}

// const2str and err2str are backed by *distinct* maps: an error code resolves
// only through err2str, a vocabulary constant only through const2str. The two
// key spaces don't even overlap — error codes are negative (al_defs.h ERR_0=-1)
// and constmap keys positive — so this documents the two-namespace design
// deterministically, in both directions.
TEST(Introspection, Const2StrAndErr2StrUseSeparateMaps) {
    // An error code is not a vocabulary constant: const2str must not resolve it.
    EXPECT_STREQ(const2str(BACKEND_ERR), "")
        << "an error code must not resolve through const2str";
    EXPECT_EQ(std::string(err2str(BACKEND_ERR)), "BACKEND_ERR");

    // Symmetrically, a backend constant is not an error code.
    EXPECT_STREQ(err2str(HDF5_BACKEND), "")
        << "a vocabulary constant must not resolve through err2str";
    EXPECT_EQ(std::string(const2str(HDF5_BACKEND)), "HDF5_BACKEND");
}

// ---------------------------------------------------------------------------
// getALVersion — returns the AL library version string.
// ---------------------------------------------------------------------------

// The tightest self-consistent oracle: the function must return exactly the
// compiled-in AL_VERSION macro (al_defs.h, from CMake PROJECT_VERSION). This
// verifies the linkage returns the constant, not merely that it is non-empty.
TEST(Introspection, GetALVersionReturnsCompiledVersionMacro) {
    const char* got = getALVersion();
    ASSERT_NE(got, nullptr);
    EXPECT_STREQ(got, AL_VERSION);
    EXPECT_FALSE(std::string(got).empty()) << "AL version must not be empty";
}

// And it must be a dotted-numeric version whose first three components parse as
// integers — the shape python/tests/test_imasdef.py::test_AL_version relies on.
TEST(Introspection, GetALVersionIsDottedNumeric) {
    const std::string v = getALVersion();
    int components = 0;
    size_t start = 0;
    while (start <= v.size() && components < 3) {
        size_t dot = v.find('.', start);
        const std::string tok = v.substr(
            start, dot == std::string::npos ? std::string::npos : dot - start);
        ASSERT_FALSE(tok.empty()) << "empty version component in '" << v << "'";
        for (char ch : tok) {
            ASSERT_TRUE(ch >= '0' && ch <= '9')
                << "non-numeric version component '" << tok << "' in '" << v << "'";
        }
        ++components;
        if (dot == std::string::npos) break;
        start = dot + 1;
    }
    EXPECT_GE(components, 3)
        << "expected at least MAJOR.MINOR.PATCH in '" << v << "'";
}

// ---------------------------------------------------------------------------
// getDDVersion — the deprecated sentinel (intended contract).
// ---------------------------------------------------------------------------

// CLAUDE.md + include/al_defs.h.in:57 deliberately set DD_VERSION to
// "!!DEPRECATED!!"; python/tests/test_imasdef.py::test_DD_version asserts the
// same at the binding layer. The core carries no compile-time DD coupling, so
// the C ABI must surface this sentinel verbatim — not real version info.
TEST(Introspection, GetDDVersionReturnsDeprecatedSentinel) {
    const char* got = getDDVersion();
    ASSERT_NE(got, nullptr);
    EXPECT_STREQ(got, "!!DEPRECATED!!");
    EXPECT_STREQ(got, DD_VERSION) << "must match the compiled-in DD_VERSION macro";
}

}  // namespace
