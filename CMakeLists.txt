# CMake build configuration for Access Layer core
cmake_minimum_required(VERSION 3.21)

if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.30)
  cmake_policy(SET CMP0167 NEW) # Use system BoostConfig instead of cmake FindBoost
endif()

# Capture scikit build cmake.args
include(./skbuild-cmake-args.cmake)

# Supress a warning
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

# Configuration options for IMAS Core Backends
# ##############################################################################
option(AL_BACKEND_MDSPLUS "Build the MDSplus backend" OFF)
option(AL_BACKEND_HDF5 "Build the HDF5 backend" ON)
option(AL_BACKEND_UDA "Build the UDA backend" OFF)
option(AL_BACKEND_UDAFAT "Build the UDA backend (use FAT UDA)" OFF)
# AL_BUILD_MDSPLUS_MODELS defaults to ON when AL_BACKEND_MDSPLUS is ON, otherwise OFF.
# This can be overridde by user by setting AL_BUILD_MDSPLUS_MODELS explicitly to ON or OFF.
if(AL_BACKEND_MDSPLUS)
  option(AL_BUILD_MDSPLUS_MODELS "Build MDSplus models" ON)
else()
  option(AL_BUILD_MDSPLUS_MODELS "Build MDSplus models" OFF)
endif()

# Configuration options for python bindings
# ##############################################################################
option(AL_PYTHON_BINDINGS "Build Python bindings" OFF)

option(AL_BUILD_TESTS "Build the C/C++ low-level ABI smoke tests (tests/)" OFF)

# When ON, the al C++ library is not built here; instead it is located with
# find_package(al-core CONFIG). Enables a two-stage build where libimas-core
# (the C/C++ library) and imas-core (the Python wrapper) are packaged
# separately, e.g. as two conda packages.
option(AL_USE_INSTALLED_CORE
       "Link Python bindings against a pre-installed al-core (find_package)" OFF)


# Configuration options for shared libraries
# ##############################################################################
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(CMAKE_INSTALL_RPATH_USE_LINK_PATH
       "Append the runtime search path (rpath) to external installed binaries" TRUE)


# Configuration option for automatic download of common third party libraries
# ##############################################################################
# include(./vcpkg.cmake)


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

if(APPLE AND CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
  # Disable MDSPlus:
  message(STATUS "Disabling MDSPlus backend on macOS")
  set(AL_BACKEND_MDSPLUS OFF CACHE BOOL "MDSPlus backend" FORCE)    
endif()


# Dependencies
# ##############################################################################

if(WIN32)
  # Ensure vcpkg paths are in CMAKE_PREFIX_PATH for finding packages
  if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    list(APPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
    message(STATUS "al-core: Added CMAKE_PREFIX_PATH: ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
  else()
    # Try to auto-detect vcpkg installed directory from build path
    if(CMAKE_CURRENT_BINARY_DIR MATCHES "(.*/build)/")
      set(_BUILD_DIR "${CMAKE_MATCH_1}")
      set(_VCPKG_PATH "${_BUILD_DIR}/vcpkg_installed/x64-windows")
      if(EXISTS "${_VCPKG_PATH}")
        list(APPEND CMAKE_PREFIX_PATH "${_VCPKG_PATH}")
        message(STATUS "al-core: Auto-detected vcpkg path: ${_VCPKG_PATH}")
        
        # Set PKG_CONFIG_EXECUTABLE for vcpkg's pkgconf
        if(EXISTS "${_VCPKG_PATH}/tools/pkgconf/pkgconf.exe")
          set(PKG_CONFIG_EXECUTABLE "${_VCPKG_PATH}/tools/pkgconf/pkgconf.exe" CACHE FILEPATH "pkg-config executable")
          message(STATUS "al-core: Set PKG_CONFIG_EXECUTABLE to ${PKG_CONFIG_EXECUTABLE}")
        endif()
      endif()
    endif()
  endif()
  
  message(STATUS "al-core: CMAKE_PREFIX_PATH = ${CMAKE_PREFIX_PATH}")
  
  # Add cmake module path for FindPThreads4W.cmake fallback
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/common/cmake")
  
  # Try CONFIG mode first, fall back to MODULE mode
  find_package(PThreads4W CONFIG QUIET)
  if(NOT PThreads4W_FOUND)
    message(STATUS "PThreads4W CONFIG not found, trying MODULE mode")
    find_package(PThreads4W MODULE REQUIRED)
  else()
    message(STATUS "Found PThreads4W via CONFIG mode")
  endif()
  
  find_package(dlfcn-win32 CONFIG REQUIRED)
endif()

# Two-stage build entry: locate a pre-installed al-core instead of building it.
# Exposes target `al-core::al` and an unqualified alias `al` so the python/
# subdirectory and any consumers can link to `al` transparently.
if(AL_USE_INSTALLED_CORE)
  find_package(al-core CONFIG)
  if(NOT al-core_FOUND)
    message(FATAL_ERROR
      "AL_USE_INSTALLED_CORE=ON requires an installed al-core that ships "
      "al-coreConfig.cmake (introduced in IMAS-Core 5.7.1 or later). "
      "find_package(al-core CONFIG) could not locate it.\n"
      "Point CMAKE_PREFIX_PATH (or al-core_DIR) at an install prefix that "
      "contains lib/cmake/al-core/al-coreConfig.cmake — typically the "
      "stage-1 install tree of this branch. Releases built before this "
      "change (e.g. IMAS-Core/5.6.0) only ship al-core.pc and will NOT "
      "satisfy AL_USE_INSTALLED_CORE; either install stage 1 from source "
      "or wait for a release that includes the CMake package config.")
  endif()
  if(NOT TARGET al)
    add_library(al ALIAS al-core::al)
  endif()
endif()

# build AL_CORE library only if backend is enabled and we are not reusing an
# already-installed al-core
if((AL_BACKEND_HDF5 OR AL_BACKEND_MDSPLUS OR AL_BACKEND_UDA OR AL_BACKEND_UDAFAT OR AL_PYTHON_BINDINGS) AND NOT AL_USE_INSTALLED_CORE)

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
    include/fix_include_windows.h
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

# Dummy Executable
# ##############################################################################
set( EXE_FLAG  )
set( EXE_EXT  )
add_executable( imas_print_version ${EXE_FLAG} tests/imas_print_version.cpp )
target_link_libraries( imas_print_version PRIVATE al )
if(NOT WIN32 AND NOT APPLE)
  # stdc++fs is only needed on Linux with older GCC
  # On macOS, filesystem is part of libc++
  # On Windows, filesystem is part of the standard library
  target_link_libraries( imas_print_version PRIVATE stdc++fs )
endif()
add_dependencies( imas_print_version al )

# Low-level ABI smoke tests (opt-in)
# ##############################################################################
if( AL_BUILD_TESTS )
  enable_testing()
  add_subdirectory(tests)
endif()

# Install
# ##############################################################################

# Install al library
include(GNUInstallDirs)
install(
  TARGETS al
  EXPORT al-coreTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT core
)

# Export CMake package config so downstream projects (notably the Python
# wrapper built with AL_USE_INSTALLED_CORE=ON) can `find_package(al-core CONFIG)`.
include(CMakePackageConfigHelpers)
set(_al_core_cmake_dir ${CMAKE_INSTALL_LIBDIR}/cmake/al-core)
install(
  EXPORT al-coreTargets
  FILE al-coreTargets.cmake
  NAMESPACE al-core::
  DESTINATION ${_al_core_cmake_dir}
)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/al-coreConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/al-coreConfig.cmake
  INSTALL_DESTINATION ${_al_core_cmake_dir}
)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/al-coreConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/al-coreConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/al-coreConfigVersion.cmake
  DESTINATION ${_al_core_cmake_dir}
)

# TODO: put public heades in a separate directory?
install(
  FILES ${PUBLIC_HEADER_FILES}
  TYPE INCLUDE
)
target_include_directories(
  al
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:include> # <prefix>/include
)

# Generate and install .pc file
configure_file(pkgconfig/al-core.pc.in al-core.pc @ONLY)
include(GNUInstallDirs)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/al-core.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# Install mdsplusIMASDB4to5
install(
  PROGRAMS mdsplusIMASDB4to5
  TYPE BIN
)

# Install common files
install(DIRECTORY common TYPE DATA)

# Install Dummy
install(TARGETS imas_print_version DESTINATION bin)

endif() # AL core library: (AL_BACKEND_* OR AL_PYTHON_BINDINGS) AND NOT AL_USE_INSTALLED_CORE

# Scikit-build-core entry point for python bindings — works whether `al` was
# just built here or imported via find_package(al-core).
# ##############################################################################
if(AL_PYTHON_BINDINGS)
  include(skbuild.cmake)
endif()


# MDSplus models
# ##############################################################################

if(AL_BUILD_MDSPLUS_MODELS)
  add_subdirectory(models/mdsplus)
endif()
