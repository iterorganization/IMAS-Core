# AL-Core Library

core_sources = files(
  'access_layer_plugin_manager.cpp',
  'al_backend.cpp',
  'al_const.cpp',
  'al_context.cpp',
  'al_exception.cpp',
  'al_lowlevel.cpp',
  'ascii_backend.cpp',
  'data_interpolation.cpp',
  'flexbuffers_backend.cpp',
  'memory_backend.cpp',
  'no_backend.cpp',
)

# Enable ASCII backend
core_args = [
  '-DASCII',
]

# Include local and public headers
core_inc = [include_directories('.'), public_inc]

# =============================================================================
# Dependencies
# =============================================================================
boost_dep = dependency(
  'boost',
  modules: ['filesystem'],
  required: false,
)
if not boost_dep.found()
  boost_dep = cxx.find_library('boost_filesystem')
endif

core_deps = [
  boost_dep,
]

if host_system == 'windows'
  core_deps += [
    cxx.find_library('pthreads'),
    dependency('dlfcn-win32', modules: ['dlfcn-win32::dl'], method: 'cmake'),
  ]
endif

# =============================================================================
# Add Optional Backends
# =============================================================================
if get_option('al_backend_hdf5')
  subdir('hdf5')
endif

if get_option('al_backend_mdsplus')
  subdir('mdsplus')
endif

if get_option('al_backend_uda')
  subdir('uda')
endif

# =============================================================================
# Library
# =============================================================================
al_core = library(
  'al',
  core_sources,
  cpp_args: core_args + warning_flags,
  include_directories: core_inc,
  dependencies: core_deps,
  install: true,
)

# Declare dependency for other libraries to use
al_core_dep = declare_dependency(
  link_with: al_core,
  include_directories: public_inc,
)

# =============================================================================
# Package Configuration
# =============================================================================
pkg = import('pkgconfig')
pkg.generate(
  libraries: al_core,
  subdirs: ['al_core'],
  name: 'al-core',
  description: 'IMAS Access Layer core libraries for C',
  url: 'https://github.com/iterorganization/IMAS-Core',
  version: meson.project_version(),
)