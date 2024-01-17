# CMake build configuration for Access Layer MDSplus models
cmake_minimum_required( VERSION 3.16 )

# Configuration options for common assets
################################################################################
option( AL_DOWNLOAD_DEPENDENCIES "Automatically download common assets from the AL git repository" OFF )
# set( AL_COMMON_GIT_REPOSITORY "ssh://git@git.iter.org/imas/al-common.git" CACHE STRING "Git repository of AL-common")

include(FetchContent)

# Load common assets
################################################################################
set( AL_PARENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../../../ )
if( ${AL_DOWNLOAD_DEPENDENCIES} )
  # Download common assets from the ITER git:
  message( FATAL_ERROR "Not implemented yet" )
else()
  # Look in ../al-common for common files
  FetchContent_Declare(
    al-common
    SOURCE_DIR      ${AL_PARENT_FOLDER}/al-common
  )
endif()
FetchContent_MakeAvailable( al-common )

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
        set( JTRAVERSER_JAR CACHE FILEPATH "Path to MDSPLUS jTraverser.jar" ${CLASSPATH_ITEM} )
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
string( REGEX REPLACE "[.]class$" "" COMPILETREE_CLASS ${COMPILETREE_CLASS})
string( REPLACE "/" "." COMPILETREE_CLASS ${COMPILETREE_CLASS})

if( NOT COMPILETREE_CLASS )
  message( FATAL_ERROR "Could not determine the MDSplus CompileTree class name, cannot build MDSplus models.")
endif()


# Build models
################################################################################

find_package( SaxonHE )

add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/ids.xml
  COMMAND
    java -cp ${SaxonHE_CLASSPATH}
      net.sf.saxon.Transform -threads:4 -t -warnings:fatal
      "-xsl:${CMAKE_CURRENT_SOURCE_DIR}/IDSDef2MDSpreTree.xsl"
      "-s:${IDSDEF}"
      DD_GIT_DESCRIBE=${DD_VERSION}
      AL_GIT_DESCRIBE=${AL_VERSION}
      > ${CMAKE_CURRENT_BINARY_DIR}/ids.xml
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
  ids_model.characteristics
  ids_model.datafile
  ids_model.tree
  DESTINATION models/mdsplus
)

