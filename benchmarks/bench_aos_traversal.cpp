// AOS-heavy full-traversal benchmark (issue #61, PRD #57 user story 7): the
// "before" baseline for P1/P4's round-trip-count reduction (NORTH_STAR.md
// §7) -- and the macro signal that would justify a future P5 Primitive
// benchmark (getenv per call, path-string replacement) if
// this comes back slow.
//
// Functionally this is the same traversal bench_equilibrium_read.cpp already
// drives via equilibrium_seed::read_back, isolated and named as its own
// operation (PRD #57 Implementation Decisions: "AOS-heavy traversal
// benchmark") because its coverage target -- P1/P4 round-trip count -- is
// distinct from "read everything in the IDS" as a whole. Where
// BM_EquilibriumWholeIdsRead also reads the top-level scalar and timebase
// leaves, BM_AosHeavyTraversal scopes strictly to the time_slice
// array-of-structures: al_begin_arraystruct_action, then for every element
// (all equilibrium_seed::kNSlices of them, not just one -- that's
// BM_TimeSliceElementRead's job) read all five leaves and
// al_iterate_over_arraystruct to the next element.
//
// Reuses equilibrium_seed.h verbatim (issue #51); no size parametrization
// (PRD #57 user story 9).
//
// Timed region = one operation (issue #53, PRD #50 user story 6): the
// reference equilibrium is written and the data entry opened once in untimed
// setup; only begin_global_action(READ) -> begin_arraystruct_action ->
// iterate every element, reading every leaf -> end_action(aos) ->
// end_action(op) is inside the `for (auto _ : state)` loop.
//
// Backend is a sub-axis of the operation (PRD #50 "structure:
// scenario-primary, backend as a sub-axis"): BM_AosHeavyTraversal is the one
// operation body, HDF5 and Memory are two BENCHMARK_CAPTURE registrations of
// it differing only in which backend id the URI is built for.

#include "bench_equilibrium_traversal.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <benchmark/benchmark.h>

using al_bench::BenchBackend;
using al_bench::CheckOk;

void BM_AosHeavyTraversal(benchmark::State& state, BenchBackend backend) {
  // pulse=1, matching bench_equilibrium_read.cpp and
  // bench_timeslice_element_read.cpp: all three are read-only operations, so
  // (unlike bench_equilibrium_write.cpp's pulse=2) there's no repeated-write
  // staleness concern that would require a distinct identity within the same
  // al_benchmarks process.
  const al_contract::PulseId pulse{/*database=*/"bench", /*version=*/"3",
                                    /*pulse=*/1, /*run=*/0};

  // --- untimed setup: open once, write the reference equilibrium once ------
  al_bench::BenchPulse bp = al_bench::OpenBenchPulse(backend, pulse);
  CheckOk(equilibrium_seed::write(bp.pulse_ctx), "equilibrium_seed::write");

  // --- timed region: begin_global_action(READ) -> every time_slice element,
  //     every field, via al_iterate_over_arraystruct -> end_action ---------
  std::vector<int>    shape;
  std::vector<double> data;
  for (auto _ : state) {
    int op = -1;
    CheckOk(al_begin_global_action(bp.pulse_ctx, equilibrium_seed::kIds, "",
                                    READ_OP, &op),
            "al_begin_global_action");

    int size    = 0;
    int aos_ctx = -1;
    CheckOk(al_begin_arraystruct_action(op, equilibrium_seed::kAos,
                                         equilibrium_seed::kAosTime, &size,
                                         &aos_ctx),
            "al_begin_arraystruct_action");

    for (int i = 0; i < size; ++i) {
      al_bench::ReadCurrentEquilibriumTimeSliceLeaves(aos_ctx, &shape, &data);
      if (i + 1 < size) {
        CheckOk(al_iterate_over_arraystruct(aos_ctx, 1),
                "al_iterate_over_arraystruct");
      }
    }

    CheckOk(al_end_action(aos_ctx), "al_end_action(aos_ctx)");
    CheckOk(al_end_action(op), "al_end_action(op)");
  }

  // --- untimed teardown ------------------------------------------------------
  CheckOk(al_close_pulse(bp.pulse_ctx, CLOSE_PULSE), "al_close_pulse");
  // bp.base's destructor (HDF5 only) removes the temp directory on every exit path.
}
// Repetitions(N) makes Google Benchmark report the distribution (median,
// mean, stddev, iteration count) across N independent traversal cycles, not
// a bare single-shot mean (issue #53, PRD #50 user story 8).
BENCHMARK_CAPTURE(BM_AosHeavyTraversal, HDF5, BenchBackend::kHdf5)
    ->Repetitions(20);
BENCHMARK_CAPTURE(BM_AosHeavyTraversal, Memory, BenchBackend::kMemory)
    ->Repetitions(20);
