// Throwaway proof (issue #51 acceptance criterion 3): a minimal translation
// unit living outside tests/ that includes equilibrium_seed.h, builds, and
// exercises it against a real backend through the public C ABI only -- no
// GoogleTest, no AL_BUILD_TESTS. Demonstrates that the fixture extracted by
// this issue is genuinely linkable from a target like the future
// benchmarks/ tree (PRD #50), not just from tests/contract/.
//
// Uses the always-on, zero-I/O Memory backend so this needs no on-disk setup
// and no optional backend enabled.

#include "equilibrium_seed.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

int main() {
    // The Memory backend stores nothing on disk, but URI construction still
    // resolves the legacy "user" field as a base directory, so it must exist
    // (mirrors tests/contract/al_contract.h's TempBase for the on-disk cases).
    const std::filesystem::path base =
        std::filesystem::temp_directory_path() / "al_contract_fixtures_smoke";
    std::filesystem::create_directories(base);
    const std::string base_str = base.string();

    char* uri = nullptr;
    al_status_t s = al_build_uri_from_legacy_parameters(
        MEMORY_BACKEND, /*pulse=*/1, /*run=*/0, /*user=*/base_str.c_str(),
        /*tokamak=*/"test", /*version=*/"3", /*options=*/"", &uri);
    if (s.code != 0) {
        std::fprintf(stderr, "al_build_uri_from_legacy_parameters failed: %s\n",
                     s.message);
        return 1;
    }

    int pulse_ctx = -1;
    s = al_begin_dataentry_action(uri, FORCE_CREATE_PULSE, &pulse_ctx);
    free(uri);
    if (s.code != 0) {
        std::fprintf(stderr, "al_begin_dataentry_action failed: %s\n", s.message);
        return 1;
    }

    s = equilibrium_seed::write(pulse_ctx);
    if (s.code != 0) {
        std::fprintf(stderr, "equilibrium_seed::write failed: %s\n", s.message);
        return 1;
    }

    uint64_t observed_hash = 0;
    s = equilibrium_seed::read_and_hash(pulse_ctx, &observed_hash);
    if (s.code != 0) {
        std::fprintf(stderr, "equilibrium_seed::read_and_hash failed: %s\n",
                     s.message);
        return 1;
    }

    al_close_pulse(pulse_ctx, CLOSE_PULSE);

    if (observed_hash != equilibrium_seed::expected_hash()) {
        std::fprintf(stderr,
                      "structural hash mismatch: observed=%llu expected=%llu\n",
                      static_cast<unsigned long long>(observed_hash),
                      static_cast<unsigned long long>(
                          equilibrium_seed::expected_hash()));
        return 1;
    }

    std::printf("equilibrium seed round-tripped through the Memory backend "
                "outside the test tree: hash=%llu\n",
                static_cast<unsigned long long>(observed_hash));
    return 0;
}
