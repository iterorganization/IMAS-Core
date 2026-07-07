// Known-defect contract tests — the expected-fail mechanism proof.
//
// Decision D2 (TEST_STRATEGY.md): for a genuine defect we write the test for
// the *correct* behavior and mark it expected-fail, never freezing the bug as
// if it were the spec. When someone fixes the defect the expected-fail flips to
// pass — a signal, not a regression.
//
// Scaffold defect: al_setvalue_*_parameter_plugin called with an *unregistered*
// plugin name crashes instead of returning an error.
//   FUNCTIONALITY_INVENTORY.md:410-417
//   src/al_lowlevel.cpp:453-457  (llpluginsStore[name] default-constructs an
//   LLplugin with al_plugin == NULL, then dereferences it — an uncatchable
//   null-pointer dereference the surrounding try/catch cannot convert to an
//   al_status_t failure).
//
// GoogleTest was chosen over Catch2 precisely for this class of defect
// (decision D3): the crash would take down an in-process runner, so we isolate
// it in a death test.

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <csignal>

namespace {

constexpr const char* kUnregistered = "no_such_plugin_ever_registered";

// CORRECT-CONTRACT test, expected-fail.
//
// Marked expected-fail via the DISABLED_ prefix: it does not run in the default
// `ctest` pass, so it can't crash the runner, but it stays compiled and is
// listed by `--gtest_list_tests`. The day the null-deref is fixed to return an
// error, drop the DISABLED_ prefix and this becomes a live green assertion —
// the suite then documents the fixed contract.
//
// (GoogleTest has no first-class xfail; DISABLED_ + this comment + the paired
// death test below is the framework's idiom for "known defect, correct
// behavior asserted, tracked".)
TEST(KnownDefects, DISABLED_SetValueIntScalarUnregisteredPluginReturnsError) {
    al_status_t s =
        al_setvalue_int_scalar_parameter_plugin("param", 1, kUnregistered);
    // The correct contract: report the unknown plugin as an error, like the
    // registration checks the other Cluster-3 calls perform.
    EXPECT_NE(s.code, 0)
        << "setting a parameter on an unregistered plugin must return an "
           "error, not crash";
}

// CURRENT-BEHAVIOR guard (death test).
//
// Actively pins today's crash so that a fix — which turns the crash into an
// error return — makes THIS test fail loudly, forcing whoever fixed it to also
// enable the correct-contract test above. This is the tripwire that keeps the
// quirk catalog honest; it asserts "it currently dies", not "it should die".
TEST(KnownDefectsDeath, SetValueIntScalarUnregisteredPluginCurrentlyCrashes) {
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    // Pinned to SIGSEGV specifically (the null-pointer dereference), not "any
    // death" — an unrelated abort must not be mistaken for the defect being
    // present. `.*` on the stderr matcher because a bare SIGSEGV prints nothing.
    ASSERT_EXIT(
        {
            al_setvalue_int_scalar_parameter_plugin("param", 1, kUnregistered);
        },
        ::testing::KilledBySignal(SIGSEGV), ".*")
        << "expected the unregistered-plugin null-deref (see "
           "FUNCTIONALITY_INVENTORY.md:410). If this no longer dies by SIGSEGV, "
           "the defect was likely fixed — enable "
           "KnownDefects.SetValueIntScalarUnregisteredPluginReturnsError.";
}

}  // namespace
