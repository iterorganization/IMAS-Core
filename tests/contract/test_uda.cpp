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

// -----------------------------------------------------------------------------
// Read-only parity fixture (issue #24, TRACEABILITY.md Part 5 row 2).
//
// Broadens the tracer bullet above from one scalar to the full equilibrium-seed
// composite shape (scalar + timebase-carrying 2-D array + constraints AOS,
// tests/contract/equilibrium_seed.h — issue #4/D5), reusing the seed's own
// generator and FNV-1a hash oracle unchanged: seed via HDF5 (fixture setup,
// same role the baked model tree plays for MDSplus), reopen the identical path
// through the UDA backend in remote mode, recompute the hash over what UDA
// reads back, and assert it matches the seed's own expected_hash().
//
// Empirically (against the docker/uda/ reference stack): the composite shape
// is a **divergence**, not a defect -- the same wall MDSplus hits against its
// baked model tree (TRACEABILITY.md Part 4's EquilibriumSeedMatrix/Mdsplus
// row). The scalar sub-shape alone is real DD (`vacuum_toroidal_field/r0`,
// already proven read-only-parity by UdaSmokeRoundTrip above), but the
// generator's flat `profiles_1d/psi` leaf and generic `constraints` AOS have
// no counterpart in equilibrium's real DD-4.1.1 layout (`profiles_1d` is
// itself a struct_array, not a plain field): UDA's remote get() resolves
// every path against the DD schema it loads at startup (src/uda/uda_xml.cpp),
// so `al_plugin_read_data` fails immediately on `profiles_1d/psi` with
// "cannot find node equilibrium/profiles_1d/psi in data dictionary
// (profiles_1d not found)". There is nothing to fix -- the fixture's
// synthetic composite shape simply cannot transfer to a DD-schema-validating
// backend. Unlike EquilibriumSeedMatrix's per-backend GTEST_SKIP() (that
// fixture's body genuinely runs for its other, non-MDSplus parameters), this
// suite has no other instance to keep the seed/read machinery exercised, so
// the test runs the real seed-then-reopen attempt every time, asserts the
// *specific* known failure signature, and only then GTEST_SKIP()s -- if the
// divergence ever disappears (e.g. the UDA backend gains partial-path
// resolution), the assertion below fails loudly instead of this test quietly
// staying "skipped" over a silently-changed reality.
class UdaEquilibriumSeedParity : public ::testing::Test {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_UDA_UNCONFIGURED(); }

    al_contract::TempBase base_;
};

TEST_F(UdaEquilibriumSeedParity, HdfSeededReadsBackThroughUda) {
    const std::string pulse_dir = base_.str() + "/pulse";
    std::error_code ec;
    std::filesystem::create_directories(pulse_dir, ec);

    // --- seed the full composite shape through the plain HDF5 backend ---
    {
        const std::string hdf5_uri = "imas:hdf5?path=" + pulse_dir;
        int pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(hdf5_uri.c_str(),
                                               FORCE_CREATE_PULSE, &pulse_ctx));

        // Same precondition as the scalar tracer bullet above: the UDA client's
        // cache_mode=none read path always resolves ids_properties/homogeneous_time
        // first and throws if it is absent. Written in its own GLOBAL op ahead of
        // the seed so equilibrium_seed::write() itself stays unmodified.
        {
            int op_ctx = -1;
            AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds,
                                                "", WRITE_OP, &op_ctx));
            AL_EXPECT_OK(al_contract::write_data<int>(
                op_ctx, "ids_properties/homogeneous_time", {}, {1}));
            AL_ASSERT_OK(al_end_action(op_ctx));
        }

        AL_ASSERT_OK(equilibrium_seed::write(pulse_ctx));
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    // --- read the same composite shape back through UDA, remote mode ---
    {
        const std::string uda_uri = al_contract::uda_uri_base() +
                                    "?backend=hdf5&cache_mode=none&path=" +
                                    pulse_dir;
        int pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE,
                                               &pulse_ctx));

        uint64_t hash = 0;
        al_status_t s = equilibrium_seed::read_and_hash(pulse_ctx, &hash);

        // Divergence, not a defect -- confirm the read fails with the exact
        // known signature (DD-schema resolution rejecting profiles_1d/psi as
        // a plain leaf) rather than assuming it and skipping unconditionally.
        // A fatal failure here means the divergence no longer holds: someone
        // must update this test (and TRACEABILITY.md Part 5) to reflect the
        // new behavior, rather than the change going unnoticed.
        ASSERT_NE(s.code, 0)
            << "equilibrium_seed::read_and_hash unexpectedly succeeded through "
               "UDA -- the profiles_1d/psi divergence documented in "
               "TRACEABILITY.md Part 5 no longer holds; update this test "
               "(and the traceability row) to match the new behavior.";
        EXPECT_NE(std::string(s.message).find("profiles_1d"), std::string::npos)
            << "expected the documented profiles_1d DD-resolution failure, "
               "got a different error instead: " << s.message;

        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);

        GTEST_SKIP() << "divergence, not a defect: UDA resolves every remote "
                        "read path against the DD schema it loads at startup, "
                        "and the generator's flat profiles_1d/psi leaf + "
                        "generic constraints AOS have no counterpart in "
                        "equilibrium's real DD-4.1.1 layout (profiles_1d is "
                        "itself a struct_array) -- confirmed above via the "
                        "specific failure message. Same wall MDSplus hits "
                        "(TRACEABILITY.md Part 4's EquilibriumSeedMatrix/"
                        "Mdsplus row); the scalar sub-shape alone is real DD "
                        "and is already covered read-only-parity by "
                        "UdaSmokeRoundTrip above. See TRACEABILITY.md Part 5.";
    }
}

}  // namespace

#endif  // AL_CONTRACT_HAVE_UDA
