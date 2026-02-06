# HDF5 Backend

# Add sources
core_sources += files(
  'hdf5_backend.cpp',
  'hdf5_backend_factory.cpp',
  'hdf5_dataset_handler.cpp',
  'hdf5_events_handler.cpp',
  'hdf5_hs_selection_reader.cpp',
  'hdf5_hs_selection_writer.cpp',
  'hdf5_reader.cpp',
  'hdf5_utils.cpp',
  'hdf5_writer.cpp',
)

# Add include directories
core_inc += include_directories('.')

# Add dependency
core_deps += dependency('hdf5', language: 'cpp')

# Add compiler flags
core_args += '-DHDF5'

if host_system == 'windows'
  core_args += '-DH5_BUILT_AS_DYNAMIC_LIB'
endif