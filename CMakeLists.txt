# CMake build configuration for Access Layer core
cmake_minimum_required(VERSION 3.21)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.30)
  cmake_policy(SET CMP0167 NEW) # Use system BoostConfig instead of cmake FindBoost
endif()

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
add_subdirectory(common)

# Stop here when building documentation only
if( AL_DOCS_ONLY )
  return()
endif()

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
    include/extended_access_layer_plugin.h
    include/al_backend.h
    include/al_const.h
    include/al_context.h
    include/al_exception.h
    include/al_lowlevel.h
    include/provenance_plugin_feature.h
    include/readback_plugin_feature.h
    include/uri_parser.h
    include/data_interpolation.h
    # al_defs.h is generated in the binary folder with configure_file:
    ${CMAKE_CURRENT_BINARY_DIR}/include/al_defs.h)

# Define library target
add_library(al SHARED)
# Note: sources are added by the CMakeFiles in the src/ folder
target_link_libraries(al PRIVATE Boost::filesystem)
set_target_properties(al PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

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
install(
  TARGETS al
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT core
)

# TODO: put public heades in a separate directory?
install(
  FILES ${PUBLIC_HEADER_FILES}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
target_include_directories(
  al
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:include> # <prefix>/include
)

# Generate and install .pc file
configure_file(pkgconfig/al-core.pc.in al-core.pc @ONLY)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/al-core.pc
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# Install mdsplusIMASDB4to5
install(
  PROGRAMS mdsplusIMASDB4to5
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install common files (these are used by EB modules)
# Note: EB module has to point the env variable $AL_COMMON_PATH to this installed dir:
install(DIRECTORY common TYPE DATA)


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
