# Everything needed for building the Data Dictionary
#
# This script sets the following variables:
#   IDSDEF            Path to the generated IDSDef.xml
#   IDS_NAMES         List of IDSs that are available in the data dictionary
#   DD_VERSION        Version of the data dictionary
#   DD_SAFE_VERSION   DD version, safe to use as linker symbol

if( AL_DOCS_ONLY )
  return()
endif()

# Find Saxon XSLT processor
find_package( SaxonHE REQUIRED )
# Find Python for the xsltproc.py program
find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
# Find LibXslt for the xsltproc program
find_package( LibXslt QUIET )
if( NOT LIBXSLT_XSLTPROC_EXECUTABLE )
  message( FATAL_ERROR "Could not find xsltproc" )
endif()

if( NOT AL_DOWNLOAD_DEPENDENCIES AND NOT AL_DEVELOPMENT_LAYOUT )
  # The DD easybuild module should be loaded, use that module:
  if( "$ENV{IMAS_PREFIX}" STREQUAL "" OR "$ENV{IMAS_VERSION}" STREQUAL "" )
    message( FATAL_ERROR
      "Environment variables IMAS_PREFIX ('$ENV{IMAS_PREFIX}') or "
      "IMAS_VERSION ('$ENV{IMAS_VERSION}') not set."
    )
  endif()

  # Populate IDSDEF filename
  set( IDSDEF "$ENV{IMAS_PREFIX}/include/IDSDef.xml" )
  if( NOT EXISTS "${IDSDEF}" )
    message( FATAL_ERROR "Could not find IDSDef.xml at '${IDSDEF}'." )
  endif()

  # Populate identifier source xmls
  file( GLOB DD_IDENTIFIER_FILES "$ENV{IMAS_PREFIX}/include/*/*_identifier.xml" )
else()
  # Build the DD from source:
  include(FetchContent)

  if( AL_DOWNLOAD_DEPENDENCIES )
    # Download the Data Dictionary from the ITER git:
    FetchContent_Declare(
      data-dictionary
      GIT_REPOSITORY  ${DD_GIT_REPOSITORY}
      GIT_TAG         ${DD_VERSION}
    )
  else()
    # Look in ../data-dictionary for the data dictionary
    if( NOT( AL_PARENT_FOLDER ) )
      set( AL_PARENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/.. )
    endif()
    set( DD_SOURCE_DIRECTORY ${AL_PARENT_FOLDER}/data-dictionary )
    if( NOT IS_DIRECTORY ${DD_SOURCE_DIRECTORY} )
      message( FATAL_ERROR
        "${DD_SOURCE_DIRECTORY} does not exist. Please clone the "
        "data-dictionary repository or set AL_DOWNLOAD_DEPENDENCIES=ON."
      )
    endif()

    FetchContent_Declare(
      data-dictionary
      SOURCE_DIR      ${DD_SOURCE_DIRECTORY}
    )
    set( DD_SOURCE_DIRECTORY )  # unset temporary var
  endif()
  FetchContent_MakeAvailable( data-dictionary )

  # get version of the data dictionary
  execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${data-dictionary_SOURCE_DIR}
    OUTPUT_VARIABLE DD_GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _GIT_RESULT
  )
  if(_GIT_RESULT)
    execute_process(
      COMMAND git rev-parse --short HEAD
      WORKING_DIRECTORY ${data-dictionary_SOURCE_DIR}
      OUTPUT_VARIABLE DD_GIT_DESCRIBE
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()

  # We need the IDSDef.xml at configure time, ensure it is built
  execute_process(
    COMMAND ${Python_EXECUTABLE} -m venv dd_build_env
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  
  execute_process(
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/dd_build_env/bin/pip install saxonche
    RESULT_VARIABLE _PIP_EXITCODE
  )
  
  if(_PIP_EXITCODE)
    message(FATAL_ERROR "Failed to install saxonche dependency")
  endif()
  
  execute_process(
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/dd_build_env/bin/python "${al-core_SOURCE_DIR}/xsltproc.py"
      -xsl "dd_data_dictionary.xml.xsl"
      -o "IDSDef.xml"
      -s "dd_data_dictionary.xml.xsd"
      DD_GIT_DESCRIBE=${DD_GIT_DESCRIBE}
    WORKING_DIRECTORY ${data-dictionary_SOURCE_DIR}
    RESULT_VARIABLE _MAKE_DD_EXITCODE
  )

  if( _MAKE_DD_EXITCODE )
    # make did not succeed:
    message( FATAL_ERROR "Error while building the Data Dictionary. See output on previous lines." )
  endif()

  # Populate IDSDEF filename
  set( IDSDEF "${data-dictionary_SOURCE_DIR}/IDSDef.xml" )

  # Install IDSDEF (needed for some applications and for UDA backend)
  get_filename_component( REAL_IDSDEF ${IDSDEF} REALPATH )
  install( FILES ${REAL_IDSDEF} DESTINATION include RENAME IDSDef.xml )

  # Populate identifier source xmls
  file( GLOB DD_IDENTIFIER_FILES "${data-dictionary_SOURCE_DIR}/*/*_identifier.xml" "${data-dictionary_SOURCE_DIR}/schemas/*/*_identifier.xml" )
endif()

# Find out which IDSs exist and populate IDS_NAMES
set( list_idss_file ${al-common_SOURCE_DIR}/list_idss.xsl )
set( CMAKE_CONFIGURE_DEPENDS ${CMAKE_CONFIGURE_DEPENDS};${list_idss_file};${IDSDEF} )
execute_process( COMMAND
  ${LIBXSLT_XSLTPROC_EXECUTABLE} ${list_idss_file} ${IDSDEF}
  OUTPUT_VARIABLE IDS_NAMES
)
set( list_idss_file )  # unset temporary var

# DD version
set( dd_version_file ${al-common_SOURCE_DIR}/dd_version.xsl )
execute_process( COMMAND
  ${LIBXSLT_XSLTPROC_EXECUTABLE} ${dd_version_file} ${IDSDEF}
  OUTPUT_VARIABLE DD_VERSION
)
string( REGEX REPLACE "[+-]" "_" DD_SAFE_VERSION ${DD_VERSION} )
set( dd_version_file )  # unset temporary var
