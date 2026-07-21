// The shared "before" baseline for two future optimizations (issue #60,
// PRD #57): a single time_slice element's full leaf set --  its own timebase
// leaf, radial profile array, scalar quantity, and constraints subtree --
// read by index, node-by-node, through the public C ABI. Today this
// node-by-node walk is the *only* legal way to fetch one AOS element: no
// al_read_subtree call exists yet (NORTH_STAR.md §7 P2) and HDF5 slice-mode
// buffering hasn't been built yet either (§7 P3). Both future optimizations
// diff against BM_TimeSliceElementRead below rather than against two
// near-identical registrations of their own.
//
// Reuses equilibrium_seed.h verbatim (issue #51) for the fixture, exactly as
// bench_equilibrium_read.cpp does, so the workload never drifts from what the
// contract suite already pins. Only the read shape differs: where
// BM_EquilibriumWholeIdsRead walks every time_slice element,
// BM_TimeSliceElementRead selects one (the last, kNSlices - 1, so the
// benchmark exercises al_iterate_over_arraystruct actually moving the
// cursor rather than reading the trivial post-begin default position) and
// reads only that element's five leaves.
//
// Timed region = one operation (issue #53, PRD #50 user story 6): the
// reference equilibrium is written and the data entry opened once in untimed
// setup; only begin_global_action(READ) -> begin_arraystruct_action ->
// iterate to the target index -> read every leaf of that element ->
// end_action(aos) -> end_action(op) is inside the `for (auto _ : state)` loop.
//
// Backend is a sub-axis of the operation (PRD #50 "structure:
// scenario-primary, backend as a sub-axis"): BM_TimeSliceElementRead is the
// one operation body, HDF5 and Memory are two BENCHMARK_CAPTURE
// registrations of it differing only in which backend id the URI is built
// for.

#include "bench_common.h"
#include "al_contract_abi.h"
#include "al_contract_legacy_storage.h"
#include "equilibrium_seed.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <benchmark/benchmark.h>

using al_bench::BenchBackend;
using al_bench::CheckOk;

void BM_TimeSliceElementRead(benchmark::State& state, BenchBackend backend) {
  const al_contract::PulseId pulse{/*database=*/"bench", /*version=*/"3",
                                    /*pulse=*/1, /*run=*/0};
  constexpr int kTargetSlice = equilibrium_seed::kNSlices - 1;

  // --- untimed setup: open once, write the reference equilibrium once ------
  al_bench::BenchPulse bp = al_bench::OpenBenchPulse(backend, pulse);
  CheckOk(equilibrium_seed::write(bp.pulse_ctx), "equilibrium_seed::write");

  // --- timed region: begin_global_action(READ) -> select element ->
  //     read every leaf of that element -> end_action ----------------------
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
    // `step` is relative to the cursor's current position, not an absolute
    // index (al_lowlevel.h: "iteration step size, typically=1"); this only
    // lands on kTargetSlice because al_begin_arraystruct_action always opens
    // at index 0.
    CheckOk(al_iterate_over_arraystruct(aos_ctx, kTargetSlice),
            "al_iterate_over_arraystruct");

    auto read_leaf = [&](const char* path, int rank, const char* what) {
      CheckOk(al_contract::read_data<double>(aos_ctx, path, rank, &shape, &data),
              what);
      benchmark::DoNotOptimize(data);
    };
    read_leaf(equilibrium_seed::kSliceTime, 0, "read time");
    read_leaf(equilibrium_seed::kPsi, 1, "read profiles_1d/psi");
    read_leaf(equilibrium_seed::kIp, 0, "read global_quantities/ip");
    read_leaf(equilibrium_seed::kMeasured, 0, "read constraints/ip/measured");
    read_leaf(equilibrium_seed::kWeight, 0, "read constraints/ip/weight");

    CheckOk(al_end_action(aos_ctx), "al_end_action(aos_ctx)");
    CheckOk(al_end_action(op), "al_end_action(op)");
  }

  // --- untimed teardown ------------------------------------------------------
  CheckOk(al_close_pulse(bp.pulse_ctx, CLOSE_PULSE), "al_close_pulse");
  // bp.base's destructor (HDF5 only) removes the temp directory on every exit path.
}
// Repetitions(N) makes Google Benchmark report the distribution (median,
// mean, stddev, iteration count) across N independent read cycles, not a
// bare single-shot mean (issue #53, PRD #50 user story 8).
BENCHMARK_CAPTURE(BM_TimeSliceElementRead, HDF5, BenchBackend::kHdf5)
    ->Repetitions(20);
BENCHMARK_CAPTURE(BM_TimeSliceElementRead, Memory, BenchBackend::kMemory)
    ->Repetitions(20);
