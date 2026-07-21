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
// Repeated writes to the same open pulse context must never accumulate stale
// data from a prior write (PRD #57 user story 5): ALData::writeData clears
// its buffer before repopulating (src/memory_backend.cpp) and the
// array-of-structures path replaces rather than appends elements, for any
// whole-IDS (non-slice) write. This is checked once, untimed, after the timed
// loop -- but the timed loop's own iteration count isn't a reliable way to
// exercise "repeated": the CTest smoke entry caps every benchmark at exactly
// one iteration (--benchmark_min_time=1x), which would make the check
// vacuous (one write trivially matches one write's hash) on the one run this
// repo executes automatically. So an explicit second untimed write happens
// after the loop, guaranteeing at least two writes to the same context
// regardless of how many timed iterations ran, before a read-back of the
// resulting structural hash is compared against equilibrium_seed's
// expected_hash() exactly, on both backends. The hash folds in AOS size
// (equilibrium_seed.h), so an accumulation bug (e.g. elements appended
// instead of replaced) would drift it and this check -- which the existing
// CTest smoke entry already runs -- would throw.

#include "bench_common.h"
#include "equilibrium_seed.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <benchmark/benchmark.h>

#include <cstdint>
#include <stdexcept>

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

  // --- untimed correctness check: no stale data survives repeated writes ---
  // Force a second write regardless of the timed loop's iteration count (see
  // top-of-file comment), then verify no staleness accumulated.
  CheckOk(equilibrium_seed::write(bp.pulse_ctx), "equilibrium_seed::write");
  uint64_t hash = 0;
  CheckOk(equilibrium_seed::read_and_hash(bp.pulse_ctx, &hash),
          "equilibrium_seed::read_and_hash");
  if (hash != equilibrium_seed::expected_hash()) {
    throw std::runtime_error(
        "BM_EquilibriumWholeIdsWrite: repeated-write structural hash does "
        "not match expected_hash() -- repeated writes may have accumulated "
        "stale data");
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
