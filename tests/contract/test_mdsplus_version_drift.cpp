// MDSplus version-drift check (issue #16, reworked by issue #35;
// TRACEABILITY.md Part 4).
//
// al_lowlevel.cpp's al_begin_dataentry_action compares the backend's compiled
// version (`getVersion(NULL)`) against the version stored in the pulse being
// opened (`getVersion(pctx)`) on OPEN_PULSE/FORCE_OPEN_PULSE and throws
// LOWLEVEL_ERR on a mismatch. MDSplus stores that version in the tree's
// VERSION:BACK_MAJOR/BACK_MINOR nodes, written once at pulse creation from
// the compiled constants (currently 1/1) and read back unchanged forever.
//
// Forcing the mismatch needs out-of-band tree surgery — but that mutation is
// FIXTURE PREPARATION, not contract: it lives in the separate build-gated
// producer mdsplus_fixture_tool.cpp (issue #35), which owns all direct
// MDSplus C++ API access. This file includes only AL/test headers and calls
// only `al_*` APIs, so a rewrite satisfying the C ABI is judged by exactly
// the C ABI — no second behavioral seam through <mdsobjects.h> or the legacy
// storage layout leaks into the asserted contract.
#if defined(AL_CONTRACT_HAVE_MDSPLUS) && defined(AL_CONTRACT_MDSPLUS_FIXTURE_TOOL)

#include "al_contract.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

using al_contract::PulseId;

namespace {

// A backend version the compiled MDSplus backend (MDSPLUS_BACKEND_MAJOR == 1,
// see mdsplus_backend.cpp) can never match, forcing al_lowlevel.cpp's major
// mismatch branch ((ver.first != sver.first) || ...).
constexpr int kMismatchedMajor = 99;

class MdsplusVersionDrift : public ::testing::Test {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_MDSPLUS_UNCONFIGURED(); }

    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/16,
                   /*run=*/0};

    // The directory DataEntryContext::buildFullPath computes for this pulse
    // (al_context.cpp:140-179) and hands to the backend as the URI "path"
    // query parameter -- i.e. where the "ids" tree/shot-1 files actually
    // live. Mirrors al_contract::TempBase::make_legacy_tree's own layout.
    std::string pulse_dir() const {
        return base_.str() + "/" + pulse_.database + "/" + pulse_.version +
               "/" + std::to_string(pulse_.pulse) + "/" +
               std::to_string(pulse_.run);
    }

    // Out-of-band fixture mutation via the isolated setup tool
    // (mdsplus_fixture_tool.cpp) — the producer of the mismatched fixture,
    // not part of the asserted contract.
    void PokeStoredBackendMajorVersion(int mismatched_major) {
        const std::string cmd = std::string("\"") +
                                AL_CONTRACT_MDSPLUS_FIXTURE_TOOL + "\" \"" +
                                pulse_dir() + "\" " +
                                std::to_string(mismatched_major);
        ASSERT_EQ(std::system(cmd.c_str()), 0)
            << "fixture tool failed: " << cmd;
    }
};

// Matching case: a pulse created and immediately reopened by the same
// compiled backend carries the same VERSION:BACK_MAJOR/MINOR it just wrote,
// so the drift check's comparison is a no-op and OPEN_PULSE succeeds.
TEST_F(MdsplusVersionDrift, MatchingVersionOpensCleanly) {
    base_.make_legacy_tree(pulse_);
    const std::string uri =
        al_contract::build_uri(MDSPLUS_BACKEND, base_.str(), pulse_);
    ASSERT_FALSE(uri.empty());

    int create_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE,
                                           &create_ctx));
    EXPECT_EQ(al_close_pulse(create_ctx, CLOSE_PULSE).code, 0);

    int open_ctx = -1;
    al_status_t status =
        al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &open_ctx);
    EXPECT_TRUE(al_contract::IsOk(status));
    if (status.code == 0) {
        EXPECT_EQ(al_close_pulse(open_ctx, CLOSE_PULSE).code, 0);
    }
}

// Mismatching case: after creation, VERSION:BACK_MAJOR is overwritten (by the
// fixture tool) to a value the compiled backend (MDSPLUS_BACKEND_MAJOR == 1)
// can never match. Reopening through the C ABI must hit al_lowlevel.cpp's
// `(ver.first != sver.first)` branch and fail with LOWLEVEL_ERR, never a
// silent open of stale/incompatible data.
TEST_F(MdsplusVersionDrift, MismatchedBackendVersionRefusesOpen) {
    base_.make_legacy_tree(pulse_);
    const std::string uri =
        al_contract::build_uri(MDSPLUS_BACKEND, base_.str(), pulse_);
    ASSERT_FALSE(uri.empty());

    int create_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE,
                                           &create_ctx));
    EXPECT_EQ(al_close_pulse(create_ctx, CLOSE_PULSE).code, 0);

    PokeStoredBackendMajorVersion(kMismatchedMajor);

    int open_ctx = -1;
    al_status_t status =
        al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &open_ctx);
    EXPECT_NE(status.code, 0)
        << "expected the version-drift check to refuse opening a pulse whose "
           "stored VERSION:BACK_MAJOR ("
        << kMismatchedMajor << ") doesn't match the compiled backend's";
    EXPECT_NE(std::string(status.message).find("Compatibility"),
              std::string::npos)
        << "status.message=" << status.message;
}

}  // namespace

#endif  // AL_CONTRACT_HAVE_MDSPLUS && AL_CONTRACT_MDSPLUS_FIXTURE_TOOL
