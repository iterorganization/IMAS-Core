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
  test_plugins.cpp
  test_backend_factory.cpp
  test_hdf5_version_drift.cpp
  real_dd_path_catalog.cpp
  test_real_dd_path_catalog.cpp
  test_structured_data.cpp
  test_pulse_lifecycle.cpp
  test_mdsplus.cpp
  test_mdsplus_real_paths.cpp
  test_mdsplus_delete.cpp
  test_mdsplus_version_drift.cpp
  test_mdsplus_unique_surface.cpp
  test_uda.cpp
  test_uda_real_paths.cpp
  test_uda_breadth.cpp
  test_uda_unique_surface.cpp
)

target_link_libraries(contract_tests PRIVATE al GTest::gtest GTest::gtest_main)
add_dependencies(contract_tests al)
target_compile_features(contract_tests PRIVATE cxx_std_17)

########################################################################################################################
# HDF5 drift-fixture producer (issue #36).
#
# The stored-backend-version drift check is asserted through the public C ABI
# (test_hdf5_version_drift.cpp), but preparing a MISMATCHED pulse requires
# rewriting master.h5's HDF5_BACKEND_VERSION attribute — out-of-band HDF5 C
# API surgery that must NOT leak into contract_tests. It lives in a separate
# tool the tests invoke as a subprocess; contract_tests only learns the tool's
# path (and gates the test file's compilation on it).
if(AL_BACKEND_HDF5)
  find_package(HDF5 COMPONENTS C REQUIRED)
  add_executable(al-hdf5-fixture-tool hdf5_fixture_tool.cpp)
  target_include_directories(al-hdf5-fixture-tool PRIVATE ${HDF5_C_INCLUDE_DIRS})
  target_link_libraries(al-hdf5-fixture-tool PRIVATE ${HDF5_C_LIBRARIES})
  target_compile_definitions(al-hdf5-fixture-tool PRIVATE ${HDF5_C_DEFINITIONS})
  target_compile_features(al-hdf5-fixture-tool PRIVATE cxx_std_17)
  add_dependencies(contract_tests al-hdf5-fixture-tool)
  target_compile_definitions(contract_tests PRIVATE
    AL_CONTRACT_HDF5_FIXTURE_TOOL="$<TARGET_FILE:al-hdf5-fixture-tool>")
endif()

########################################################################################################################
# MDSplus build gate (issue #13 / TEST_STRATEGY.md D4 "compile-guarded tier").
#
# test_mdsplus.cpp wraps its whole body in #ifdef AL_CONTRACT_HAVE_MDSPLUS, so
# it compiles out to nothing when the backend is off — a developer without
# MDSplus installed never pays for it, and CI legs that build with
# AL_BACKEND_MDSPLUS=OFF are unaffected. When the backend is on, the file's
# lone test still runtime-skips (GTEST_SKIP()) if MDSPLUS_MODELS_PATH is
# unset, so "on but unconfigured" never fails either.
if(AL_BACKEND_MDSPLUS)
  target_compile_definitions(contract_tests PRIVATE AL_CONTRACT_HAVE_MDSPLUS)

  # MDSplus drift-fixture producer (issue #35). test_mdsplus_version_drift.cpp
  # (issue #16) forces the version-mismatch case by overwriting the pulse's
  # VERSION:BACK_MAJOR tree node — reachable only through the raw MDSplus C++
  # API (<mdsobjects.h>), i.e. outside the public C ABI that is the contract
  # suite's one seam. That mutation is fixture PREPARATION, so it lives in a
  # separate build-gated tool the test invokes as a subprocess; contract_tests
  # itself has no MDSplus include or link dependency, and a rewrite satisfying
  # the C ABI is judged by exactly the C ABI. find_package(MDSplus) is
  # repeated (idempotent) for the tool only.
  find_package(MDSplus REQUIRED)
  add_executable(al-mdsplus-fixture-tool mdsplus_fixture_tool.cpp)
  target_include_directories(al-mdsplus-fixture-tool PRIVATE ${MDSPLUS_INCLUDES})
  target_link_libraries(al-mdsplus-fixture-tool PRIVATE ${MDSPLUS_LIBRARIES})
  target_compile_features(al-mdsplus-fixture-tool PRIVATE cxx_std_17)
  add_dependencies(contract_tests al-mdsplus-fixture-tool)
  target_compile_definitions(contract_tests PRIVATE
    AL_CONTRACT_MDSPLUS_FIXTURE_TOOL="$<TARGET_FILE:al-mdsplus-fixture-tool>")
endif()

########################################################################################################################
# UDA build gate (issue #23 / PRD #21, TEST_STRATEGY.md D4 "compile-guarded tier").
#
# Same shape as the MDSplus gate above: test_uda.cpp wraps its whole body in
# #ifdef AL_CONTRACT_HAVE_UDA, so it compiles out to nothing when neither UDA
# backend is built, and always-on CI legs (both UDA options OFF) are unaffected.
# The gate tracks AL_BACKEND_UDA *and* AL_BACKEND_UDAFAT (the fat-client build),
# matching how src/CMakeLists.txt enables the UDA source. When the
# backend is on the test still runtime-skips (GTEST_SKIP()) if UDA_HOST is unset,
# so "on but no reference stack reachable" never fails either. Unlike MDSplus the
# gate needs no extra test-only link deps: the whole round trip goes through the
# public C ABI, and the uda-cpp client is already linked PRIVATEly into `al` by
# src/uda/CMakeLists.txt (the smoke test never touches UDA's own API).
if(AL_BACKEND_UDA OR AL_BACKEND_UDAFAT)
  target_compile_definitions(contract_tests PRIVATE AL_CONTRACT_HAVE_UDA)
endif()

########################################################################################################################
# Test plugin fixture (issue #7, TEST_STRATEGY.md step 5).
#
# A real, loadable plugin the C ABI can dlopen, so the register/bind/unbind/
# unregister state machine and the al_setvalue_*_parameter_plugin happy paths
# run end to end (not just their failure modes). This is the classic red-green
# fixture/harness the issue calls out. Built as a MODULE named exactly
# "<name>_plugin.so" (PREFIX ""/SUFFIX ".so") because registerPlugin resolves
# "$IMAS_AL_PLUGINS/<plugin_name>_plugin.so" (src/al_lowlevel.cpp:335). The
# plugin base classes are header-only, so the module links no libal symbols —
# it only needs al's include dirs (and al's generated al_defs.h). The plugins
# framework is dlopen/dlfcn-based, so this is POSIX-only.
if(NOT WIN32)
  add_library(al_contract_plugin MODULE test_plugin_fixture.cpp)
  # Pull in al's (source + generated) include dirs without linking its symbols.
  target_include_directories(al_contract_plugin
    PRIVATE $<TARGET_PROPERTY:al,INTERFACE_INCLUDE_DIRECTORIES>)
  add_dependencies(al_contract_plugin al)          # ensure al_defs.h is generated
  add_dependencies(contract_tests al_contract_plugin)
  target_compile_features(al_contract_plugin PRIVATE cxx_std_17)
  set_target_properties(al_contract_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "alcontract_plugin"
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins")

  # Tell test_plugins.cpp where the built .so lives and what name to register.
  target_compile_definitions(contract_tests PRIVATE
    AL_CONTRACT_PLUGIN_DIR="${CMAKE_CURRENT_BINARY_DIR}/plugins"
    AL_CONTRACT_PLUGIN_NAME="alcontract")
endif()

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
# --gtest_list_tests / -R selection works). Every case carries the "contract"
# label; on top of that we split by *substrate* (decision D6) so `ctest -L unit`
# is a fast, hermetic inner loop (PRD user story 29) and `ctest -L integration`
# is the on-disk tier:
#   unit        = pure lookups (Introspection.*), the in-RAM Memory backend
#                 (*Memory* parametrized instances), the plugin ABI death
#                 tests (KnownDefects*), the plugin lifecycle suites
#                 (Plugin* and ReadbackPlugins.*, an in-process registry over
#                 the Memory backend — no on-disk backend), and the URI
#                 builder ownership check (UriOwnership.*, pure string).
#   integration = everything else (HDF5 / ASCII / Flexbuffers on disk).
# One discovery pass over the one binary; assign_contract_labels.cmake then
# classifies every discovered test into exactly one tier (disjoint by
# construction — see the note at the registration block below).

# Issue #31: labels are assigned POST-discovery, not through
# gtest_discover_tests. Two version-dependent defects ruled the old multi-pass
# TEST_FILTER partition out across the supported CMake range:
#   - gtest_discover_tests' PROPERTIES forwarding does not preserve a
#     multi-element LABELS list on every supported version (empirically
#     CMake 3.28 collapses "contract\;unit" to just "contract", silently
#     dropping the rest), so `ctest -L unit` selected nothing there; and
#   - TEST_FILTER needs CMake >= 3.22, so the declared 3.21 floor fell back
#     to a single all-"contract" pass with no unit/integration split at all.
# Instead there is ONE discovery pass (no TEST_FILTER, single-element LABELS —
# both fine on 3.21), and assign_contract_labels.cmake runs afterwards via the
# directory TEST_INCLUDE_FILES property: ctest processes it after the
# discovery include has registered every test, and it re-labels each test by
# name with a plain set_tests_properties list — a mechanism with no
# version-dependent forwarding in the middle. The tier definitions (which
# suites are unit / integration / mdsplus / uda, per issues #13-#17 and
# #23-#26) are documented and maintained in assign_contract_labels.cmake.in.
# No LABELS here: ctest MERGES labels across set_tests_properties calls
# (append semantics), so a discovery-time "contract" could not be removed by
# the labeler and would leak onto the mdsplus/uda tiers, changing what
# `ctest -L contract` (sanitizers.yml) selects. assign_contract_labels.cmake
# assigns each test its complete, final label list instead.
gtest_discover_tests(contract_tests)

# Bake the generated registration file's path into the labeler. A single
# discovery pass for contract_tests means GoogleTest writes
# "contract_tests[1]_tests.cmake".
set(_AL_CONTRACT_DISCOVERED_TESTS_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/contract_tests[1]_tests.cmake")
configure_file(assign_contract_labels.cmake.in assign_contract_labels.cmake @ONLY)
set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES
  "${CMAKE_CURRENT_BINARY_DIR}/assign_contract_labels.cmake")

########################################################################################################################
# Install (mirrors the smoke-test targets).

install(
  TARGETS contract_tests
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
