# CMake build configuration for Access Layer MDSplus models
cmake_minimum_required( VERSION 3.16 )

# Configuration options for common assets
################################################################################
option( AL_DOWNLOAD_DEPENDENCIES "Automatically download assets from the AL git repository" ON )
set( DD_GIT_REPOSITORY "git@github.com:ITEROrganization/IMAS-Data-Dictionary.git" CACHE STRING "Git repository of the Data Dictionary" )
set( DD_VERSION "main" CACHE STRING "Data dictionary version (tag or branch name) to use for this build" )

# Load common assets
################################################################################
add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/../../common" _common )

project( al-models-mdsplus )

# Data dictionary
include( ALBuildDataDictionary )

# Find jtraverser jar file
################################################################################

if( NOT JTRAVERSER_JAR )
  set( CLASSPATH_PATHS )
  string( REPLACE ":" ";" CLASSPATH $ENV{CLASSPATH} )
  foreach( CLASSPATH_ITEM IN LISTS CLASSPATH )
    if( CLASSPATH_ITEM MATCHES "/[*]$")
      string( REGEX REPLACE "/[*]$" "" CP_PATH ${CLASSPATH_ITEM} )
      list( APPEND CLASSPATH_PATHS ${CP_PATH} )
    else()
      # Check if it's the jTraverser.jar
      if( CLASSPATH_ITEM MATCHES "(^|/)(jTraverser|jtraverser).jar" )
        set( JTRAVERSER_JAR ${CLASSPATH_ITEM} CACHE FILEPATH "Path to MDSPLUS jTraverser.jar" )
        break()
      endif()
    endif()
  endforeach()

  if( NOT JTRAVERSER_JAR )
    find_file( JTRAVERSER_JAR
      NAMES jtraverser.jar jTraverser.jar
      PATHS "$ENV{MDSPLUS_DIR}/java/classes/" ${CLASSPATH_PATHS}
    )
  endif()
endif()

if( NOT JTRAVERSER_JAR )
  message( FATAL_ERROR "jTraverser.jar nor found, cannot build MDSplus models." )
endif()

# CompileTree class undergoes a rename to mds.jtraverser.CompileTree at some
# version of MDSplus... catch which it is
execute_process(
  COMMAND jar tf ${JTRAVERSER_JAR} CompileTree mds/jtraverser/CompileTree
  OUTPUT_VARIABLE COMPILETREE_CLASS
)
# Output should be either "CompileTree.class\n" or
# "mds/jtraverser/CompileTree.class\n". What we need is "CompileTree" or
# "mds.jtraverser.CompileTree"
string( STRIP "${COMPILETREE_CLASS}" COMPILETREE_CLASS )
string( REGEX REPLACE "[.]class$" "" COMPILETREE_CLASS "${COMPILETREE_CLASS}")
string( REPLACE "/" "." COMPILETREE_CLASS "${COMPILETREE_CLASS}")

if( NOT COMPILETREE_CLASS )
  message( FATAL_ERROR "Could not determine the MDSplus CompileTree class name, cannot build MDSplus models.")
endif()


# Build models
################################################################################

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)


execute_process(
  COMMAND ${Python_EXECUTABLE} -m venv build_env
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

execute_process(
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/build_env/bin/pip install saxonche
  RESULT_VARIABLE _PIP_EXITCODE
)

if(_PIP_EXITCODE)
  message(FATAL_ERROR "Failed to install saxonche dependency")
endif()

add_custom_command(
  OUTPUT
  ${CMAKE_CURRENT_BINARY_DIR}/ids.xml
  COMMAND
  ${CMAKE_CURRENT_BINARY_DIR}/build_env/bin/python '${CMAKE_SOURCE_DIR}/xsltproc.py'
  -xsl '${CMAKE_CURRENT_SOURCE_DIR}/IDSDef2MDSpreTree.xsl'
  -o '${CMAKE_CURRENT_BINARY_DIR}/ids.xml'
  -s '${IDSDEF}'
  'DD_GIT_DESCRIBE=${DD_VERSION}'
  'AL_GIT_DESCRIBE=${AL_VERSION}'
  DEPENDS ${IDSDEF} IDSDef2MDSpreTree.xsl
)

add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/dummy.txt
  BYPRODUCTS
    ${CMAKE_CURRENT_BINARY_DIR}/ids_model.characteristics
    ${CMAKE_CURRENT_BINARY_DIR}/ids_model.datafile
    ${CMAKE_CURRENT_BINARY_DIR}/ids_model.tree
  COMMAND
    cmake -E env ids_path=${CMAKE_CURRENT_BINARY_DIR}
      java -cp "${JTRAVERSER_JAR}" ${COMPILETREE_CLASS} ids
  COMMAND
    cmake -E touch ${CMAKE_CURRENT_BINARY_DIR}/dummy.txt
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ids.xml
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target( al-mdsplus-model ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dummy.txt
)

# Install
################################################################################

install( FILES
  ${CMAKE_CURRENT_BINARY_DIR}/ids_model.characteristics
  ${CMAKE_CURRENT_BINARY_DIR}/ids_model.datafile
  ${CMAKE_CURRENT_BINARY_DIR}/ids_model.tree
  DESTINATION models/mdsplus
)

