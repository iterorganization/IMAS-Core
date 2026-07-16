// Unknown-backend-ID coverage at the C-ABI factory boundary (issue #47).
//
// TRACEABILITY.md Part 2 marked Backend::initBackend's unrecognized-ID throw
// path (src/al_backend.cpp) covered only "by inspection". These tests give the
// refusal a real verdict through the public surface, at the two spots an
// unknown backend identifier can arrive from a caller:
//
//   - al_begin_dataentry_action with a URI naming an unknown (or empty)
//     backend: DataEntryContext::setBackendID (src/al_context.cpp) is the
//     factory-boundary gate that refuses it before initBackend runs. A
//     regression that "defaults" an unrecognized name to a real backend makes
//     the calls below succeed and the tests fail.
//   - al_build_uri_from_legacy_parameters with an unknown positive numeric
//     backend ID: DataEntryContext::getURIBackend refuses to synthesize a URI.
//
// Everything asserted here is hermetic (no backend substrate is ever
// constructed), so the suite name is classified into the `unit` tier by
// assign_contract_labels.cmake.in.

#include "al_contract.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace {

constexpr int kUntouchedCtx = -424242;  // sentinel: must survive failed calls

bool dir_is_empty(const std::string& p) {
    namespace fs = std::filesystem;
    return fs::exists(p) && fs::is_directory(p) &&
           fs::begin(fs::directory_iterator(p)) == fs::end(fs::directory_iterator(p));
}

// ===========================================================================
// URI surface: unknown / empty backend names.
// ===========================================================================

TEST(BackendFactory, UnknownBackendNameInUriReturnsCleanError) {
    al_contract::TempBase tmp;
    int ctx = kUntouchedCtx;
    const std::string uri = "imas:nosuchbackend?path=" + tmp.str() + "/pulse";

    al_status_t s = al_begin_dataentry_action(uri.c_str(),
                                              alconst::force_create_pulse, &ctx);

    EXPECT_EQ(s.code, alerror::unknown_err) << "message: " << s.message;
    EXPECT_NE(std::string(s.message).find("Unable to identify a backend"),
              std::string::npos)
        << "actual message: " << s.message;
    // No context may be handed out and no artifact left on disk.
    EXPECT_EQ(ctx, kUntouchedCtx);
    EXPECT_TRUE(dir_is_empty(tmp.str()));
}

TEST(BackendFactory, EmptyBackendNameInUriReturnsCleanError) {
    al_contract::TempBase tmp;
    int ctx = kUntouchedCtx;
    const std::string uri = "imas:?path=" + tmp.str() + "/pulse";

    al_status_t s = al_begin_dataentry_action(uri.c_str(),
                                              alconst::force_create_pulse, &ctx);

    EXPECT_NE(s.code, 0);
    EXPECT_EQ(ctx, kUntouchedCtx);
    EXPECT_TRUE(dir_is_empty(tmp.str()));
}

TEST(BackendFactory, UnknownBackendNameIsRefusedForEveryOpenMode) {
    al_contract::TempBase tmp;
    const int modes[] = {alconst::open_pulse, alconst::force_open_pulse,
                         alconst::create_pulse, alconst::force_create_pulse};
    const std::string uri = "imas:nosuchbackend?path=" + tmp.str() + "/pulse";
    for (int mode : modes) {
        int ctx = kUntouchedCtx;
        al_status_t s = al_begin_dataentry_action(uri.c_str(), mode, &ctx);
        EXPECT_EQ(s.code, alerror::unknown_err) << "mode=" << mode;
        EXPECT_EQ(ctx, kUntouchedCtx) << "mode=" << mode;
    }
    EXPECT_TRUE(dir_is_empty(tmp.str()));
}

// ===========================================================================
// Legacy surface: unknown positive numeric backend ID.
// ===========================================================================

TEST(BackendFactory, UnknownPositiveLegacyBackendIdIsRefused) {
    std::string uri;
    al_status_t s = al_build_uri_from_legacy_parameters(
        /*backendID=*/424242, /*pulse=*/1, /*run=*/1, "user", "test", "3",
        /*options=*/"", uri);

    EXPECT_EQ(s.code, alerror::lowlevel_err) << "message: " << s.message;
    EXPECT_NE(std::string(s.message).find("getURIBackend"), std::string::npos)
        << "actual message: " << s.message;
    EXPECT_TRUE(uri.empty()) << "no URI may be synthesized, got: " << uri;
}

}  // namespace
