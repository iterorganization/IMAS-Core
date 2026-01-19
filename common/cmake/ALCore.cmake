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
if(WIN32)
  if( AL_DOWNLOAD_DEPENDENCIES )
    # Download the AL core from the ITER git using direct git commands:
    set( al-core_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/al-core-src" )
    if( NOT EXISTS "${al-core_SOURCE_DIR}/.git" )
      message( STATUS "Cloning al-core from ${AL_CORE_GIT_REPOSITORY}" )
      execute_process(
        COMMAND git clone "${AL_CORE_GIT_REPOSITORY}" "${al-core_SOURCE_DIR}"
        RESULT_VARIABLE _GIT_CLONE_RESULT
        ERROR_VARIABLE _GIT_CLONE_ERROR
      )
      if( _GIT_CLONE_RESULT )
        message( FATAL_ERROR "Failed to clone al-core: ${_GIT_CLONE_ERROR}" )
      endif()
    endif()
    # Checkout the specified version
    execute_process(
      COMMAND git fetch origin
      WORKING_DIRECTORY "${al-core_SOURCE_DIR}"
      RESULT_VARIABLE _GIT_FETCH_RESULT
    )
    execute_process(
      COMMAND git checkout "${AL_CORE_VERSION}"
      WORKING_DIRECTORY "${al-core_SOURCE_DIR}"
      RESULT_VARIABLE _GIT_CHECKOUT_RESULT
      ERROR_VARIABLE _GIT_CHECKOUT_ERROR
    )
    if( _GIT_CHECKOUT_RESULT )
      message( FATAL_ERROR "Failed to checkout ${AL_CORE_VERSION}: ${_GIT_CHECKOUT_ERROR}" )
    endif()
  else()
    # Look in ../al-core
    set( al-core_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../al-core )
    if( NOT IS_DIRECTORY ${al-core_SOURCE_DIR} )
      # Repository used to be called "al-lowlevel", check this directory as well for
      # backwards compatibility:
      set( al-core_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../al-lowlevel )
      if( NOT IS_DIRECTORY ${al-core_SOURCE_DIR} )
        message( FATAL_ERROR
          "${al-core_SOURCE_DIR} does not exist. Please clone the "
          "al-core repository or set AL_DOWNLOAD_DEPENDENCIES=ON."
        )
      endif()
    endif()
  endif()
else()
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
endif()

# Don't load the AL core when only building documentation
if( NOT AL_DOCS_ONLY )
  # Ensure vcpkg packages are found in the subdirectory
  if(WIN32)
    # On Windows, ensure vcpkg packages are available to the subdirectory
    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
      # Add vcpkg installed directory to CMAKE_PREFIX_PATH for the subdirectory
      set(CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET};${CMAKE_PREFIX_PATH}")
      # Pass vcpkg variables to subdirectory by setting them in parent scope
      set(VCPKG_INSTALLED_DIR "${VCPKG_INSTALLED_DIR}" CACHE STRING "vcpkg installed dir" FORCE)
      set(VCPKG_TARGET_TRIPLET "${VCPKG_TARGET_TRIPLET}" CACHE STRING "vcpkg triplet" FORCE)
      message(STATUS "ALCore: Passing vcpkg paths to al-core subdirectory")
      message(STATUS "  VCPKG_INSTALLED_DIR: ${VCPKG_INSTALLED_DIR}")
      message(STATUS "  VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")
      message(STATUS "  CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
    endif()
    add_subdirectory( ${al-core_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/_deps/al-core-build )
  else()
    FetchContent_MakeAvailable( al-core )
  endif()
  get_target_property( AL_CORE_VERSION al VERSION )
endif()


if( ${AL_PLUGINS} )
  if(WIN32)
    if( ${AL_DOWNLOAD_DEPENDENCIES} )
      # Download the AL plugins from the ITER git using direct git commands:
      set( al-plugins_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/al-plugins-src" )
      if( NOT EXISTS "${al-plugins_SOURCE_DIR}/.git" )
        message( STATUS "Cloning al-plugins from ${AL_PLUGINS_GIT_REPOSITORY}" )
        execute_process(
          COMMAND git clone "${AL_PLUGINS_GIT_REPOSITORY}" "${al-plugins_SOURCE_DIR}"
          RESULT_VARIABLE _GIT_CLONE_RESULT
          ERROR_VARIABLE _GIT_CLONE_ERROR
        )
        if( _GIT_CLONE_RESULT )
          message( FATAL_ERROR "Failed to clone al-plugins: ${_GIT_CLONE_ERROR}" )
        endif()
      endif()
      # Checkout the specified version
      execute_process(
        COMMAND git fetch origin
        WORKING_DIRECTORY "${al-plugins_SOURCE_DIR}"
        RESULT_VARIABLE _GIT_FETCH_RESULT
      )
      execute_process(
        COMMAND git checkout "${AL_PLUGINS_VERSION}"
        WORKING_DIRECTORY "${al-plugins_SOURCE_DIR}"
        RESULT_VARIABLE _GIT_CHECKOUT_RESULT
        ERROR_VARIABLE _GIT_CHECKOUT_ERROR
      )
      if( _GIT_CHECKOUT_RESULT )
        message( FATAL_ERROR "Failed to checkout ${AL_PLUGINS_VERSION}: ${_GIT_CHECKOUT_ERROR}" )
      endif()
    else()
      # Look in ../plugins
      set( al-plugins_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../al-plugins )
      if( NOT IS_DIRECTORY ${al-plugins_SOURCE_DIR} )
        message( FATAL_ERROR
          "${al-plugins_SOURCE_DIR} does not exist. Please clone the "
          "al-plugins repository or set AL_DOWNLOAD_DEPENDENCIES=ON."
        )
      endif()
    endif()

  else()
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
endif()

if( AL_HLI_DOCS )
  include( ALBuildDocumentation )
endif()

