// MDSplus version-drift check (issue #16, TRACEABILITY.md Part 4).
//
// al_lowlevel.cpp's al_begin_dataentry_action (src/al_lowlevel.cpp:868-883)
// compares the backend's compiled version (`getVersion(NULL)`) against the
// version stored in the pulse being opened (`getVersion(pctx)`) on
// OPEN_PULSE/FORCE_OPEN_PULSE and throws LOWLEVEL_ERR on a mismatch. This is
// generic al_lowlevel.cpp logic shared by every backend, but MDSplus is the
// one backend where `getVersion(pctx)` reads a value a pulse can genuinely
// carry different from today's build: `VERSION:BACK_MAJOR`/`BACK_MINOR`,
// written into the tree once at pulse creation
// (`mdsplus_backend.cpp`'s `saveVersion`, called from `createPulse`) from the
// compiled `MDSPLUS_BACKEND_MAJOR`/`MINOR` constants (currently 1/1) and read
// back unchanged forever after. Forcing the mismatch case therefore needs
// writing a different value into that node after the fact -- through the raw
// MDSplus C++ API (`<mdsobjects.h>`), not the public C ABI, exactly the kind
// of out-of-band access issue #12's Part 2 row B ("gap", see TRACEABILITY.md)
// declined for HDF5 because that suite has no HDF5 include/link wiring. This
// file adds that wiring for MDSplus only (tests/contract/CMakeLists.txt,
// gated the same way as AL_CONTRACT_HAVE_MDSPLUS): the MDSplus tier already
// links libMDSplus into `al` PRIVATE-ly for the backend itself, and doing the
// same for `contract_tests` is a test-only, build-gated addition -- so this
// finding *is* closeable for MDSplus even though it stayed a terminal gap for
// HDF5 (whose always-on suite deliberately doesn't carry an HDF5 C API
// dependency at all).
#ifdef AL_CONTRACT_HAVE_MDSPLUS

#include "al_contract.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <mdsobjects.h>

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

    // Reproduces MDSplusBackend::setDataEnv/resetIdsPath's "ids_path" env-var
    // dance (mdsplus_backend.cpp:1458-1517) well enough to open the same tree
    // the backend itself opened through the C ABI -- this test has no access
    // to the backend's private state, only the public MDSplus tree API.
    void PokeStoredBackendMajorVersion(int mismatched_major) {
        const char* models_path = std::getenv("MDSPLUS_MODELS_PATH");
        ASSERT_TRUE(models_path && *models_path);
        const std::string original_ids_path =
            std::getenv("ids_path") ? std::getenv("ids_path") : "";

        const std::string new_ids_path = pulse_dir() + ";" + models_path;
        ASSERT_EQ(setenv("ids_path", new_ids_path.c_str(), 1), 0);

        try {
            MDSplus::Tree tree("ids", /*shot=*/1, "NORMAL");
            MDSplus::TreeNode* node = tree.getNode("VERSION:BACK_MAJOR");
            MDSplus::Int32* value = new MDSplus::Int32(mismatched_major);
            node->putData(value);
            MDSplus::deleteData(value);
            delete node;
        } catch (MDSplus::MdsException& exc) {
            FAIL() << "Could not poke VERSION:BACK_MAJOR directly via the "
                      "MDSplus tree API: "
                   << exc.what();
        }

        if (!original_ids_path.empty()) {
            setenv("ids_path", original_ids_path.c_str(), 1);
        } else {
            unsetenv("ids_path");
        }
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

// Mismatching case: after creation, VERSION:BACK_MAJOR is overwritten to a
// value the compiled backend (MDSPLUS_BACKEND_MAJOR == 1) can never match.
// Reopening through the C ABI must hit al_lowlevel.cpp's
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
    ASSERT_TRUE(status.message != nullptr);
    EXPECT_NE(std::string(status.message).find("Compatibility"),
              std::string::npos)
        << "status.message=" << status.message;
}

}  // namespace

#endif  // AL_CONTRACT_HAVE_MDSPLUS
