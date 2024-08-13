# AL core and plugins

if( NOT AL_DOWNLOAD_DEPENDENCIES AND NOT AL_DEVELOPMENT_LAYOUT )
  # The Access Layer core should be available as a module, use PkgConfig to create a
  # target:
  find_package( PkgConfig )
  pkg_check_modules( al REQUIRED IMPORTED_TARGET al-core )
  add_library( al ALIAS PkgConfig::al )
  set( AL_CORE_VERSION ${al_VERSION} )

  # Stop processing
  return()
endif()

include(FetchContent)

if( AL_DOWNLOAD_DEPENDENCIES )
  # Download the AL core from the ITER git:
  FetchContent_Declare(
    al-core
    GIT_REPOSITORY  ${AL_CORE_GIT_REPOSITORY}
    GIT_TAG         ${AL_CORE_VERSION}
  )
else()
  # Look in ../al-core
  set( AL_SOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../al-core )
  if( NOT IS_DIRECTORY ${AL_SOURCE_DIRECTORY} )
    # Repository used to be called "al-lowlevel", check this directory as well for
    # backwards compatibility:
    set( AL_SOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../al-lowlevel )
    if( NOT IS_DIRECTORY ${AL_SOURCE_DIRECTORY} )
      message( FATAL_ERROR
        "${AL_SOURCE_DIRECTORY} does not exist. Please clone the "
        "al-core repository or set AL_DOWNLOAD_DEPENDENCIES=ON."
      )
    endif()
  endif()

  FetchContent_Declare(
    al-core
    SOURCE_DIR      ${AL_SOURCE_DIRECTORY}
  )
  set( AL_SOURCE_DIRECTORY )  # unset temporary var
endif()
# Don't load the AL core when only building documentation
if( NOT AL_DOCS_ONLY )
  FetchContent_MakeAvailable( al-core )
  get_target_property( AL_CORE_VERSION al VERSION )
endif()


if( ${AL_PLUGINS} )
  if( ${AL_DOWNLOAD_DEPENDENCIES} )
    # Download the AL plugins from the ITER git:
    FetchContent_Declare(
      al-plugins
      GIT_REPOSITORY  ${AL_PLUGINS_GIT_REPOSITORY}
      GIT_TAG         ${AL_PLUGINS_VERSION}
    )
  else()
    # Look in ../plugins
    set( PLUGINS_SOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../al-plugins )
    if( NOT IS_DIRECTORY ${PLUGINS_SOURCE_DIRECTORY} )
      message( FATAL_ERROR
        "${PLUGINS_SOURCE_DIRECTORY} does not exist. Please clone the "
        "al-plugins repository or set AL_DOWNLOAD_DEPENDENCIES=ON."
      )
    endif()

    FetchContent_Declare(
      al-plugins
      SOURCE_DIR      ${PLUGINS_SOURCE_DIRECTORY}
    )
    set( PLUGINS_SOURCE_DIRECTORY )  # unset temporary var
  endif()
  FetchContent_MakeAvailable( al-plugins )
endif()

if( AL_HLI_DOCS )
  include( ALBuildDocumentation )
endif()

