hdf5_backend_sources = files(
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

private_includes += [include_directories('.')]

core_deps += dependency('hdf5', language: 'c', required: true)

# Add compiler flags for HDF5
al_core_cpp_args += '-DHDF5'

if host_system == 'windows'
  al_core_cpp_args += '-DH5_BUILT_AS_DYNAMIC_LIB'
endif

# Add HDF5 backend sources
al_core_sources += hdf5_backend_sources