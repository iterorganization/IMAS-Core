// Version sentinel contract test — the tripwire for accidental DD-awareness.
//
// TEST_STRATEGY.md decision D5 + issue #5: the core must treat
// `ids_properties/version_put/data_dictionary` as ordinary opaque string data.
// It is the field the HLI uses to record which DD version wrote the data, but
// per CLAUDE.md the core interprets no DD version anywhere — it stores what it
// is given and returns it verbatim. This test pins that: a write→read of an
// obviously-fake version string comes back byte-for-byte, AND getDDVersion()
// stays the deprecated sentinel, so any future move to version-awareness shows
// up here as a deliberate, visible change rather than an accident (this is the
// current-behavior test that Task 3's version-negotiation work will watch).
//
// Runs on Memory (hermetic, unit-ish) and HDF5 (the real on-disk path): the
// non-interpretation property is backend-independent, so proving it on both
// rules out a backend quietly parsing the field.

#include "al_contract.h"  // pulls in al_lowlevel.h + al_const.h transitively

#include <gtest/gtest.h>

#include <string>

using al_contract::BackendCase;
using al_contract::PulseId;

namespace {

// The DD path the HLI writes the writer's DD version into. Opaque to the core.
constexpr const char* kIds = "magnetics";
constexpr const char* kVersionPutPath =
    "ids_properties/version_put/data_dictionary";

// Deliberately NOT a real DD version and NOT the deprecated sentinel: if the
// core ever interpreted this field, storing this value would either be rejected
// or would perturb getDDVersion(). Neither may happen.
constexpr const char* kFakeVersion = "9.9.9-sentinel-not-a-real-dd";

// The compile-time value getDDVersion() must keep returning regardless of what
// version string is stored (include/al_defs.h.in:57; test_introspection.cpp
// cross-checks it against the DD_VERSION macro).
constexpr const char* kDeprecatedSentinel = "!!DEPRECATED!!";

class VersionSentinel : public ::testing::TestWithParam<BackendCase> {
protected:
    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/13, /*run=*/0};
};

TEST_P(VersionSentinel, VersionPutRoundTripsOpaquelyAndIsNotInterpreted) {
    const BackendCase backend = GetParam();
    if (backend.on_disk) {
        base_.make_legacy_tree(pulse_);
    }

    // Baseline: the sentinel before any data is written.
    const std::string dd_before = getDDVersion();
    EXPECT_EQ(dd_before, kDeprecatedSentinel);

    const std::string uri =
        al_contract::build_uri(backend.id, base_.str(), pulse_);
    ASSERT_FALSE(uri.empty());

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE,
                                           &pulse_ctx));

    // --- write the fake version into version_put/data_dictionary ---
    {
        int op_ctx = -1;
        AL_ASSERT_OK(
            al_begin_global_action(pulse_ctx, kIds, "", WRITE_OP, &op_ctx));
        AL_EXPECT_OK(
            al_contract::write_char_array(op_ctx, kVersionPutPath, kFakeVersion));
        AL_ASSERT_OK(al_end_action(op_ctx));
    }

    // Writing a DD-version string must not have taught the core a DD version:
    // getDDVersion() is invariant under stored data. Compared to the captured
    // baseline (not the literal sentinel) so this asserts *non-interpretation*
    // itself — the specific "!!DEPRECATED!!" value is pinned by
    // test_introspection.cpp::GetDDVersionReturnsDeprecatedSentinel.
    EXPECT_EQ(std::string(getDDVersion()), dd_before)
        << "the core must not adopt a stored version_put value as its DD version";

    // --- read it back; it must be the exact bytes we wrote (opaque) ---
    {
        int op_ctx = -1;
        AL_ASSERT_OK(
            al_begin_global_action(pulse_ctx, kIds, "", READ_OP, &op_ctx));
        std::string read_back;
        AL_EXPECT_OK(
            al_contract::read_char_array(op_ctx, kVersionPutPath, &read_back));
        EXPECT_EQ(read_back, kFakeVersion)
            << "version_put/data_dictionary must round-trip verbatim, unparsed";
        AL_ASSERT_OK(al_end_action(op_ctx));
    }

    EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);

    // Still unchanged after a full write→read cycle.
    EXPECT_EQ(std::string(getDDVersion()), dd_before);
}

INSTANTIATE_TEST_SUITE_P(
    Backends, VersionSentinel,
    ::testing::Values(BackendCase{HDF5_BACKEND, "HDF5", /*on_disk=*/true},
                      BackendCase{MEMORY_BACKEND, "Memory", /*on_disk=*/false}),
    [](const ::testing::TestParamInfo<BackendCase>& info) {
        return std::string(info.param.name);
    });

}  // namespace
