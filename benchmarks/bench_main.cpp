// Shared Google Benchmark entry point. Every bench_*.cpp in al_benchmarks only
// registers benchmarks via BENCHMARK(...); exactly one translation unit may
// define main(), so it lives here rather than in whichever bench_*.cpp file
// happened to have it first.
#include <benchmark/benchmark.h>

BENCHMARK_MAIN();
