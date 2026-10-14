########################################################################################################################
# Shared contract fixtures (issue #51).
#
# The realistic equilibrium seed (equilibrium_seed.h) and the generic typed
# ABI round-trip helpers it needs (al_contract_abi.h), factored out of
# tests/contract/ so a build target outside the test tree -- the future
# benchmarks/ tree (PRD #50, issues #52/#53) -- can link the exact same
# workload the contract suite pins. No fixture drift between what is tested
# and what is measured (spec #50, Option C).
#
# Header-only and carries no GoogleTest dependency (unlike
# tests/contract/al_contract.h), so a non-test binary can link it for free.
# Added unconditionally (not gated by AL_BUILD_TESTS) so it is genuinely
# usable independent of the test build.

add_library(al_contract_fixtures INTERFACE)
target_include_directories(al_contract_fixtures INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(al_contract_fixtures INTERFACE al)
target_compile_features(al_contract_fixtures INTERFACE cxx_std_17)

########################################################################################################################
# Throwaway proof (issue #51 acceptance criterion 3), opt-in via
# AL_BUILD_FIXTURE_SMOKE -- off by default like every other extra target in
# the root CMakeLists.txt (AL_BUILD_TESTS, AL_BUILD_BENCHMARKS), so an
# ordinary/minimal build (e.g. the Python wheel) never pays for it. The
# al_contract_fixtures library above stays unconditional regardless: that is
# the actual "usable outside the test tree" requirement, this executable is
# just a demonstration of it.
#
# A minimal translation unit outside tests/ that includes the fixture, builds,
# and round-trips it against a real backend -- demonstrating the fixture is
# usable without AL_BUILD_TESTS or GoogleTest. Uses the always-on, zero-I/O
# Memory backend so this needs no on-disk setup and no optional backend.
if(AL_BUILD_FIXTURE_SMOKE)
  add_executable(al_contract_fixtures_smoke fixture_smoke.cpp)
  target_link_libraries(al_contract_fixtures_smoke PRIVATE al_contract_fixtures)
  add_dependencies(al_contract_fixtures_smoke al)
  target_compile_features(al_contract_fixtures_smoke PRIVATE cxx_std_17)

  if(NOT WIN32 AND NOT APPLE)
    # stdc++fs is only needed on Linux with older GCC (std::filesystem in the smoke TU).
    target_link_libraries(al_contract_fixtures_smoke PRIVATE stdc++fs)
  endif()
endif()
