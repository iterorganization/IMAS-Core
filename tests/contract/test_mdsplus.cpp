// MDSplus tracer-bullet round trip (issue #13, TRACEABILITY.md Part 4 row 1).
//
// Proves the whole MDSplus characterization machine end-to-end before any
// behavioral breadth is attempted (TEST_STRATEGY.md §4, issue #12's
// vertical-slice-1 scoping note): one equilibrium scalar round-trips through
// the public C ABI against a real MDSplus tree baked from DD 4.1.1
// (AL_BUILD_MDSPLUS_MODELS=ON, see docker/mdsplus/). Reuses the real DD path
// and value from equilibrium_seed.h rather than inventing a new one, since
// MDSplus resolves paths against its model tree (issue #12 Q2) and cannot
// take the synthetic opaque paths RoundTrip/RoundTripMatrix use.
//
// Build-gated by AL_CONTRACT_HAVE_MDSPLUS (defined in CMakeLists.txt only
// when AL_BACKEND_MDSPLUS=ON) so the file compiles out entirely otherwise;
// runtime-skipped (GTEST_SKIP()) when MDSPLUS_MODELS_PATH is unset, per
// TEST_STRATEGY D4 — absent/skipped, never failed.
#ifdef AL_CONTRACT_HAVE_MDSPLUS

#include "al_contract.h"
#include "equilibrium_seed.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <string>
#include <vector>

using al_contract::PulseId;

namespace {

class MdsplusTracerBullet : public ::testing::Test {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_MDSPLUS_UNCONFIGURED(); }

    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/13,
                   /*run=*/0};
};

TEST_F(MdsplusTracerBullet, ScalarSurvivesWriteThenRead) {
    base_.make_legacy_tree(pulse_);

    const std::string uri =
        al_contract::build_uri(MDSPLUS_BACKEND, base_.str(), pulse_);
    ASSERT_FALSE(uri.empty());

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE,
                                           &pulse_ctx));

    const double kValue = equilibrium_seed::scalar_r0();

    // --- write ---
    {
        int op_ctx = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds,
                                            "", WRITE_OP, &op_ctx));
        AL_EXPECT_OK(al_contract::write_data<double>(
            op_ctx, equilibrium_seed::kScalar, {}, {kValue}));
        AL_ASSERT_OK(al_end_action(op_ctx));
    }

    // --- read back ---
    {
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
    }

    EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
}

}  // namespace

#endif  // AL_CONTRACT_HAVE_MDSPLUS
