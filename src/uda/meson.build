# UDA Backend

# Add sources
core_sources += files(
  'pugixml.cpp',
  'uda_backend.cpp',
  'uda_cache.cpp',
  'uda_path.cpp',
  'uda_xml.cpp',
)

# Add include directories
core_inc += include_directories('.')

# Add dependencies
if get_option('uda_fat')
  uda_fat_cpp_dep = dependency('uda-fat-cpp')
  core_deps += [
    dependency('uda-fat-client'),
    uda_fat_cpp_dep,
  ]
else
  uda_cpp_dep = dependency('uda-cpp')
  core_deps += [
    dependency('uda-client'),
    uda_cpp_dep,
  ]
endif

# Add compiler flags
if get_option('uda_fat')
  if uda_fat_cpp_dep.version().version_compare('>=2.7.6')
    core_args += ['-DUDA', '-DUDA_LEGACY_276', '-DUDA_CLIENT_FLAGS_API']
  else
    core_args += ['-DUDA']
  endif
else
  if uda_cpp_dep.version().version_compare('>=2.7.6')
    core_args += ['-DUDA', '-DUDA_LEGACY_276', '-DUDA_CLIENT_FLAGS_API']
  else
    core_args += ['-DUDA']
  endif
endif