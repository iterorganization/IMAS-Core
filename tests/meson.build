# Test executables for AL-Core

# =============================================================================
# Test executables
# =============================================================================

# C++ test executable
executable(
  'testlowlevel',
  'testlowlevel.cpp',
  dependencies: [al_core_dep],
  cpp_args: ['-DNOIMPLIB'],
  install: true,
)

# C test executable
executable(
  'testlowlevel_c',
  'testlowlevel_c.c',
  dependencies: [al_core_dep],
  c_args: ['-DNOIMPLIB'],
  install: true,
)