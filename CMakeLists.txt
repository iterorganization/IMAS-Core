# CMake build configuration for Access Layer core
cmake_minimum_required(VERSION 3.21)

# Capture scikit build cmake.args 
include(./skbuild-cmake-args.cmake)


# Configuration options for IMAS Core Backends
# ##############################################################################
option(AL_BACKEND_MDSPLUS "Build the MDSplus backend" OFF)
option(AL_BACKEND_HDF5 "Build the HDF5 backend" ON)
option(AL_BACKEND_UDA "Build the UDA backend" OFF)
option(AL_BACKEND_UDAFAT "Build the UDA backend (use FAT UDA)" OFF)
include(CMakeDependentOption)
cmake_dependent_option(AL_BUILD_MDSPLUS_MODELS "Build MDSplus models" ON 
AL_BACKEND_MDSPLUS OFF)

# Configuration options for python bindings
# ##############################################################################
option(AL_PYTHON_BINDINGS "Build Python bindings" OFF)


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


# Configuration options for shared libraries
# ##############################################################################
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(CMAKE_INSTALL_RPATH_USE_LINK_PATH 
       "Append the runtime search path (rpath) to external installed binaries" TRUE)

  
# Configuration option for automatic download of common third party libraries 
# ##############################################################################
include(./vcpkg.cmake)


# Load common assets
# ##############################################################################
include(FetchContent)
if(AL_DOWNLOAD_DEPENDENCIES)
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
if(SKBUILD)
  set(VERSION ${SKBUILD_PROJECT_VERSION})
  set(FULL_VERSION ${SKBUILD_PROJECT_VERSION_FULL})
else()
  # Set VERSION and FULL_VERSION from `git describe`:
  include(./GitArchiveDescribe.cmake)
  include(ALDetermineVersion)
endif()
project(
  al-core
  VERSION ${VERSION}
  DESCRIPTION "Core component of the IMAS Access Layer"
  HOMEPAGE_URL "https://imas.iter.org/"
  LANGUAGES C CXX)
if(NOT PROJECT_VERSION_TWEAK EQUAL 0)
  message("Building a development version of the Access Layer core")
endif()


# Dependencies
# ##############################################################################

if(WIN32)
  find_package(PThreads4W REQUIRED)
  find_package(dlfcn-win32 CONFIG REQUIRED)
endif()

# Core dependencies
set(Boost_USE_MULTITHREADED FALSE)
find_package(
  Boost
  COMPONENTS filesystem
  REQUIRED)


# Target
# ##############################################################################

# Compiler settings
include(ALSetCompilerFlags)

configure_file("include/al_defs.h.in" "include/al_defs.h" @ONLY)

set(PUBLIC_HEADER_FILES
    include/access_layer_base_plugin.h
    include/access_layer_plugin.h
    include/access_layer_plugin_manager.h
    include/al_backend.h
    include/al_const.h
    include/al_context.h
    include/al_exception.h
    include/al_lowlevel.h
    include/provenance_plugin_feature.h
    include/readback_plugin_feature.h
    include/uri_parser.h
    # al_defs.h is generated in the binary folder with configure_file:
    ${CMAKE_CURRENT_BINARY_DIR}/include/al_defs.h)

# Define library target
add_library(al SHARED)
# Note: sources are added by the CMakeFiles in the src/ folder
target_link_libraries(al PRIVATE Boost::filesystem)
set_target_properties(al PROPERTIES VERSION ${PROJECT_VERSION})

# Enable ASCII backend
target_compile_definitions(al PRIVATE -DASCII)

if(NOT WIN32)
  target_link_libraries(al PRIVATE dl)
else()
  target_link_libraries(al PRIVATE dlfcn-win32::dl PThreads4W::PThreads4W ws2_32)
endif()

add_subdirectory(src)

# Install
# ##############################################################################

# Install al library
install(TARGETS al COMPONENT core)

# TODO: put public heades in a separate directory?
install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include)
target_include_directories(
  al
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:include> # <prefix>/include
)

# Generate and install .pc file
configure_file(pkgconfig/al-core.pc.in al-core.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/al-core.pc DESTINATION lib/pkgconfig)

# Install mdsplusIMASDB4to5
install(PROGRAMS mdsplusIMASDB4to5 DESTINATION bin)


# Scikit-build-core entry point for python bindings
# ##############################################################################
if(AL_PYTHON_BINDINGS)
  include(skbuild.cmake)
endif()


# MDSplus models
# ##############################################################################

if(AL_BUILD_MDSPLUS_MODELS)
  add_subdirectory(models/mdsplus)
endif()
