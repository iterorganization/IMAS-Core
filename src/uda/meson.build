uda_backend_sources = files(
  'pugixml.cpp',
  'uda_backend.cpp',
  'uda_cache.cpp',
  'uda_path.cpp',
  'uda_xml.cpp',
)

private_includes += [include_directories('.')]

uda_client_dep = dependency('uda-client', required: false)
uda_cpp_dep = dependency('uda-cpp', required: false)

uda_client_dep = dependency('uda-fat-client', required: false)
uda_fat_cpp_dep = dependency('uda-fat-cpp', required: false)

if not uda_cpp_dep.found() and not uda_fat_cpp_dep.found()
  error('UDA backend requested but UDA dependency not found.')
endif

if uda_cpp_dep.found()
  core_deps += [uda_cpp_dep, uda_client_dep]
  if uda_cpp_dep.version() >= '2.7.6'
    al_core_cpp_args += ['-DUDA', '-DUDA_LEGACY_276', '-DUDA_CLIENT_FLAGS_API']
  else
    al_core_cpp_args += ['-DUDA']
  endif
endif

if uda_fat_cpp_dep.found()
  core_deps += [uda_fat_cpp_dep, uda_client_dep]
  if uda_fat_cpp_dep.version() >= '2.7.6'
    al_core_cpp_args += ['-DUDA', '-DUDA_LEGACY_276', '-DUDA_CLIENT_FLAGS_API']
  else
    al_core_cpp_args += ['-DUDA']
  endif
endif

# Add UDA backend sources if UDA dependency is found
if uda_cpp_dep.found() or uda_fat_cpp_dep.found()
  al_core_sources += uda_backend_sources
endif