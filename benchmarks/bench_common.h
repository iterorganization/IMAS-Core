// Shared by every bench_*.cpp TU in al_benchmarks: the status-checking helper
// each benchmark uses to turn a failing al_status_t into a thrown exception
// (Google Benchmark reports the failure instead of silently timing garbage),
// plus the per-backend (HDF5 vs. Memory) URI construction and pulse-open
// boilerplate that every whole-IDS-scoped benchmark needs identically (issue
// #58, prefactor for PRD #57's write/single-slice/AOS-traversal benchmarks).
// Header-only so it carries no extra link surface.
#ifndef AL_BENCHMARKS_COMMON_H
#define AL_BENCHMARKS_COMMON_H

#include "al_contract_legacy_storage.h"

#include <al_const.h>
#include <al_lowlevel.h>

#include <cassert>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>

namespace al_bench {

inline void CheckOk(const al_status_t& status, const char* what) {
  if (status.code != 0) {
    throw std::runtime_error(std::string(what) + " failed (code=" +
                              std::to_string(status.code) + "): " +
                              status.message);
  }
}

enum class BenchBackend { kHdf5, kMemory };

// HDF5 resolves FORCE_CREATE_PULSE against a real on-disk legacy tree, so it
// needs `base` alive for the URI to mean anything. Memory never touches disk
// (bench_write_read.cpp's MemoryUri established the pattern): an arbitrary
// absolute-looking `user` sidesteps the legacy URI builder's getpwnam()
// fallback for an empty/unknown user, without paying for a real temp dir.
inline std::string BuildUri(BenchBackend backend, const al_contract::PulseId& pulse,
                             const al_contract::LegacyPulseDirectory* base) {
  char* uri = nullptr;
  if (backend == BenchBackend::kHdf5) {
    assert(base != nullptr && "HDF5 backend requires a legacy pulse directory for its URI");
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

// An open pulse context plus (HDF5 only) the temp legacy directory backing
// it. `base` is a unique_ptr rather than the more obvious optional<> so
// BenchPulse stays movable out of OpenBenchPulse regardless of
// LegacyPulseDirectory's own non-movability (its destructor removes the temp
// dir on every exit path, so it must outlive the pulse being open).
struct BenchPulse {
  int pulse_ctx = -1;
  std::unique_ptr<al_contract::LegacyPulseDirectory> base;
};

// Per-backend pulse lifecycle setup shared by every whole-IDS-scoped
// benchmark: materialize the on-disk legacy tree (HDF5 only), build the
// backend's URI, and open the pulse with FORCE_CREATE_PULSE. Callers write
// their fixture and run their timed region on the returned pulse_ctx, then
// close it themselves (close is operation-specific: some benchmarks time
// around it, none should hide it in a header).
inline BenchPulse OpenBenchPulse(BenchBackend backend, const al_contract::PulseId& pulse) {
  BenchPulse result;
  if (backend == BenchBackend::kHdf5) {
    result.base = std::make_unique<al_contract::LegacyPulseDirectory>();
    result.base->make_legacy_tree(pulse);
  }
  const std::string uri = BuildUri(backend, pulse, result.base.get());
  CheckOk(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &result.pulse_ctx),
          "al_begin_dataentry_action");
  return result;
}

}  // namespace al_bench

#endif  // AL_BENCHMARKS_COMMON_H
