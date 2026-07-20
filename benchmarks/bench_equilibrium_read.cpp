// The headline §7 P1 metric (NORTH_STAR.md §8.4 "whole-IDS get"): a whole-IDS
// read of the realistic equilibrium fixture through the HDF5 backend, driven
// strictly at the public C ABI (al_lowlevel.h). Reuses equilibrium_seed.h
// verbatim (issue #51) as the workload -- no bespoke fixture -- so the
// benchmarked shape never drifts from what the contract suite already pins.
//
// Timed region = one operation (issue #53, PRD #50 user story 6): the
// reference equilibrium is written and the data entry opened once in untimed
// setup; only equilibrium_seed::read_back's begin_global_action(READ) ->
// traverse every leaf -> end_action is inside the `for (auto _ : state)` loop.
// Backend is a sub-axis of the whole-IDS-read operation (PRD #50 "structure:
// scenario-primary, backend as a sub-axis"), so the Memory control (issue #54)
// lands as a second BENCHMARK() registration reusing this same function shape.

#include "bench_common.h"
#include "equilibrium_seed.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <benchmark/benchmark.h>

#include <cstdlib>
#include <filesystem>
#include <string>

#if defined(_WIN32)
#include <process.h>
#define AL_BENCH_GETPID _getpid
#else
#include <unistd.h>
#define AL_BENCH_GETPID getpid
#endif

namespace {

using al_bench::CheckOk;

// The HDF5 backend is on-disk (unlike Memory in bench_write_read.cpp), so
// FORCE_CREATE_PULSE needs the legacy <base>/<db>/<ver>/<pulse>/<run> tree
// pre-created -- mirrors tests/contract/al_contract.h's TempBase, reimplemented
// here without a GoogleTest dependency (this tree links al_contract_fixtures,
// not al_contract.h). RAII like TempBase: the directory is created in the
// constructor and always removed in the destructor, including when a
// CheckOk() throws mid-setup, so a failing run never leaks the temp dir.
class TempHdf5Dir {
public:
  TempHdf5Dir()
      : path_(std::filesystem::temp_directory_path() /
              ("al_benchmarks_hdf5_" + std::to_string(AL_BENCH_GETPID()))) {
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
    std::filesystem::create_directories(path_ / "bench" / "3" / "1" / "0", ec);
  }
  ~TempHdf5Dir() {
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
  }
  TempHdf5Dir(const TempHdf5Dir&)            = delete;
  TempHdf5Dir& operator=(const TempHdf5Dir&) = delete;

  std::string uri() const {
    char*       uri = nullptr;
    CheckOk(al_build_uri_from_legacy_parameters(
                HDF5_BACKEND, /*pulse=*/1, /*run=*/0, path_.string().c_str(),
                /*tokamak=*/"bench", /*version=*/"3", /*options=*/"", &uri),
            "al_build_uri_from_legacy_parameters");
    std::string result(uri ? uri : "");
    free(uri);
    return result;
  }

private:
  std::filesystem::path path_;
};

}  // namespace

void BM_EquilibriumWholeIdsRead_HDF5(benchmark::State& state) {
  TempHdf5Dir       dir;
  const std::string uri = dir.uri();

  // --- untimed setup: open once, write the reference equilibrium once ------
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
  // dir's destructor removes the temp directory on every exit path.
}
// Repetitions(N) makes Google Benchmark report the distribution (median,
// mean, stddev, iteration count) across N independent setup/read/teardown
// cycles, not a bare single-shot mean (issue #53, PRD #50 user story 8).
BENCHMARK(BM_EquilibriumWholeIdsRead_HDF5)->Repetitions(20);
