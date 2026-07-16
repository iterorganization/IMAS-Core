// HDF5 stored-backend-version drift check (issue #36, TRACEABILITY.md Part 2
// row B).
//
// al_begin_dataentry_action (src/al_lowlevel.cpp) compares the backend's
// compiled version (`getVersion(NULL)`, HDF5: {1,0}) against the version
// stored in the pulse being opened (`getVersion(pctx)`, HDF5: the
// HDF5_BACKEND_VERSION attribute of master.h5) on OPEN_PULSE/FORCE_OPEN_PULSE
// and throws LOWLEVEL_ERR on a mismatch. Part 2 row B previously declared
// this reachable check a terminal `gap` because forcing a mismatch needs
// out-of-band HDF5 attribute surgery; that surgery now lives in the isolated
// fixture producer hdf5_fixture_tool.cpp (same producer/observer split as the
// MDSplus drift tier), so this file asserts the contract exclusively through
// the public C ABI — no HDF5 include or link enters contract_tests.
//
// Empirically characterized: for HDF5 the generic LOWLEVEL_ERR drift branch
// is SHADOWED — HDF5Backend::openPulse constructs its reader/writer through
// HDF5BackendFactory, which accepts exactly "1.0"
// (src/hdf5/hdf5_backend_factory.cpp) and throws for any other stored value,
// so an incompatible pulse is refused with BACKEND_ERR ("No backend writer
// with version: <stored>") BEFORE al_lowlevel.cpp's comparison ever runs.
// Every stored version other than the compiled one is thereby rejected —
// stricter than the generic check (which would tolerate an older minor).
// These pins assert that exact current refusal.
#ifdef AL_CONTRACT_HDF5_FIXTURE_TOOL

#include "al_contract.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

namespace {

class Hdf5VersionDrift : public ::testing::Test {
protected:
    // Creates a genuine pulse through the public C ABI and returns its
    // directory (the URI "path" query parameter).
    std::string create_pulse() {
        const std::string dir = base_.str() + "/pulse";
        const std::string uri = "imas:hdf5?path=" + dir;
        int ctx = -1;
        EXPECT_EQ(
            al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &ctx)
                .code,
            0);
        EXPECT_EQ(al_close_pulse(ctx, CLOSE_PULSE).code, 0);
        return dir;
    }

    // Out-of-band fixture mutation: rewrite the pulse's stored
    // HDF5_BACKEND_VERSION via the isolated setup tool. Fixture producer
    // only — nothing asserted here is part of the contract.
    void set_stored_version(const std::string& dir,
                            const std::string& version) {
        const std::string cmd = std::string("\"") +
                                AL_CONTRACT_HDF5_FIXTURE_TOOL + "\" \"" + dir +
                                "\" \"" + version + "\"";
        ASSERT_EQ(std::system(cmd.c_str()), 0)
            << "fixture tool failed: " << cmd;
    }

    al_status_t open_pulse(const std::string& dir, int* ctx) {
        const std::string uri = "imas:hdf5?path=" + dir;
        return al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, ctx);
    }

    al_contract::TempBase base_;
};

// Baseline: a pulse created and reopened by the same compiled backend carries
// a matching stored version, so the drift check is a no-op and OPEN succeeds.
TEST_F(Hdf5VersionDrift, MatchingStoredVersionOpensCleanly) {
    const std::string dir = create_pulse();
    int ctx = -1;
    al_status_t s = open_pulse(dir, &ctx);
    EXPECT_EQ(s.code, 0) << "message: " << s.message;
    if (s.code == 0) {
        EXPECT_EQ(al_close_pulse(ctx, CLOSE_PULSE).code, 0);
    }
}

// A stored major the compiled backend (1.x) can never match is refused —
// never a silent open of incompatible data. Exact observed status: the
// factory's BACKEND_ERR, which fires before the generic LOWLEVEL_ERR drift
// comparison (see the header note).
TEST_F(Hdf5VersionDrift, HigherStoredMajorRefusesOpen) {
    const std::string dir = create_pulse();
    set_stored_version(dir, "999.0");
    int ctx = -1;
    al_status_t s = open_pulse(dir, &ctx);
    EXPECT_EQ(s.code, alerror::backend_err) << "message: " << s.message;
    EXPECT_NE(std::string(s.message).find("No backend writer with version"),
              std::string::npos)
        << "message: " << s.message;
    EXPECT_NE(std::string(s.message).find("999.0"), std::string::npos)
        << "the refusal must name the stored version; message: " << s.message;
}

// The minor comparison boundary: a stored minor NEWER than the compiled one
// is likewise refused (the factory accepts exactly "1.0", so this is refused
// by the same shadowing BACKEND_ERR, not the generic minor comparison).
TEST_F(Hdf5VersionDrift, NewerStoredMinorRefusesOpen) {
    const std::string dir = create_pulse();
    set_stored_version(dir, "1.999");
    int ctx = -1;
    al_status_t s = open_pulse(dir, &ctx);
    EXPECT_EQ(s.code, alerror::backend_err) << "message: " << s.message;
    EXPECT_NE(std::string(s.message).find("1.999"), std::string::npos)
        << "message: " << s.message;
}

}  // namespace

#endif  // AL_CONTRACT_HDF5_FIXTURE_TOOL
