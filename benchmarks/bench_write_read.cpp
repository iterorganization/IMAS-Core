// This is the vertical tracer bullet through the whole build+run+report path:
// one benchmark, driving strictly the public C ABI (al_lowlevel.h), against a
// tiny inline fixture -- deliberately NOT the equilibrium seed yet, so this
// benchmark stays independent of that fixture prefactor. It exists to prove
// the harness itself (FetchContent pin, CTest smoke wiring, JSON output),
// not to produce a meaningful performance number; the real hot-path scenarios
// (whole-IDS read/write, AOS traversal, ...) land separately (NORTH_STAR.md §8.4).

#include "bench_common.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <benchmark/benchmark.h>

#include <cstdlib>
#include <string>

namespace {

using al_bench::CheckOk;

// The core attaches no DD semantics to either string (DD paths are opaque
// strings through the whole C ABI), so an arbitrary IDS name and leaf path
// exercise the same code path a real IDS would.
constexpr const char* kIds   = "magnetics";
constexpr const char* kField = "ids_properties/homogeneous_time";
constexpr int         kValue = 42;

// Builds a fresh in-RAM Memory-backend URI. The Memory backend never touches
// disk, so the harness has no filesystem dependency to smoke-validate the
// build+run+report path -- but the legacy-parameter URI builder always
// resolves `user` into a filesystem path before the backend even sees it
// (al_context.cpp's DataEntryContext::parseUri), and an empty/unknown user
// falls through to a getpwnam() lookup that throws. An absolute-looking
// string (never actually opened for Memory) sidesteps that lookup, the same
// trick tests/contract/al_contract.h's TempBase uses for on-disk backends.
// `pulse` is bumped by the caller on every iteration so FORCE_CREATE_PULSE
// never collides with a still-registered in-process pulse from a prior one.
std::string MemoryUri(int pulse) {
  char* uri = nullptr;
  CheckOk(al_build_uri_from_legacy_parameters(
              MEMORY_BACKEND, pulse, /*run=*/0, /*user=*/"/al-benchmarks",
              /*tokamak=*/"bench", /*version=*/"3", /*options=*/"", &uri),
          "al_build_uri_from_legacy_parameters");
  std::string result(uri ? uri : "");
  free(uri);
  return result;
}

}  // namespace

// Single-leaf write-once-then-read: each iteration opens a fresh Memory-backend
// data entry, writes one INTEGER scalar, reads it back, and closes the pulse.
// The whole sequence is the timed region for this tracer-bullet benchmark; the
// finer open-once/timed-read-only split NORTH_STAR.md §8.4 calls for arrives
// with the real equilibrium read benchmark.
void BM_WriteReadRoundTrip(benchmark::State& state) {
  int pulse = 0;
  for (auto _ : state) {
    const std::string uri = MemoryUri(pulse++);

    int pulse_ctx = -1;
    CheckOk(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE,
                                       &pulse_ctx),
            "al_begin_dataentry_action");

    int write_ctx = -1;
    CheckOk(al_begin_global_action(pulse_ctx, kIds, "", WRITE_OP, &write_ctx),
            "al_begin_global_action(write)");
    int value = kValue;
    CheckOk(al_write_data(write_ctx, kField, "", &value, INTEGER_DATA, 0,
                           nullptr),
            "al_write_data");
    CheckOk(al_end_action(write_ctx), "al_end_action(write)");

    int read_ctx = -1;
    CheckOk(al_begin_global_action(pulse_ctx, kIds, "", READ_OP, &read_ctx),
            "al_begin_global_action(read)");
    int   read_value = 0;
    void* buf        = &read_value;
    int   size[MAXDIM] = {0};
    CheckOk(al_read_data(read_ctx, kField, "", &buf, INTEGER_DATA, 0, size),
            "al_read_data");
    CheckOk(al_end_action(read_ctx), "al_end_action(read)");
    benchmark::DoNotOptimize(read_value);

    CheckOk(al_close_pulse(pulse_ctx, CLOSE_PULSE), "al_close_pulse");
  }
}
BENCHMARK(BM_WriteReadRoundTrip);
