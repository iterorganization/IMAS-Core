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

using al_bench::BenchBackend;
using al_bench::CheckOk;

void BM_EquilibriumWholeIdsRead(benchmark::State& state, BenchBackend backend) {
  const al_contract::PulseId pulse{/*database=*/"bench", /*version=*/"3",
                                    /*pulse=*/1, /*run=*/0};

  // --- untimed setup: open once, write the reference equilibrium once ------
  // Memory keeps the fixed pulse identity across repetitions unlike
  // bench_write_read.cpp's per-iteration bump: MemoryBackend::closePulse is a
  // no-op (src/memory_backend.h), so correctness across the 20 Repetitions
  // rests on FORCE_CREATE_PULSE clearing the reused entry's idsMap on reopen,
  // not on pulse identities never colliding.
  al_bench::BenchPulse bp = al_bench::OpenBenchPulse(backend, pulse);
  CheckOk(equilibrium_seed::write(bp.pulse_ctx), "equilibrium_seed::write");

  // --- timed region: begin_global_action(READ) -> every leaf -> end_action -
  std::vector<equilibrium_seed::Obs> records;
  for (auto _ : state) {
    CheckOk(equilibrium_seed::read_back(bp.pulse_ctx, &records),
            "equilibrium_seed::read_back");
    benchmark::DoNotOptimize(records);
  }

  // --- untimed teardown ------------------------------------------------------
  CheckOk(al_close_pulse(bp.pulse_ctx, CLOSE_PULSE), "al_close_pulse");
  // bp.base's destructor (HDF5 only) removes the temp directory on every exit path.
}
// Repetitions(N) makes Google Benchmark report the distribution (median,
// mean, stddev, iteration count) across N independent setup/read/teardown
// cycles, not a bare single-shot mean (issue #53, PRD #50 user story 8).
BENCHMARK_CAPTURE(BM_EquilibriumWholeIdsRead, HDF5, BenchBackend::kHdf5)
    ->Repetitions(20);
BENCHMARK_CAPTURE(BM_EquilibriumWholeIdsRead, Memory, BenchBackend::kMemory)
    ->Repetitions(20);
