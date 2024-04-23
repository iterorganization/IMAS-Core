# CMake build configuration for the Access Layer lowlevel Python bindings
cmake_minimum_required(VERSION 3.18)

# Configuration options for common assets
# ##############################################################################
option(AL_DOWNLOAD_DEPENDENCIES
       "Automatically download common assets from the AL git repository" ON)
set(AL_COMMON_GIT_REPOSITORY
    "ssh://git@git.iter.org/imas/al-common.git"
    CACHE STRING "Git repository of AL-common")
set(AL_COMMON_VERSION
    "main"
    CACHE STRING "Git commit/tag/branch of AL-common")

include(FetchContent)

# Load common assets
# ##############################################################################
set(AL_PARENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../../)
if(${AL_DOWNLOAD_DEPENDENCIES})
  # Download common assets from the ITER git:
  FetchContent_Declare(
    al-common
    GIT_REPOSITORY "${AL_COMMON_GIT_REPOSITORY}"
    GIT_TAG "${AL_COMMON_VERSION}")
else()
  # Look in $AL_COMMON_PATH or ../al-common for common files
  if(DEFINED ENV{AL_COMMON_PATH})
    set(AL_COMMON_PATH $ENV{AL_COMMON_PATH})
  else()
    set(AL_COMMON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../al-common")
  endif()
  FetchContent_Declare(al-common SOURCE_DIR "${AL_COMMON_PATH}")
endif()
FetchContent_MakeAvailable(al-common)

# Define project
# ##############################################################################

project(
  ${SKBUILD_PROJECT_NAME}
  VERSION ${SKBUILD_PROJECT_VERSION}
  DESCRIPTION "Python bindings to the IMAS Access Layer core"
  HOMEPAGE_URL "https://imas.iter.org/"
  LANGUAGES C)
if(NOT PROJECT_VERSION_TWEAK EQUAL 0)
  message("Building a development version of the Access Layer Python bindings")
endif()

# Dependencies
# ##############################################################################

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module NumPy)
find_package(Cython REQUIRED)
add_subdirectory(imas_core)
