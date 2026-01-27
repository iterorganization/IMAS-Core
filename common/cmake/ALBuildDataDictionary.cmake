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

# Find Python for the xsltproc.py program
if(WIN32)
  if(NOT Python3_FOUND AND NOT PYTHON_EXECUTABLE)
	  # Check if Python is in PATH
	  find_program(PYTHON_EXECUTABLE NAMES python3.exe python.exe python3 python DOC "Python interpreter")
	  if(NOT PYTHON_EXECUTABLE)
	    message(FATAL_ERROR "Could not find Python. Please ensure Python is installed and in PATH.")
	  endif()
  else()
	  set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
  endif()
else()
	find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
	set(PYTHON_EXECUTABLE ${Python_EXECUTABLE})
endif()

message(STATUS "Found Python: ${PYTHON_EXECUTABLE}")

# Set up Python venv paths for saxonche (used for all XSLT transformations)
if(WIN32)
  set(_VENV_PYTHON "${CMAKE_CURRENT_BINARY_DIR}/dd_build_env/Scripts/python.exe")
  set(_VENV_PIP "${CMAKE_CURRENT_BINARY_DIR}/dd_build_env/Scripts/pip.exe")
else()
  set(_VENV_PYTHON "${CMAKE_CURRENT_BINARY_DIR}/dd_build_env/bin/python")
  set(_VENV_PIP "${CMAKE_CURRENT_BINARY_DIR}/dd_build_env/bin/pip")
endif()

if( NOT AL_DOWNLOAD_DEPENDENCIES AND NOT AL_DEVELOPMENT_LAYOUT )
  # The DD easybuild module should be loaded, use that module:
  # Create Python venv first and install imas_data_dictionary
  if(NOT EXISTS "${_VENV_PYTHON}")
    execute_process(
      COMMAND ${PYTHON_EXECUTABLE} -m venv dd_build_env
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      RESULT_VARIABLE _VENV_EXITCODE
      OUTPUT_VARIABLE _VENV_OUTPUT
      ERROR_VARIABLE _VENV_ERROR
    )
    
    if(_VENV_EXITCODE)
      message(STATUS "venv stdout: ${_VENV_OUTPUT}")
      message(STATUS "venv stderr: ${_VENV_ERROR}")
      message(FATAL_ERROR "Failed to create venv (exit code: ${_VENV_EXITCODE}). Ensure Python has venv module installed: python -m venv --help")
    endif()
    
    if(DEFINED DD_VERSION)
      execute_process(
        COMMAND ${_VENV_PIP} install imas_data_dictionary==${DD_VERSION}
        RESULT_VARIABLE _PIP_EXITCODE
        OUTPUT_VARIABLE _PIP_OUTPUT
        ERROR_VARIABLE _PIP_ERROR
      )
    else()
      execute_process(
        COMMAND ${_VENV_PIP} install imas_data_dictionary
        RESULT_VARIABLE _PIP_EXITCODE
        OUTPUT_VARIABLE _PIP_OUTPUT
        ERROR_VARIABLE _PIP_ERROR
      )
    endif()
    
    if(_PIP_EXITCODE)
      message(STATUS "imas_data_dictionary pip output: ${_PIP_OUTPUT}")
      message(STATUS "imas_data_dictionary pip error: ${_PIP_ERROR}")
      message(FATAL_ERROR "Failed to install imas_data_dictionary dependency (exit code: ${_PIP_EXITCODE}). Check network connectivity and Python wheel compatibility.")
    endif()
    
    execute_process(
      COMMAND ${_VENV_PIP} install saxonche
      RESULT_VARIABLE _PIP_EXITCODE
      OUTPUT_VARIABLE _PIP_OUTPUT
      ERROR_VARIABLE _PIP_ERROR
    )
    
    if(_PIP_EXITCODE)
      message(STATUS "saxonche pip output: ${_PIP_OUTPUT}")
      message(STATUS "saxonche pip error: ${_PIP_ERROR}")
      message(FATAL_ERROR "Failed to install saxonche dependency (exit code: ${_PIP_EXITCODE}). Check network connectivity and Python wheel compatibility.")
    endif()
  endif()
# Set up idsinfo command path
if(WIN32)
  set(_IDSINFO_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/dd_build_env/Scripts/idsinfo.exe")
else()
  set(_IDSINFO_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/dd_build_env/bin/idsinfo")
endif()

  # Use idsinfo idspath command from venv to get the path to IDSDef.xml or data_dictionary.xml
  execute_process(
    COMMAND ${_IDSINFO_COMMAND} idspath
    OUTPUT_VARIABLE IDSDEF
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _IDSINFO_EXITCODE
  )
  
  if( _IDSINFO_EXITCODE )
    message( FATAL_ERROR 
      "Failed to run 'idsinfo idspath' command. "
      "Please ensure IMAS-Data-Dictionary module is loaded."
    )
  endif()
  
  if( NOT EXISTS "${IDSDEF}" )
    message( FATAL_ERROR 
      "idsinfo idspath returned '${IDSDEF}' but file does not exist. "
      "Please ensure IMAS-Data-Dictionary module is properly loaded."
    )
  endif()
  
  message( STATUS "Found Data Dictionary: ${IDSDEF}" )

  # Populate identifier source xmls based on the IDSDEF location 
  get_filename_component( DD_BASE_DIR "${IDSDEF}" DIRECTORY )
  
  if( DD_BASE_DIR MATCHES "schemas$" )
    # DD 4.1.0+ layout: resources/schemas/<ids_name>/*_identifier.xml
    file( GLOB DD_IDENTIFIER_FILES "${DD_BASE_DIR}/*/*_identifier.xml" )
  else()
    # DD 3.x/4.0.0 layout: dd_x.y.z/include/<ids_name>/*_identifier.xml
    file( GLOB DD_IDENTIFIER_FILES "${DD_BASE_DIR}/*/*_identifier.xml" )
  endif()
  
  if( NOT DD_IDENTIFIER_FILES )
    message( WARNING "No identifier XML files found in Data Dictionary at: ${IDSDEF}" )
  endif()
  
  # When using pre-installed DD, we still need venv for extracting IDS names and version
  # Create Python venv and install saxonche if not already done
  if(NOT EXISTS "${_VENV_PYTHON}")
    execute_process(
      COMMAND ${PYTHON_EXECUTABLE} -m venv dd_build_env
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      RESULT_VARIABLE _VENV_EXITCODE
      OUTPUT_VARIABLE _VENV_OUTPUT
      ERROR_VARIABLE _VENV_ERROR
    )
    
    if(_VENV_EXITCODE)
      message(STATUS "venv stdout: ${_VENV_OUTPUT}")
      message(STATUS "venv stderr: ${_VENV_ERROR}")
      message(FATAL_ERROR "Failed to create venv (exit code: ${_VENV_EXITCODE}). Ensure Python has venv module installed: python -m venv --help")
    endif()

    execute_process(
      COMMAND ${_VENV_PIP} install saxonche
      RESULT_VARIABLE _PIP_EXITCODE
      OUTPUT_VARIABLE _PIP_OUTPUT
      ERROR_VARIABLE _PIP_ERROR
    )
    
    if(_PIP_EXITCODE)
      message(STATUS "saxonche pip output: ${_PIP_OUTPUT}")
      message(STATUS "saxonche pip error: ${_PIP_ERROR}")
      message(FATAL_ERROR "Failed to install saxonche dependency (exit code: ${_PIP_EXITCODE}). Check network connectivity and Python wheel compatibility.")
    endif()
  endif()
else()
  if(WIN32)
    # Build the DD from source using direct git commands:
    if( AL_DOWNLOAD_DEPENDENCIES )
      # Download the Data Dictionary from the ITER git:
      set( data-dictionary_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/data-dictionary-src" )
      if( NOT EXISTS "${data-dictionary_SOURCE_DIR}/.git" )
        message( STATUS "Cloning data-dictionary from ${DD_GIT_REPOSITORY}" )
        execute_process(
          COMMAND git clone "${DD_GIT_REPOSITORY}" "${data-dictionary_SOURCE_DIR}"
          RESULT_VARIABLE _GIT_CLONE_RESULT
          ERROR_VARIABLE _GIT_CLONE_ERROR
        )
        if( _GIT_CLONE_RESULT )
          message( FATAL_ERROR "Failed to clone data-dictionary: ${_GIT_CLONE_ERROR}" )
        endif()
      endif()
      # Checkout the specified version
      execute_process(
        COMMAND git fetch origin
        WORKING_DIRECTORY "${data-dictionary_SOURCE_DIR}"
        RESULT_VARIABLE _GIT_FETCH_RESULT
      )
      execute_process(
        COMMAND git checkout "${DD_VERSION}"
        WORKING_DIRECTORY "${data-dictionary_SOURCE_DIR}"
        RESULT_VARIABLE _GIT_CHECKOUT_RESULT
        ERROR_VARIABLE _GIT_CHECKOUT_ERROR
      )
      if( _GIT_CHECKOUT_RESULT )
        message( FATAL_ERROR "Failed to checkout ${DD_VERSION}: ${_GIT_CHECKOUT_ERROR}" )
      endif()
    else()
      # Look in ../data-dictionary for the data dictionary
      if( NOT( AL_PARENT_FOLDER ) )
        set( AL_PARENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/.. )
      endif()
      set( data-dictionary_SOURCE_DIR ${AL_PARENT_FOLDER}/data-dictionary )
      if( NOT IS_DIRECTORY ${data-dictionary_SOURCE_DIR} )
        message( FATAL_ERROR
          "${data-dictionary_SOURCE_DIR} does not exist. Please clone the "
          "data-dictionary repository or set AL_DOWNLOAD_DEPENDENCIES=ON."
        )
      endif()
    endif() 
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
  endif()


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
  # Create Python venv and install saxonche if not already done
  if(NOT EXISTS "${_VENV_PYTHON}")
    execute_process(
      COMMAND ${PYTHON_EXECUTABLE} -m venv dd_build_env
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      RESULT_VARIABLE _VENV_EXITCODE
      OUTPUT_VARIABLE _VENV_OUTPUT
      ERROR_VARIABLE _VENV_ERROR
    )
    
    if(_VENV_EXITCODE)
      message(STATUS "venv stdout: ${_VENV_OUTPUT}")
      message(STATUS "venv stderr: ${_VENV_ERROR}")
      message(FATAL_ERROR "Failed to create venv (exit code: ${_VENV_EXITCODE}). Ensure Python has venv module installed: python -m venv --help")
    endif()

    execute_process(
      COMMAND ${_VENV_PIP} install saxonche
      RESULT_VARIABLE _PIP_EXITCODE
      OUTPUT_VARIABLE _PIP_OUTPUT
      ERROR_VARIABLE _PIP_ERROR
    )
    
    if(_PIP_EXITCODE)
      message(STATUS "saxonche pip output: ${_PIP_OUTPUT}")
      message(STATUS "saxonche pip error: ${_PIP_ERROR}")
      message(FATAL_ERROR "Failed to install saxonche dependency (exit code: ${_PIP_EXITCODE}). Check network connectivity and Python wheel compatibility.")
    endif()
  endif()
  
  execute_process(
    COMMAND ${_VENV_PYTHON} "${al-common_SOURCE_DIR}/xsltproc.py"
      -xsl "dd_data_dictionary.xml.xsl"
      -o "IDSDef.xml"
      -s "dd_data_dictionary.xml.xsd"
      DD_GIT_DESCRIBE=${DD_GIT_DESCRIBE}
    WORKING_DIRECTORY ${data-dictionary_SOURCE_DIR}
    RESULT_VARIABLE _MAKE_DD_EXITCODE
    OUTPUT_VARIABLE _MAKE_DD_OUTPUT
    ERROR_VARIABLE _MAKE_DD_ERROR
  )

  if( _MAKE_DD_EXITCODE )
    message(STATUS "xsltproc.py output: ${_MAKE_DD_OUTPUT}")
    message(STATUS "xsltproc.py error: ${_MAKE_DD_ERROR}")
    message(FATAL_ERROR "Error while building the Data Dictionary (exit code: ${_MAKE_DD_EXITCODE}). Check paths and Saxon-HE configuration.")
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
# Ensure saxonche is installed before using xsltproc.py
# Check if saxonche is available in the venv
execute_process(
  COMMAND ${_VENV_PYTHON} -c "import saxonche"
  RESULT_VARIABLE _SAXONCHE_CHECK
  OUTPUT_QUIET
  ERROR_QUIET
)

if(_SAXONCHE_CHECK)
  message(STATUS "Installing saxonche in venv...")
  execute_process(
    COMMAND ${_VENV_PIP} install saxonche
    RESULT_VARIABLE _PIP_EXITCODE
    OUTPUT_VARIABLE _PIP_OUTPUT
    ERROR_VARIABLE _PIP_ERROR
  )
  
  if(_PIP_EXITCODE)
    message(STATUS "saxonche pip output: ${_PIP_OUTPUT}")
    message(STATUS "saxonche pip error: ${_PIP_ERROR}")
    message(FATAL_ERROR "Failed to install saxonche dependency (exit code: ${_PIP_EXITCODE}). Check network connectivity and Python wheel compatibility.")
  endif()
endif()

set( list_idss_file ${al-common_SOURCE_DIR}/list_idss.xsl )
set( CMAKE_CONFIGURE_DEPENDS ${CMAKE_CONFIGURE_DEPENDS};${list_idss_file};${IDSDEF} )
set( ids_names_tmpfile "${CMAKE_CURRENT_BINARY_DIR}/ids_names_tmp.txt" )
execute_process( COMMAND
  ${_VENV_PYTHON} "${al-common_SOURCE_DIR}/xsltproc.py"
    -xsl ${list_idss_file}
    -s ${IDSDEF}
    -o ${ids_names_tmpfile}
  RESULT_VARIABLE _XSLT_RESULT
  ERROR_VARIABLE _XSLT_ERROR
)
if(_XSLT_RESULT)
  message(FATAL_ERROR "Failed to extract IDS names: ${_XSLT_ERROR}")
endif()
if(EXISTS ${ids_names_tmpfile})
  file(READ ${ids_names_tmpfile} IDS_NAMES)
  string(STRIP "${IDS_NAMES}" IDS_NAMES)
  file(REMOVE ${ids_names_tmpfile})
else()
  message(FATAL_ERROR "IDS names output file not created")
endif()
set( list_idss_file )  # unset temporary var

# DD version
set( dd_version_file ${al-common_SOURCE_DIR}/dd_version.xsl )
set( dd_version_tmpfile "${CMAKE_CURRENT_BINARY_DIR}/dd_version_tmp.txt" )
execute_process( COMMAND
  ${_VENV_PYTHON} "${al-common_SOURCE_DIR}/xsltproc.py"
    -xsl ${dd_version_file}
    -s ${IDSDEF}
    -o ${dd_version_tmpfile}
  RESULT_VARIABLE _XSLT_RESULT
  ERROR_VARIABLE _XSLT_ERROR
)
if(_XSLT_RESULT)
  message(FATAL_ERROR "Failed to extract DD version: ${_XSLT_ERROR}")
endif()
if(EXISTS ${dd_version_tmpfile})
  file(READ ${dd_version_tmpfile} DD_VERSION)
  string(STRIP "${DD_VERSION}" DD_VERSION)
  file(REMOVE ${dd_version_tmpfile})
else()
  message(FATAL_ERROR "DD version output file not created")
endif()
string( REGEX REPLACE "[+-]" "_" DD_SAFE_VERSION "${DD_VERSION}" )
set( dd_version_file )  # unset temporary var
