conf_data = configuration_data()
conf_data.set('PROJECT_VERSION', meson.project_version())
al_defs_h = configure_file(input: 'al_defs.h.in', output: 'al_defs.h', configuration: conf_data)

public_headers = files(
  'access_layer_base_plugin.h',
  'access_layer_plugin.h',
  'access_layer_plugin_manager.h',
  'al_backend.h',
  'al_const.h',
  'al_context.h',
  'al_exception.h',
  'al_lowlevel.h',
  'data_interpolation.h',
  'extended_access_layer_plugin.h',
  'provenance_plugin_feature.h',
  'readback_plugin_feature.h',
  'uri_parser.h',
) + al_defs_h

# Include both source include directory and build include directory
public_includes = include_directories('.')
install_headers(public_headers, subdir: 'al_core')