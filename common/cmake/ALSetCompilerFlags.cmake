# Compiler flags for building the Access Layer

include_guard( DIRECTORY )

################################################################################
# Compile definitions for Fortran interface

if( CMAKE_SYSTEM_NAME STREQUAL Linux )
  add_compile_definitions( $<$<COMPILE_LANGUAGE:Fortran>:_Linux> )
elseif( CMAKE_SYSTEM_NAME STREQUAL Darwin )
  add_compile_definitions( $<$<COMPILE_LANGUAGE:Fortran>:_MacOS> )
elseif( CMAKE_SYSTEM_NAME STREQUAL Windows )
  add_compile_definitions( $<$<COMPILE_LANGUAGE:Fortran>:_Windows> )
else()
  message( WARNING "Unknown CMAKE_SYSTEM_NAME '${CMAKE_SYSTEM_NAME}', continuing..." )
endif()


################################################################################
# General options

set( CMAKE_POSITION_INDEPENDENT_CODE ON )

# Set default build type to RelWithDebInfo (optimize build, keep debugging symbols)
if( NOT CMAKE_BUILD_TYPE )
  set( CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE
  )
endif()

################################################################################
# C++
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set( CMAKE_CXX_STANDARD 17 )
endif()
if( CMAKE_CXX_COMPILER_ID STREQUAL "Intel" )
  # icpc options
  string( APPEND CMAKE_CXX_FLAGS
    # " -Wall"
  )
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
  # g++ options
  string( APPEND CMAKE_CXX_FLAGS
    # " -Wall -Wextra"
  )
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )
  # Visual Studio C++ options
  string( APPEND CMAKE_CXX_FLAGS
    # " -Wall"
  )
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" )
  # Apple Clang C++ options
  string( APPEND CMAKE_CXX_FLAGS
    # " -Wall"
  )
else()
  message( WARNING "Unsupported C++ compiler: ${CMAKE_CXX_COMPILER_ID}" )
endif()


################################################################################
# Fortran

get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
if( "Fortran" IN_LIST languages )

  set( CMAKE_Fortran_FORMAT FREE )
  set( CMAKE_Fortran_PREPROCESS ON )
  set( CMAKE_Fortran_MODULE_DIRECTORY include )

  if( CMAKE_Fortran_COMPILER_ID STREQUAL "Intel" )
    # ifort options
    string( APPEND CMAKE_Fortran_FLAGS
      " -r8 -assume no2underscore"
    )
    # string( APPEND CMAKE_Fortran_FLAGS " -warn all" )
  elseif( CMAKE_Fortran_COMPILER_ID STREQUAL "GNU" )
    # gfortran options
    string( APPEND CMAKE_Fortran_FLAGS
      " -fdefault-real-8 -fdefault-double-8 -fno-second-underscore -ffree-line-length-none"
    )
    # string( APPEND CMAKE_Fortran_FLAGS " -Wall -Wextra" )
  elseif( CMAKE_Fortran_COMPILER_ID STREQUAL "NAG" )
    # nagfort options
    string( APPEND CMAKE_Fortran_FLAGS
      " -maxcontin=4000 -w=unused -w=x95 -kind=byte -r8"
    )
    # Set CONFIG options for NAG, CMake's definition keep them all empty
    string( APPEND CMAKE_Fortran_FLAGS_DEBUG " -g" )
    string( APPEND CMAKE_Fortran_FLAGS_RELWITHDEBINFO " -O2 -g" )
    string( APPEND CMAKE_Fortran_FLAGS_RELEASE " -O3" )
  elseif( CMAKE_Fortran_COMPILER_ID STREQUAL "PGI" )
    # PGI / nvfortran options
    string( APPEND CMAKE_Fortran_FLAGS
      " -r8 -Mnosecond_underscore"
    )
  else()
    message( WARNING "Unsupported Fortran compiler: ${CMAKE_Fortran_COMPILER_ID}" )
  endif()

endif()

################################################################################
# Windows support

# TODO: salvaged from old CMakeLists
# Need to figure out if this is needed and functional before uncommenting

# add_definitions( -D_CRT_SECURE_NO_WARNINGS )
# add_compile_options( /bigobj )

# set( CMAKE_FIND_LIBRARY_SUFFIXES ".dll.lib" ".lib" )
# set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO" )
# set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO /SUBSYSTEM:CONSOLE" )
# set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++14 /Zc:__cplusplus" )
