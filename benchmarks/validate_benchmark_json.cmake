# Parse the Google Benchmark output rather than merely checking delimiter
# balance. CMake 3.21 is the repository minimum and provides string(JSON).

if(NOT DEFINED INPUT)
  message(FATAL_ERROR "benchmark JSON checker requires -DINPUT=<path>")
endif()

file(READ "${INPUT}" content)
if(content STREQUAL "")
  message(FATAL_ERROR "benchmark JSON file is empty: ${INPUT}")
endif()

string(JSON root_type ERROR_VARIABLE parse_error TYPE "${content}")
if(parse_error)
  message(FATAL_ERROR "benchmark output is not parseable JSON: ${parse_error}")
endif()
if(NOT root_type STREQUAL "OBJECT")
  message(FATAL_ERROR "benchmark JSON root is not an object")
endif()

string(JSON benchmarks_type ERROR_VARIABLE benchmarks_error TYPE "${content}" benchmarks)
if(benchmarks_error)
  message(FATAL_ERROR "benchmark JSON has no top-level 'benchmarks' key: ${benchmarks_error}")
endif()
if(NOT benchmarks_type STREQUAL "ARRAY")
  message(FATAL_ERROR "benchmark JSON 'benchmarks' key is not an array")
endif()
