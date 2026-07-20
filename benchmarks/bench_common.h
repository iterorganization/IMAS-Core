// Shared by every bench_*.cpp TU in al_benchmarks: the one status-checking
// helper each benchmark uses to turn a failing al_status_t into a thrown
// exception (Google Benchmark reports the failure instead of silently timing
// garbage). Header-only so it carries no extra link surface.
#ifndef AL_BENCHMARKS_COMMON_H
#define AL_BENCHMARKS_COMMON_H

#include <al_lowlevel.h>

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

}  // namespace al_bench

#endif  // AL_BENCHMARKS_COMMON_H
