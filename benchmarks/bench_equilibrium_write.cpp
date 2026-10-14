// The write-side counterpart to bench_equilibrium_read.cpp (issue #59, PRD
// #57 user stories 2-5): whole-IDS write of the reference equilibrium
// fixture, driven strictly at the public C ABI (al_lowlevel.h), reusing
// equilibrium_seed.h verbatim and bench_common.h's shared per-backend
// pulse-open helper (issue #58).
//
// Timed region mirrors the read benchmark's discipline exactly: the pulse is
// opened once (FORCE_CREATE_PULSE) and closed once, both untimed; only the
// repeated equilibrium_seed::write call sits inside the
// `for (auto _ : state)` loop, so open/close cost never contaminates the
// write measurement.
//
// Correctness of repeated writes on the same open pulse is pinned separately
// by EquilibriumSeedMatrix.RepeatedWholeIdsWriteOnSameOpenPulseSucceeds; this
// benchmark measures the operation without re-asserting the contract.

#include "bench_common.h"
#include "equilibrium_seed.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <benchmark/benchmark.h>

using al_bench::BenchBackend;
using al_bench::CheckOk;

void BM_EquilibriumWholeIdsWrite(benchmark::State& state, BenchBackend backend) {
  // pulse=2 (distinct from bench_equilibrium_read.cpp's pulse=1) so the two
  // benchmarks' Memory-backend entries never share an identity within the
  // same al_benchmarks process.
  const al_contract::PulseId pulse{/*database=*/"bench", /*version=*/"3",
                                    /*pulse=*/2, /*run=*/0};

  // --- untimed setup: open once ----------------------------------------------
  al_bench::BenchPulse bp = al_bench::OpenBenchPulse(backend, pulse);

  // --- timed region: repeated whole-IDS write on the same open context -----
  for (auto _ : state) {
    CheckOk(equilibrium_seed::write(bp.pulse_ctx), "equilibrium_seed::write");
  }

  // --- untimed teardown ------------------------------------------------------
  CheckOk(al_close_pulse(bp.pulse_ctx, CLOSE_PULSE), "al_close_pulse");
  // bp.base's destructor (HDF5 only) removes the temp directory on every exit path.
}
// Repetitions(N) makes Google Benchmark report the distribution (median,
// mean, stddev, iteration count) across N independent setup/write/teardown
// cycles, not a bare single-shot mean (issue #59, PRD #57 user story 10),
// mirroring bench_equilibrium_read.cpp.
BENCHMARK_CAPTURE(BM_EquilibriumWholeIdsWrite, HDF5, BenchBackend::kHdf5)
    ->Repetitions(20);
BENCHMARK_CAPTURE(BM_EquilibriumWholeIdsWrite, Memory, BenchBackend::kMemory)
    ->Repetitions(20);
