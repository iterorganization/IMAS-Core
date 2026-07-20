// The headline §7 P1 metric (NORTH_STAR.md §8.4 "whole-IDS get"): a whole-IDS
// read of the realistic equilibrium fixture, driven strictly at the public C
// ABI (al_lowlevel.h). Reuses equilibrium_seed.h verbatim (issue #51) as the
// workload -- no bespoke fixture -- so the benchmarked shape never drifts
// from what the contract suite already pins.
//
// Timed region = one operation (issue #53, PRD #50 user story 6): the
// reference equilibrium is written and the data entry opened once in untimed
// setup; only equilibrium_seed::read_back's begin_global_action(READ) ->
// traverse every leaf -> end_action is inside the `for (auto _ : state)` loop.
//
// Backend is a sub-axis of the whole-IDS-read operation (PRD #50 "structure:
// scenario-primary, backend as a sub-axis"): BM_EquilibriumWholeIdsRead below
// is the one operation body, and HDF5 (issue #53) / Memory (issue #54) are
// two BENCHMARK_CAPTURE registrations of it, differing only in which backend
// id the URI is built for. Memory is a zero-I/O control -- it never touches
// disk -- so subtracting its number from HDF5's decomposes the total into
// storage cost vs per-node traversal/CPU cost.

#include "bench_common.h"
#include "al_contract_legacy_storage.h"
#include "equilibrium_seed.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <benchmark/benchmark.h>

#include <cstdlib>
#include <optional>
#include <string>

namespace {

using al_bench::CheckOk;

enum class BenchBackend { kHdf5, kMemory };

// HDF5 resolves FORCE_CREATE_PULSE against a real on-disk legacy tree, so it
// needs `base` alive for the URI to mean anything. Memory never touches disk
// (bench_write_read.cpp's MemoryUri established the pattern): an arbitrary
// absolute-looking `user` sidesteps the legacy URI builder's getpwnam()
// fallback for an empty/unknown user, without paying for a real temp dir.
std::string BuildUri(BenchBackend backend, const al_contract::PulseId& pulse,
                     const al_contract::LegacyPulseDirectory* base) {
  char* uri = nullptr;
  if (backend == BenchBackend::kHdf5) {
    CheckOk(al_build_uri_from_legacy_parameters(
                HDF5_BACKEND, pulse.pulse, pulse.run, base->str().c_str(),
                pulse.database.c_str(), pulse.version.c_str(),
                /*options=*/"", &uri),
            "al_build_uri_from_legacy_parameters");
  } else {
    CheckOk(al_build_uri_from_legacy_parameters(
                MEMORY_BACKEND, pulse.pulse, pulse.run, "/al-benchmarks",
                pulse.database.c_str(), pulse.version.c_str(),
                /*options=*/"", &uri),
            "al_build_uri_from_legacy_parameters");
  }
  std::string result(uri ? uri : "");
  free(uri);
  return result;
}

}  // namespace

void BM_EquilibriumWholeIdsRead(benchmark::State& state, BenchBackend backend) {
  const al_contract::PulseId pulse{/*database=*/"bench", /*version=*/"3",
                                    /*pulse=*/1, /*run=*/0};
  std::optional<al_contract::LegacyPulseDirectory> base;
  if (backend == BenchBackend::kHdf5) {
    base.emplace();
    base->make_legacy_tree(pulse);
  }
  const std::string uri = BuildUri(backend, pulse, base ? &*base : nullptr);

  // --- untimed setup: open once, write the reference equilibrium once ------
  // Memory keeps the fixed pulse identity across repetitions unlike
  // bench_write_read.cpp's per-iteration bump: MemoryBackend::closePulse is a
  // no-op (src/memory_backend.h), so correctness across the 20 Repetitions
  // rests on FORCE_CREATE_PULSE clearing the reused entry's idsMap on reopen,
  // not on pulse identities never colliding.
  int pulse_ctx = -1;
  CheckOk(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pulse_ctx),
          "al_begin_dataentry_action");
  CheckOk(equilibrium_seed::write(pulse_ctx), "equilibrium_seed::write");

  // --- timed region: begin_global_action(READ) -> every leaf -> end_action -
  std::vector<equilibrium_seed::Obs> records;
  for (auto _ : state) {
    CheckOk(equilibrium_seed::read_back(pulse_ctx, &records),
            "equilibrium_seed::read_back");
    benchmark::DoNotOptimize(records);
  }

  // --- untimed teardown ------------------------------------------------------
  CheckOk(al_close_pulse(pulse_ctx, CLOSE_PULSE), "al_close_pulse");
  // base's destructor (HDF5 only) removes the temp directory on every exit path.
}
// Repetitions(N) makes Google Benchmark report the distribution (median,
// mean, stddev, iteration count) across N independent setup/read/teardown
// cycles, not a bare single-shot mean (issue #53, PRD #50 user story 8).
BENCHMARK_CAPTURE(BM_EquilibriumWholeIdsRead, HDF5, BenchBackend::kHdf5)
    ->Repetitions(20);
BENCHMARK_CAPTURE(BM_EquilibriumWholeIdsRead, Memory, BenchBackend::kMemory)
    ->Repetitions(20);
