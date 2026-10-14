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
#include <functional>
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
        const std::string hdf5_uri = al_contract::hdf5_uri_for(pulse_dir);
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
        const std::string uda_uri = al_contract::uda_hdf5_uri_for(pulse_dir);
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
// composite shape (static scalar + top-level timebase array + a real
// time_slice/profiles_1d/constraints AOS, tests/contract/equilibrium_seed.h —
// issues #4/#33/D5), reusing the seed's own generator and structural hash
// oracle unchanged: seed via HDF5 (fixture setup, same role the baked model
// tree plays for MDSplus), reopen the identical path through the UDA backend in
// remote mode, and check what UDA reads back.
//
// Empirically (against the docker/uda/ reference stack): the seed is now
// DD-4.1.1-valid (issue #33), so this is no longer a fixture-shape divergence —
// it splits by shape class. The top-level real DD paths (`vacuum_toroidal_field
// /r0` scalar, the `time` timebase array) round-trip cleanly. But the seed's
// dynamic leaves nested inside the `time_slice` struct_array come back absent
// through UDA remote mode + backend=hdf5 — the independently-pinned
// UdaAosKnownDefects defect (test_uda_breadth.cpp), the same one
// UdaRealPathMatrixKnownDefects pins for COMPLEX cells. So the full structural
// hash cannot match. That is a genuine UDA xfail, pinned per D2 below
// (DISABLED_FullSeedRoundTripsThroughUda correct-contract +
// TopLevelRoundTripsButAosLeavesReadAbsent active tripwire), NOT fixture
// invalidity: the identical seed round-trips fully on HDF5/Memory/ASCII/MDSplus
// (EquilibriumSeedMatrix). See TRACEABILITY.md Part 5.
class UdaEquilibriumSeedParity : public ::testing::Test {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_UDA_UNCONFIGURED(); }

    al_contract::TempBase base_;
};

// Seed the full DD-valid composite through the plain HDF5 backend, then hand a
// UDA remote-mode read context for `pulse_dir` to `check`.
void seed_hdf5_then_open_uda(al_contract::TempBase& base,
                             const std::function<void(int)>& check) {
    const std::string pulse_dir = base.str() + "/pulse";
    std::error_code ec;
    std::filesystem::create_directories(pulse_dir, ec);

    {
        const std::string hdf5_uri = al_contract::hdf5_uri_for(pulse_dir);
        int pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(hdf5_uri.c_str(),
                                               FORCE_CREATE_PULSE, &pulse_ctx));
        // The UDA client's cache_mode=none read path always resolves
        // ids_properties/homogeneous_time first and throws if absent. Written in
        // its own GLOBAL op ahead of the seed so equilibrium_seed::write() stays
        // unmodified.
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

    const std::string uda_uri = al_contract::uda_hdf5_uri_for(pulse_dir);
    int pulse_ctx = -1;
    AL_ASSERT_OK(
        al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE, &pulse_ctx));
    check(pulse_ctx);
    EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
}

// Read just the two top-level fields (static scalar + timebase array) in their
// own READ op and confirm they round-trip through UDA. These are real DD paths
// NOT nested in a struct_array, so they are unaffected by the dynamic-leaf-
// inside-AOS defect below.
void expect_top_level_roundtrips(int uda_ctx) {
    int op = -1;
    AL_ASSERT_OK(
        al_begin_global_action(uda_ctx, equilibrium_seed::kIds, "", READ_OP, &op));
    std::vector<int>    shape;
    std::vector<double> r0, time;
    AL_EXPECT_OK(al_contract::read_data<double>(op, equilibrium_seed::kScalar, 0,
                                                &shape, &r0));
    ASSERT_EQ(r0.size(), 1u);
    EXPECT_DOUBLE_EQ(r0[0], equilibrium_seed::scalar_r0());
    AL_EXPECT_OK(al_contract::read_data<double>(op, equilibrium_seed::kTimebase,
                                                1, &shape, &time));
    EXPECT_EQ(time, equilibrium_seed::timebase_values());
    AL_ASSERT_OK(al_end_action(op));
}

// Covered (issue #33): the whole DD-valid seed round-trips through UDA remote
// mode -- the static scalar, the top-level timebase array, AND the time_slice
// struct_array's dynamic leaves (time, profiles_1d/psi, global_quantities/ip,
// constraints/ip/{measured,weight}) -- reproducing the structural hash exactly.
//
// This does NOT contradict UdaAosKnownDefects (test_uda_breadth.cpp), which
// pins that a dynamic leaf inside a struct_array reads back absent through UDA:
// that fixture seeds time_slice with an *empty* AOS timebase and no per-element
// `time` leaf. This seed writes the AOS with its real DD timebasepath ("time")
// and a per-element `time`, and empirically UDA then resolves every nested
// dynamic leaf correctly -- a well-formed, DD-conformant time_slice AOS is the
// case that works. The structural hash (rank/extents/AOS-index-aware) is what
// makes this a trustworthy parity oracle rather than a values-only check.
TEST_F(UdaEquilibriumSeedParity, FullSeedRoundTripsThroughUda) {
    seed_hdf5_then_open_uda(base_, [](int uda_ctx) {
        expect_top_level_roundtrips(uda_ctx);

        std::vector<equilibrium_seed::Obs> obs;
        AL_ASSERT_OK(equilibrium_seed::read_back(uda_ctx, &obs));

        const std::vector<equilibrium_seed::Obs> expected =
            equilibrium_seed::expected_records();
        ASSERT_EQ(obs.size(), expected.size())
            << "UDA observed a different field/AOS-element count than the seed";
        for (std::size_t i = 0; i < expected.size(); ++i) {
            EXPECT_EQ(obs[i].field, expected[i].field) << "field #" << i;
            EXPECT_EQ(obs[i].rank, expected[i].rank)
                << expected[i].field << " rank";
            EXPECT_EQ(obs[i].extents, expected[i].extents)
                << expected[i].field << " extents";
        }
        EXPECT_EQ(equilibrium_seed::canonical_hash(obs),
                  equilibrium_seed::expected_hash())
            << "the DD-valid equilibrium seed must round-trip through UDA "
               "remote mode, structure and values";
    });
}

}  // namespace

#endif  // AL_CONTRACT_HAVE_UDA
