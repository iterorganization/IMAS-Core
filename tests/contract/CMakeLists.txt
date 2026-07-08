########################################################################################################################
# Contract test suite (Task 2 — TEST_STRATEGY.md)
#
# GoogleTest + CTest suite binding the public C ABI (al_lowlevel.h + al_const.h)
# — decisions D1 (C-ABI seam) and D3 (GoogleTest for death tests + TEST_P). This
# is the scaffold slice (step 1): it proves every mechanism — round trip on an
# on-disk and an in-RAM backend, and one expected-fail on a real defect — before
# the matrix is broadened in later steps.
#
# The whole suite lives under AL_BUILD_TESTS (OFF by default), so it never
# perturbs the normal library / wheel build.

########################################################################################################################
# GoogleTest — prefer an installed copy, fall back to fetching a pinned release.
#
# find_package first keeps CI/offline builds fast when GTest is already provided
# (e.g. a module or system package). FetchContent is the portable fallback; it
# only runs when tests are being built.

find_package(GTest QUIET)

if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    # Pinned to the immutable commit for v1.15.2 (a tag can be re-pointed).
    GIT_TAG        b514bdc898e2951020cbdca1304b75f5950d1f59
  )
  # Honor the parent project's shared-CRT choice on Windows.
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

########################################################################################################################
# Optional sanitizer build (decision D7): compile the same functional suite with
# AddressSanitizer + UndefinedBehaviorSanitizer so it self-checks for UB / UAF /
# overruns with no per-test code. Non-blocking / informational — a dedicated CI
# leg, not a hard gate, until the baseline is triaged. Enable with
# -DAL_CONTRACT_SANITIZE=ON. LeakSanitizer's leak detection is Linux-only; macOS
# gives crash/overrun detection but not reliable leak detection.

option(AL_CONTRACT_SANITIZE
  "Build the contract test suite with ASan/UBSan (non-blocking)" OFF)

########################################################################################################################
# The contract test executable.
#
# Step 3 (issue #5) adds two files to the single suite: the pure introspection
# lookups (test_introspection.cpp — the thin unit tier: const2str/err2str/
# versions, no backend) and the version sentinel (test_version_sentinel.cpp).
# They join the existing round-trip and known-defect files from the scaffold.
#
# The finer CTest unit/integration LABEL split (decision D6) stays deferred, as
# the scaffold commit recorded — it arrives with the broader matrix in a later
# step. Until then the pure-lookup subset is addressable directly via
# `ctest -R Introspection` / `--gtest_filter=Introspection.*` (a fast, disk-free
# inner loop) even though every case shares the single "contract" label.

add_executable(contract_tests
  test_roundtrip.cpp
  test_roundtrip_matrix.cpp
  test_introspection.cpp
  test_version_sentinel.cpp
  test_capability_gated.cpp
  test_known_defects.cpp
)

target_link_libraries(contract_tests PRIVATE al GTest::gtest GTest::gtest_main)
add_dependencies(contract_tests al)
target_compile_features(contract_tests PRIVATE cxx_std_17)

if(NOT WIN32 AND NOT APPLE)
  # stdc++fs is only needed on Linux with older GCC (std::filesystem in the harness).
  target_link_libraries(contract_tests PRIVATE stdc++fs)
endif()

if(AL_CONTRACT_SANITIZE)
  if(MSVC)
    message(WARNING "AL_CONTRACT_SANITIZE not supported with MSVC; ignoring.")
  else()
    set(_al_sanitize_flags -fsanitize=address,undefined -fno-omit-frame-pointer -g)
    target_compile_options(contract_tests PRIVATE ${_al_sanitize_flags})
    target_link_options(contract_tests PRIVATE -fsanitize=address,undefined)
    # Instrument the library too, not just the tests: D7's value ("a future
    # leak/UAF in the hot path is caught for free") is about libal's code, so
    # the flags must reach the `al` target — otherwise the round trip runs
    # through an uninstrumented library and most internal UAF/overruns escape.
    # `al` is defined in the root CMakeLists before tests/ is added, so the
    # target is visible here. Note: a sanitized libal is meant for the dedicated
    # C++-only sanitizer CI leg; don't combine this option with a wheel build,
    # whose bindings would then link a sanitized libal without the ASan runtime.
    target_compile_options(al PRIVATE ${_al_sanitize_flags})
    target_link_options(al PRIVATE -fsanitize=address,undefined)
  endif()
endif()

########################################################################################################################
# CTest registration.
#
# gtest_discover_tests runs each GoogleTest case as its own ctest entry (so
# --gtest_list_tests / -R selection works) and applies the "contract" label to
# all of them. The finer unit/integration labelling (decision D6) arrives with
# the broader matrix in later steps; the suite keeps one label.

gtest_discover_tests(contract_tests
  PROPERTIES LABELS "contract"
)

########################################################################################################################
# Install (mirrors the smoke-test targets).

install(
  TARGETS contract_tests
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
