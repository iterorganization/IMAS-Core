# MDSplus Backend

# Add sources
core_sources += files(
  'mdsplus_backend.cpp',
)

# Add include directories
core_inc += include_directories('.')

# Add dependency
core_deps += dependency('mdsplus')

# Add compiler flags
core_args += '-DMDSPLUS'

if host_system == 'windows'
  core_args += '-DMDSOBJECTSCPPSHRVS_EXPORTS'
endif