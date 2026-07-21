// Shared equilibrium time_slice leaf traversal for the operation benchmarks.
// Keeping the fixture-specific path list here prevents the single-element and
// full-AOS benchmarks from drifting while leaving their iteration boundaries
// and timed operation structure explicit at each call site.
#ifndef AL_BENCHMARKS_EQUILIBRIUM_TRAVERSAL_H
#define AL_BENCHMARKS_EQUILIBRIUM_TRAVERSAL_H

#include "al_contract_abi.h"
#include "bench_common.h"
#include "equilibrium_seed.h"

#include <benchmark/benchmark.h>

#include <vector>

namespace al_bench {

inline void ReadCurrentEquilibriumTimeSliceLeaves(
    int aos_ctx, std::vector<int>* shape, std::vector<double>* data) {
  auto read_leaf = [&](const char* path, int rank, const char* what) {
    CheckOk(al_contract::read_data<double>(aos_ctx, path, rank, shape, data),
            what);
    benchmark::DoNotOptimize(*data);
  };

  read_leaf(equilibrium_seed::kSliceTime, 0, "read time");
  read_leaf(equilibrium_seed::kPsi, 1, "read profiles_1d/psi");
  read_leaf(equilibrium_seed::kIp, 0, "read global_quantities/ip");
  read_leaf(equilibrium_seed::kMeasured, 0, "read constraints/ip/measured");
  read_leaf(equilibrium_seed::kWeight, 0, "read constraints/ip/weight");
}

}  // namespace al_bench

#endif  // AL_BENCHMARKS_EQUILIBRIUM_TRAVERSAL_H
