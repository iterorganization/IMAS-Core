# CMake build configuration for Access Layer lowlevel
cmake_minimum_required( VERSION 3.16 )

# Configuration options for common assets
################################################################################
option( AL_DOWNLOAD_DEPENDENCIES "Automatically download common assets from the AL git repository" OFF )
# option( AL_COMMON_GIT_REPOSITORY "Git repository of AL-common" "ssh://git@git.iter.org/imas/al-common.git")

include(FetchContent)

# Load common assets
################################################################################
if( ${AL_DOWNLOAD_DEPENDENCIES} )
  # Download common assets from the ITER git:
  message( FATAL_ERROR "Not implemented yet" )
else()
  # Look in ../al-common for common files
  FetchContent_Declare(
    al-common
    SOURCE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/../al-common
  )
endif()
FetchContent_MakeAvailable( al-common )


# Lowlevel options
################################################################################

option( BUILD_SHARED_LIBS "Build shared libraries" ON )
option( AL_BACKEND_MDSPLUS "Build the MDSplus backend" OFF )
option( AL_BACKEND_HDF5 "Build the HDF5 backend" ON )
option( AL_BACKEND_UDA "Build the UDA backend" OFF )
option( AL_BACKEND_UDAFAT "Build the UDA backend (use FAT UDA)" OFF )

include(CMakeDependentOption)
cmake_dependent_option( AL_BUILD_MDSPLUS_MODELS "Build MDSplus models" ON AL_BACKEND_MDSPLUS OFF )

# Define project
################################################################################

# Set VERSION and FULL_VERSION from `git describe`:
include( ALDetermineVersion )
project( al-lowlevel
  VERSION ${VERSION}
  DESCRIPTION "Lowlevel component of the IMAS Access Layer"
  HOMEPAGE_URL "https://imas.iter.org/"
  LANGUAGES C CXX
)
if( NOT PROJECT_VERSION_TWEAK EQUAL 0 )
  message( "Building a development version of the Access Layer lowlevel" )
endif()

# Compiler settings
include( ALSetCompilerFlags )

# Dependencies
################################################################################

IF( WIN32 )
  find_package( pthreads REQUIRED )
  find_package( dlfcn-win32 CONFIG REQUIRED )
endif()

# Core dependencies
set( Boost_USE_MULTITHREADED FALSE )
find_package( Boost COMPONENTS filesystem REQUIRED )

# Backend-specific dependencies
if( AL_BACKEND_MDSPLUS )
  find_package( MDSplus REQUIRED )
endif()

if( AL_BACKEND_UDA OR AL_BACKEND_UDAFAT )
  # Use pkg config to find uda requirements
  find_package( PkgConfig REQUIRED )
  if( IMAS_UDAFAT )
    pkg_check_modules( uda REQUIRED IMPORTED_TARGET uda-fat-cpp )
  else()
    pkg_check_modules( uda REQUIRED IMPORTED_TARGET uda-cpp )
  endif()
  # if( IMAS_UDAFAT )
  #   find_package( PostgreSQL REQUIRED )
  # endif()
endif()

if( AL_BACKEND_HDF5 )
  find_package( HDF5 COMPONENTS C HL REQUIRED )
endif()


# Target
################################################################################

configure_file( "al_defs.h.in" "al_defs.h" @ONLY )

set( PUBLIC_HEADER_FILES
  access_layer_base_plugin.h
  access_layer_plugin.h
  access_layer_plugin_manager.h
  al_backend.h
  al_const.h
  al_context.h
  # al_defs.h is generated in the binary folder with configure_file:
  ${CMAKE_CURRENT_BINARY_DIR}/al_defs.h
  al_exception.h
  al_lowlevel.h
  provenance_plugin_feature.h
  readback_plugin_feature.h
  uri_parser.h
)

# Define library target
add_library( al
  al_backend.cpp
  al_lowlevel.cpp
  al_context.cpp
  al_const.cpp
  al_exception.cpp
  no_backend.cpp
  memory_backend.cpp
  ascii_backend.cpp
  access_layer_plugin_manager.cpp
)
target_link_libraries( al PRIVATE Boost::filesystem dl )
set_target_properties( al PROPERTIES VERSION ${PROJECT_VERSION} )

# Enable ASCII backend
target_compile_definitions( al PRIVATE -DASCII )

# Optional backends
if( AL_BACKEND_MDSPLUS )
  target_sources( al PRIVATE mdsplus_backend.cpp )
  target_compile_definitions( al PRIVATE -DMDSPLUS )
  if( WIN32 )
    target_compile_definitions( al PRIVATE -DMDSOBJECTSCPPSHRVS_EXPORTS )
  endif()

  target_link_libraries( al PRIVATE ${MDSPLUS_LIBRARIES} )
endif()

if( AL_BACKEND_UDA OR AL_BACKEND_UDAFAT )
  target_sources( al PRIVATE
    uda_backend.cpp
    uda_cache.cpp
    uda_path.cpp
    uda_xml.cpp
    pugixml.cpp
  )
  target_compile_definitions( al PRIVATE -DUDA )

  target_link_libraries( al PRIVATE PkgConfig::uda )
endif()

if( AL_BACKEND_HDF5 )
  target_sources( al PRIVATE
    hdf5_backend.cpp
    hdf5_writer.cpp
    hdf5_reader.cpp
    hdf5_utils.cpp
    hdf5_hs_selection_reader.cpp
    hdf5_hs_selection_writer.cpp
    hdf5_dataset_handler.cpp
    hdf5_events_handler.cpp
    hdf5_backend_factory.cpp
  )
  target_compile_definitions( al PRIVATE -DHDF5 )

  target_include_directories( al PRIVATE ${HDF5_C_INCLUDE_DIRS} )
  target_link_libraries( al PRIVATE ${HDF5_C_LIBRARIES} )
  target_compile_definitions( al PRIVATE ${HDF5_C_DEFINITIONS} )
  # hdf5::hdf5 imported target is only available in CMake 3.19+
  # target_link_libraries( al PRIVATE hdf5::hdf5 )
endif()

if( WIN32 )
  target_link_libraries( al PRIVATE dlfcn-win32::dl ${PThreads4W_LIBRARY} ws2_32 )
endif()


# Documentation
################################################################################

find_package(Doxygen)
if( DOXYGEN_FOUND )
  doxygen_add_docs(al-lowlevel-doxygen)
endif()


# Install
################################################################################

# Install library (.so)
install( TARGETS al )

# TODO: put public heades in a separate directory?
install( FILES ${PUBLIC_HEADER_FILES} DESTINATION include )
target_include_directories(al PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:include>  # <prefix>/include
)

# Generate and install .pc file
configure_file( al-lowlevel.pc.in al-lowlevel.pc @ONLY )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/al-lowlevel.pc DESTINATION lib/pkgconfig )


# Tests
################################################################################

# Tests were disabled in the Makefile as well


# MDSplus models?
################################################################################

if( AL_BUILD_MDSPLUS_MODELS )
  add_subdirectory( models/mdsplus )
endif()
