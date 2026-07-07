// Round-trip contract tests — the scaffold-slice behavior proof.
//
// TEST_STRATEGY.md §4 step 1: one write->read round trip on an on-disk backend
// (HDF5) *and* an in-RAM backend (Memory), proving the machine end-to-end
// before the matrix is broadened. Oracle = round-trip self-consistency
// (read-back equals write), which needs zero DD artifacts (decision D5).
//
// This is intended-contract territory (decision D2): the ABI genuinely stores
// and returns what it was given, so we assert it directly. The suffix on each
// ctest name is the backend label (HDF5 / Memory).

#include "al_contract.h"

#include <gtest/gtest.h>

#include <string>

using al_contract::BackendCase;
using al_contract::PulseId;

namespace {

// A magnetics IDS is used purely as an opaque container of two leaf paths; the
// core attaches no DD semantics to it (Key architecture fact: DD paths are
// opaque strings through the whole C ABI).
constexpr const char* kIds = "magnetics";
constexpr const char* kIntField = "ids_properties/homogeneous_time";
constexpr const char* kStrField = "ids_properties/comment";

class RoundTrip : public ::testing::TestWithParam<BackendCase> {
protected:
    al_contract::TempBase base_;
    // A single fixed pulse address; every case gets its own base_ dir, so the
    // on-disk trees can never alias even though the address is shared.
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/12,
                   /*run=*/0};
};

// Write an INTEGER scalar and a CHAR array, then read both back through a fresh
// READ operation on the same data entry, and assert exact equality.
TEST_P(RoundTrip, IntScalarAndCharArraySurviveWriteThenRead) {
    const BackendCase backend = GetParam();
    if (backend.on_disk) {
        base_.make_legacy_tree(pulse_);
    }

    const std::string uri =
        al_contract::build_uri(backend.id, base_.str(), pulse_);
    ASSERT_FALSE(uri.empty());

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE,
                                           &pulse_ctx));

    const int kIntValue = 1;
    const std::string kStrValue = "round-trip";

    // --- write ---
    {
        int op_ctx = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, kIds, "", WRITE_OP,
                                            &op_ctx));
        AL_EXPECT_OK(al_contract::write_int_scalar(op_ctx, kIntField,
                                                   kIntValue));
        AL_EXPECT_OK(al_contract::write_char_array(op_ctx, kStrField,
                                                   kStrValue));
        AL_ASSERT_OK(al_end_action(op_ctx));
    }

    // --- read back ---
    {
        int op_ctx = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, kIds, "", READ_OP,
                                            &op_ctx));

        int read_int = -999;
        AL_EXPECT_OK(al_contract::read_int_scalar(op_ctx, kIntField,
                                                  &read_int));
        EXPECT_EQ(read_int, kIntValue);

        std::string read_str;
        AL_EXPECT_OK(al_contract::read_char_array(op_ctx, kStrField,
                                                  &read_str));
        EXPECT_EQ(read_str, kStrValue);

        AL_ASSERT_OK(al_end_action(op_ctx));
    }

    EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
}

INSTANTIATE_TEST_SUITE_P(
    Backends, RoundTrip,
    ::testing::Values(BackendCase{HDF5_BACKEND, "HDF5", /*on_disk=*/true},
                      BackendCase{MEMORY_BACKEND, "Memory", /*on_disk=*/false}),
    [](const ::testing::TestParamInfo<BackendCase>& info) {
        return std::string(info.param.name);
    });

}  // namespace
