project(
  'imas-core',
  ['c', 'cpp'],
  license: 'LGPL-3.0-or-later',
  meson_version: '>=0.60.0',
  version: run_command('setuptools-scm', '--strip-dev', check: true).stdout().strip(),
  default_options: [
    'c_std=c17',
    'cpp_std=c++17',
    'buildtype=debugoptimized',
  ],
)

# =============================================================================
# Compiler and System Setup
# =============================================================================
cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')
host_system = host_machine.system()
host_cpu = host_machine.cpu_family()

# =============================================================================
# Access Layer core (al-core) Library
# =============================================================================
if get_option('al_core')
  subdir('include')
  subdir('src')
else
  if get_option('al_dummy_exe')
    al_core_dep = dependency('al-core', required: true)
  endif
endif

# =============================================================================
# Dummy Executable for version printing
# =============================================================================
if get_option('al_dummy_exe')
  executable(
    'imas_print_version',
    'tests/imas_print_version.cpp',
    dependencies: [al_core_dep],
    install: true,
  )
endif

# =============================================================================
# Python Bindings (imas-core)
# =============================================================================
if get_option('python_bindings')
  subdir('python')
endif

#
# MDSplus Models
#
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
      'C++ Arguments': al_core_cpp_args,
    },
    section: 'Compiler Arguments',
  )
endif