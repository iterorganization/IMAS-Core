# CMake build configuration for the Access Layer core Python bindings
cmake_minimum_required( VERSION 3.16 )

# Configuration options for common assets
################################################################################
option( AL_DOWNLOAD_DEPENDENCIES "Automatically download common assets from the AL git repository" ON )
set( AL_COMMON_GIT_REPOSITORY "ssh://git@git.iter.org/imas/al-common.git" CACHE STRING "Git repository of AL-common" )
set( AL_COMMON_VERSION "main" CACHE STRING "Git commit/tag/branch of AL-common" )

include(FetchContent)

# Load common assets
################################################################################
set( AL_PARENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../../ )
if( ${AL_DOWNLOAD_DEPENDENCIES} )
  # Download common assets from the ITER git:
  FetchContent_Declare(
    al-common
    GIT_REPOSITORY "${AL_COMMON_GIT_REPOSITORY}"
    GIT_TAG "${AL_COMMON_VERSION}"
  )
else()
  # Look in $AL_COMMON_PATH or ../al-common for common files
  if( DEFINED ENV{AL_COMMON_PATH} )
    set( AL_COMMON_PATH $ENV{AL_COMMON_PATH} )
  else()
    set( AL_COMMON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../al-common" )
  endif()
  FetchContent_Declare(
    al-common
    SOURCE_DIR      "${AL_COMMON_PATH}"
  )
endif()
FetchContent_MakeAvailable( al-common )

# Define project
################################################################################

# Set VERSION and FULL_VERSION from `git describe`:
include( ../GitArchiveDescribe.cmake )
include( ALDetermineVersion )
project( al-python-bindings
  VERSION ${VERSION}
  DESCRIPTION "Python bindings to the IMAS Access Layer core"
  HOMEPAGE_URL "https://imas.iter.org/"
  LANGUAGES C CXX
)
if( NOT PROJECT_VERSION_TWEAK EQUAL 0 )
  message( "Building a development version of the Access Layer Python bindings" )
endif()


# Dependencies
################################################################################

find_package( Python3 REQUIRED COMPONENTS Interpreter Development )

if( NOT TARGET al )
  # Access Layer should be available as a module, ask PkgConfig for include and library
  # paths
  find_package( PkgConfig )
  pkg_check_modules( al REQUIRED IMPORTED_TARGET al-core )
  add_library( al ALIAS PkgConfig::al )
  string( REPLACE ";" ":" AL_INCLUDE_PATH "${al_INCLUDE_DIRS};${Python3_INCLUDE_DIRS}" )
  string( REPLACE ";" ":" AL_LIBRARY_PATH "${al_LIBRARY_DIRS}" )
else()
  # `al` target exists:
  get_target_property( AL_SOURCE_DIR al SOURCE_DIR )
  get_target_property( AL_BINARY_DIR al BINARY_DIR )
  set( AL_INCLUDE_PATH "${AL_SOURCE_DIR}/include:${AL_BINARY_DIR}/include" )
  set( AL_LIBRARY_PATH $<TARGET_FILE_DIR:al> )
endif()


# Targets
################################################################################

set( PYTHON_FILES
    imas_core/__init__.py
    imas_core/_al_lowlevel.pyx
    imas_core/al_defs.pxd
    imas_core/al_defs.pyx
    imas_core/al_lowlevel_interface.pxd
    imas_core/exception.py
    imas_core/imasdef.py
    setup.py
    MANIFEST.in
)

# Use python's build package without build isolation.
# This requires the following pip packages installed:
# - build
# - wheel
# - cython
# - numpy

add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dist.txt
    COMMAND ${CMAKE_COMMAND} -E env
        # Pass version to setup.py
        AL_PROJECT_VERSION=${FULL_VERSION}
        # Include folder for AL headers
        AL_INCLUDE_PATH=${AL_INCLUDE_PATH}
        # Library path for AL library
        AL_LIBRARY_PATH=${AL_LIBRARY_PATH}
        ${Python3_EXECUTABLE} -m build --no-isolation . -o ${CMAKE_CURRENT_BINARY_DIR}/dist/
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/dist.txt
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS al ${PYTHON_FILES}
)

add_custom_target( al-python-bindings ALL
    ${CMAKE_COMMAND} -E echo ""
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dist.txt al ${PYTHON_FILES}
)
set_target_properties( al-python-bindings PROPERTIES DIST_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/dist/ )

# Install
################################################################################

install( CODE "
  execute_process( COMMAND ${Python3_EXECUTABLE} -m pip install imas_core
    --prefix ${CMAKE_INSTALL_PREFIX}
    --no-deps
    --upgrade
    --no-index
    --find-links ${CMAKE_CURRENT_BINARY_DIR}/dist/
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
  )
  "
)
