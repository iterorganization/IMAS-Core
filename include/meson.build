# =============================================================================
# Configuration Data for al_defs.h
# =============================================================================

cdata = configuration_data()

# Version information
cdata.set('PROJECT_VERSION', meson.project_version())

# Generate Config Headers
al_defs_h = configure_file(
  input: 'al_defs.h.in',
  output: 'al_defs.h',
  configuration: cdata,
)

install_headers(al_defs_h, subdir: 'al_core')

# =============================================================================
# Install Public Headers
# =============================================================================

install_headers(
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
  'fix_include_windows.h',
  'provenance_plugin_feature.h',
  'readback_plugin_feature.h',
  'uri_parser.h',
  subdir: 'al_core',
)

# =============================================================================
# Include Directories
# =============================================================================
public_inc = include_directories('.')