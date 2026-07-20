project( al-benchmarks )

########################################################################################################################
# Google Benchmark, pinned via FetchContent (NORTH_STAR.md §8.4).
#
# Unlike tests/contract's GoogleTest dependency, there is no find_package
# fallback here: the benchmark harness is a standalone tree gated by its own
# option (AL_BUILD_BENCHMARKS), and must not rely on a cluster module (e.g.
# the ITER SDCC `googlebenchmark` module) -- FetchContent keeps the pin
# identical on every machine the harness runs on. Pinned to the immutable
# commit for release v1.9.5 (a tag can be re-pointed).

include(FetchContent)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        192ef10025eb2c4cdd392bc502f0c852196baa48
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

########################################################################################################################
# The benchmark executable.
#
# bench_write_read.cpp is the original tracer bullet: a single-leaf
# write-once-then-read on a tiny inline fixture, against the in-RAM Memory
# backend, needing no filesystem or optional backend.
#
# bench_equilibrium_read.cpp is the real P1 headline workload (issue #53):
# whole-IDS equilibrium read on the HDF5 backend, reusing equilibrium_seed.h
# (issue #51) via the al_contract_fixtures INTERFACE library below -- hence
# needs the filesystem (std::filesystem temp dirs) that Memory-only
# bench_write_read.cpp does not.
#
# bench_main.cpp holds the one BENCHMARK_MAIN() shared by every bench_*.cpp
# TU in this executable.

add_executable( al_benchmarks
  bench_write_read.cpp
  bench_equilibrium_read.cpp
  bench_main.cpp )
target_link_libraries( al_benchmarks PRIVATE al_contract_fixtures benchmark::benchmark )
add_dependencies( al_benchmarks al )
target_compile_features( al_benchmarks PRIVATE cxx_std_17 )

if(NOT WIN32 AND NOT APPLE)
  # stdc++fs is only needed on Linux with older GCC (std::filesystem use in
  # bench_equilibrium_read.cpp), mirroring fixtures/CMakeLists.txt's smoke target.
  target_link_libraries( al_benchmarks PRIVATE stdc++fs )
endif()

########################################################################################################################
# CTest smoke entry.
#
# Proves the harness builds, links libal, drives the C ABI, and produces
# parseable Google Benchmark JSON -- it asserts none of the reported timing
# values (a benchmark "failing" means slower, not wrong; NORTH_STAR.md §8.4).
# --benchmark_min_time=1x caps every registered benchmark at exactly one
# iteration, so the smoke run stays fast regardless of what benchmarks are
# added later.
#
# Follows the FIXTURES_SETUP/FIXTURES_REQUIRED shape tests/CMakeLists.txt uses
# for its own smoke registration: one test produces the JSON file, then CMake's
# built-in JSON parser validates it without adding another runtime dependency.

set( _smoke_json ${CMAKE_CURRENT_BINARY_DIR}/benchmark_smoke.json )

add_test( NAME benchmark_smoke_run
  COMMAND al_benchmarks --benchmark_min_time=1x
                         --benchmark_out=${_smoke_json}
                         --benchmark_out_format=json )
set_tests_properties( benchmark_smoke_run PROPERTIES FIXTURES_SETUP BENCHMARK_SMOKE_JSON )

add_test( NAME benchmark_smoke_validate
  COMMAND ${CMAKE_COMMAND} -DINPUT=${_smoke_json}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/validate_benchmark_json.cmake )
set_tests_properties( benchmark_smoke_validate PROPERTIES FIXTURES_REQUIRED BENCHMARK_SMOKE_JSON )
