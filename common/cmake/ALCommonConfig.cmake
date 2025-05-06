# Shared options

# Note: default options are also listed in the docs: doc_common/building_installing.rst
# When changing default values, the documentation should be updated to reflect that.

# Note: AL_DOWNLOAD_DEPENDENCIES is also shared, but needs to be set before this
# repository is available, so not listing it here.

option( BUILD_SHARED_LIBS "Build shared libraries" ON )
option( AL_EXAMPLES "Build and test examples" ON )
option( AL_TESTS "Build tests and enable test framework" ON )
option( AL_PLUGINS "Enable plugin framework for tests and examples" ON )
option( AL_HLI_DOCS "Build the Sphinx-based High Level Interface documentation" OFF )
option( AL_DOCS_ONLY "Don't build anything, except the Sphinx-based High Level Interface documentation" OFF )

if( NOT AL_DOWNLOAD_DEPENDENCIES )
  if( DEFINED ENV{AL_COMMON_PATH} )
    set( _DEV OFF )
  else()
    set( _DEV ON )
  endif()
  option( AL_DEVELOPMENT_LAYOUT "Look into parent directories for dependencies" ${_DEV} )
endif()

# Enable CTest?
if( AL_EXAMPLES OR AL_TESTS )
    include( CTest )
endif()

# Allow configuration of repositories and branches for dependent packages
################################################################################

if( AL_DOWNLOAD_DEPENDENCIES )
  # AL plugins
  ##############################################################################
  set(
    AL_PLUGINS_GIT_REPOSITORY "ssh://git@git.iter.org/imas/al-plugins.git"
    CACHE STRING "Git repository of al-plugins"
  )
  set(
    AL_PLUGINS_VERSION "main"
    CACHE STRING "al-plugins version (tag or branch name) to use for this build"
  )

  # Data dictionary
  ##############################################################################
  set(
    DD_GIT_REPOSITORY "https://github.com/iterorganization/IMAS-data-Dictionary.git"
    CACHE STRING "Git repository of the Data Dictionary"
  )
  set(
    DD_VERSION "main"
    CACHE STRING "Data dictionary version (tag or branch name) to use for this build"
  )
endif()
