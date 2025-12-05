# AL-Core Library

al_core_sources = files(
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

private_includes = [include_directories('.')]

# Enable ASCII backend
al_core_cpp_args = [
  '-DASCII',
]

# =============================================================================
# Dependencies
# =============================================================================
boost_dep = dependency('boost', modules: ['filesystem'], required: false)
if not boost_dep.found()
  boost_dep = cxx.find_library('boost_filesystem', required: true)
endif

core_deps = [
  boost_dep,
]

if host_system == 'windows'
  core_deps += [
    dependency('threads', required: true),
    dependency('dlfcn-win32', required: true),
  ]
endif

# =============================================================================
# Add Optional Backends
# =============================================================================
if get_option('al_backend_mdsplus')
  warning('MDSplus backend is not yet supported in Meson build system.')
  # subdir('mdsplus')
endif

if get_option('al_backend_hdf5')
  subdir('hdf5')
endif

if get_option('al_backend_uda')
  subdir('uda')
endif

# =============================================================================
# Library
# =============================================================================
al_core = library(
  'al',
  al_core_sources,
  cpp_args: al_core_cpp_args,
  include_directories: private_includes + public_includes,
  dependencies: core_deps,
  version: meson.project_version(),
  install: true,
)

# Declare dependency for other libraries to use
al_core_dep = declare_dependency(
  link_with: al_core,
  include_directories: public_includes,
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