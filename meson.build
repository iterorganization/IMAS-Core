project(
  'imas-core',
  license: 'LGPL-3.0-or-later',
  meson_version: '>=1.1.0',
  version: run_command(
    find_program('python3'),
    '-m', 'setuptools_scm',
    '--strip-dev',
    check: true,
  ).stdout().strip(),
  default_options: [
    'c_std=c17',
    'cpp_std=c++17',
    'buildtype=debugoptimized',
  ],
)

# =============================================================================
# Compiler and System Setup
# =============================================================================
if get_option('al_core') or get_option('al_test') or get_option('al_dummy_exe')
  add_languages('cpp', native: false)
  cxx = meson.get_compiler('cpp')
endif
host_system = host_machine.system()
host_cpu = host_machine.cpu_family()

# =============================================================================
# Warning Flags
# =============================================================================
warning_flags = [
  '-Wall',
]
if host_system == 'darwin'
  warning_flags += [
    '-Wno-deprecated-declarations',
    '-Wno-inconsistent-missing-override',
    '-Wno-delete-non-abstract-non-virtual-dtor',
    '-Wno-unused-but-set-variable',
    '-Wno-unused-variable',
    '-Wno-misleading-indentation',
    '-Wno-reorder-ctor',
    '-Wno-pessimizing-move',
    '-Wno-sometimes-uninitialized',
  ]
endif

# =============================================================================
# Access Layer core (al-core) Library
# =============================================================================
if get_option('al_core')
  subdir('include')
  subdir('src')
else
  # Use external al-core installation
  al_core_dep = dependency('al-core')
endif

# =============================================================================
# Dummy Executable for version printing
# =============================================================================
if get_option('al_dummy_exe')
  imas_print_version = executable(
    'imas_print_version',
    'tests/imas_print_version.cpp',
    dependencies: [al_core_dep],
    install: true,
  )
  test('imas_print_version_test', imas_print_version)
endif

# =============================================================================
# Test Low-level C API (al-core)
# =============================================================================
if get_option('al_test')
  # subdir('tests')  # TODO: fix test sources
endif

# =============================================================================
# Python Bindings (imas-core)
# =============================================================================
if get_option('python_bindings')
  subdir('python')
endif

# =============================================================================
# MDSplus Models
# =============================================================================
if get_option('mdsplus_models')
  warning('MDSplus models are not yet supported in Meson build system.')
  # subdir('models/mdsplus')
endif

# =============================================================================
# Summary
# =============================================================================
if get_option('al_core')
  summary(
    {
      'HDF5': get_option('al_backend_hdf5'),
      'UDA': get_option('al_backend_uda'),
      'MDSplus': get_option('al_backend_mdsplus'),
    },
    section: 'Backends',
  )
  summary(
    {
      'Core Arguments': core_args,
      'Warning Flags': warning_flags,
    },
    section: 'Compiler Arguments',
  )
endif

if get_option('python_bindings')
  summary(
    {
      'Python Version': py.language_version(),
      'Buildtime Python Path': py.full_path(),
      'Cython Arguments': cython_args,
    },
    section: 'Python Bindings',
  )
endif