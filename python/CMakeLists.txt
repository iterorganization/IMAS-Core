# CMake build configuration for the Access Layer lowlevel Python bindings
cmake_minimum_required( VERSION 3.16 )

# TODO: support standalone building

# Define project
################################################################################

# Set VERSION and FULL_VERSION from `git describe`:
include( ALDetermineVersion )
project( al-python-bindings
  VERSION ${VERSION}
  DESCRIPTION "Python bindings to the IMAS Access Layer lowlevel"
  HOMEPAGE_URL "https://imas.iter.org/"
  LANGUAGES C CXX
)
if( NOT PROJECT_VERSION_TWEAK EQUAL 0 )
  message( "Building a development version of the Access Layer Python bindings" )
endif()


# Dependencies
################################################################################

find_package( Python3 REQUIRED )

get_target_property( AL_SOURCE_DIR al SOURCE_DIR )
get_target_property( AL_BINARY_DIR al BINARY_DIR )
set( AL_INCLUDE_PATH "${AL_SOURCE_DIR}/include:${AL_BINARY_DIR}/include" )
set( AL_LIBRARY_PATH "${AL_BINARY_DIR}" )


# Targets
################################################################################

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
    DEPENDS al
)

set( PYTHON_FILES
    al_lowlevel/__init__.py
    al_lowlevel/_al_lowlevel.pyx
    al_lowlevel/al_defs.pxd
    al_lowlevel/al_defs.pyx
    al_lowlevel/al_lowlevel_interface.pxd
    al_lowlevel/exception.py
    al_lowlevel/imasdef.py
    setup.py
    MANIFEST.in
)

add_custom_target( al-python-bindings ALL
    ${CMAKE_COMMAND} -E echo ""
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dist.txt al ${PYTHON_FILES}
)
set_target_properties( al-python-bindings PROPERTIES DIST_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/dist/ )
