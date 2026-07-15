// UDA smoke round trip (issue #23, feasibility spike / tracer bullet for #21).
//
// Proves the whole UDA characterization machine end-to-end before any
// behavioral breadth is attempted (the PRD's subtask-1 gate): one equilibrium
// scalar, seeded through the plain HDF5 backend, reads back byte-identical
// through the UDA backend in *remote mode* -- i.e. across the wire, through a
// real UDA server running the `IMAS` server plugin (linked against a server-side
// IMAS-Core), addressed by the public C ABI. This is the UDA analogue of
// test_mdsplus.cpp's tracer bullet, but read-only: UDA has no remote write path
// (PRD Solution), so parity arrives via a seed-then-reopen fixture rather than a
// write->read round trip. Seeding via HDF5 is fixture setup -- it plays the role
// the baked model tree plays for MDSplus; the subject under test is always the
// IMAS-Core UDA client.
//
// The whole reference stack (server + `IMAS` plugin + IMAS-Core + IDSDef.xml
// 4.1.1) runs in one container; docker/uda/run.sh builds it and drives this
// suite. See docker/uda/README.md.
//
// Build-gated by AL_CONTRACT_HAVE_UDA (defined in CMakeLists.txt only when a UDA
// backend is built -- AL_BACKEND_UDA or AL_BACKEND_UDAFAT) so the file compiles
// out entirely otherwise; runtime-skipped (GTEST_SKIP()) when UDA_HOST is unset,
// per TEST_STRATEGY D4 -- absent/skipped, never failed.
//
// Remoteness assumption: run.sh does not set IMAS_LOCAL_HOSTS, so the
// imas://localhost/uda authority is NOT short-circuited to a local backend by
// checkUriHost (src/al_context.cpp) -- the read genuinely traverses the wire to
// uda_server. Do not set IMAS_LOCAL_HOSTS in the reference container.
#ifdef AL_CONTRACT_HAVE_UDA

#include "al_contract.h"
#include "equilibrium_seed.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

namespace {

class UdaSmokeRoundTrip : public ::testing::Test {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_UDA_UNCONFIGURED(); }

    al_contract::TempBase base_;
};

TEST_F(UdaSmokeRoundTrip, ScalarSeededViaHdf5ReadsBackThroughUda) {
    // The pulse directory both backends address: the HDF5 backend writes
    // master.h5 (+ per-IDS files) here (src/hdf5/hdf5_utils.cpp getPulseFilePath
    // reads the URI's path= query verbatim), and the UDA client forwards the
    // same path= to the server-side HDF5 open. Must live under an allowed path
    // the UDA server will read (docker/uda/run.sh sets UDA_ALLOWED_PATHS).
    const std::string pulse_dir = base_.str() + "/pulse";
    std::error_code ec;
    std::filesystem::create_directories(pulse_dir, ec);

    const double kValue = equilibrium_seed::scalar_r0();

    // --- seed one scalar through the plain HDF5 backend (fixture setup) ---
    {
        const std::string hdf5_uri = "imas:hdf5?path=" + pulse_dir;
        int pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(hdf5_uri.c_str(),
                                               FORCE_CREATE_PULSE, &pulse_ctx));

        int op_ctx = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds,
                                            "", WRITE_OP, &op_ctx));
        // The UDA client's read path (readData, cache_mode=none) always resolves
        // ids_properties/homogeneous_time first (get_homogeneous_flag), and
        // throws — uncaught — if it is absent. Seed it so the tracer bullet
        // exercises the r0 read, not that precondition. 1 == HOMOGENEOUS_TIME.
        AL_EXPECT_OK(al_contract::write_data<int>(
            op_ctx, "ids_properties/homogeneous_time", {}, {1}));
        AL_EXPECT_OK(al_contract::write_data<double>(
            op_ctx, equilibrium_seed::kScalar, {}, {kValue}));
        AL_ASSERT_OK(al_end_action(op_ctx));

        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    // --- read it back through the UDA backend, remote mode, across the wire ---
    {
        // cache_mode=none takes readData's direct per-field IMAS::get path --
        // the simplest remote read, sufficient for the tracer bullet. backend=
        // hdf5 tells the server which local backend to open behind the plugin.
        const std::string uda_uri = al_contract::uda_uri_base() +
                                    "?backend=hdf5&cache_mode=none&path=" +
                                    pulse_dir;
        int pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE,
                                               &pulse_ctx));

        int op_ctx = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds,
                                            "", READ_OP, &op_ctx));

        std::vector<int> shape;
        std::vector<double> value;
        AL_EXPECT_OK(al_contract::read_data<double>(
            op_ctx, equilibrium_seed::kScalar, 0, &shape, &value));
        ASSERT_EQ(value.size(), 1u);
        EXPECT_EQ(value.at(0), kValue);

        AL_ASSERT_OK(al_end_action(op_ctx));
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }
}

}  // namespace

#endif  // AL_CONTRACT_HAVE_UDA
