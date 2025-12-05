# Test executables for AL-Core

# =============================================================================
# Test executables
# =============================================================================

# C++ test executable
testlowlevel = executable(
  'testlowlevel',
  'testlowlevel.cpp',
  dependencies: [al_core_dep],
  cpp_args: ['-DNOIMPLIB'],
  install: true,
)

# C test executable
testlowlevel_c = executable(
  'testlowlevel_c',
  'testlowlevel_c.c',
  dependencies: [al_core_dep],
  c_args: ['-DNOIMPLIB'],
  install: true,
)

# Test cases
test('Test lowlevel C++', testlowlevel)
test('Test lowlevel C', testlowlevel_c)