# Extract CMAKE CACHE_VARS Store user-defined ARGS in SKBUILD_CMAKE_ARGS
# SKBUILD_CMAKE_ARGS are forwarded onto python build-backend Extension of
# https://stackoverflow.com/questions/10205986/how-to-capture-cmake-command-line-arguments
get_cmake_property(CACHE_VARS CACHE_VARIABLES)
foreach(CACHE_VAR ${CACHE_VARS})
  get_property(CACHE_VAR_HELPSTRING CACHE ${CACHE_VAR} PROPERTY HELPSTRING)
  if(CACHE_VAR_HELPSTRING STREQUAL
     "No help, variable specified on the command line." OR 
       CACHE_VAR IN_LIST SKBUILD_CACHE)
    if(NOT CACHE_VAR IN_LIST SKBUILD_CACHE)
      list(APPEND SKBUILD_CACHE ${CACHE_VAR})
    endif()
    get_property(
      CACHE_VAR_TYPE
      CACHE ${CACHE_VAR}
      PROPERTY TYPE)
    if(CACHE_VAR_TYPE STREQUAL "UNINITIALIZED")
      set(CACHE_VAR_TYPE)
    else()
      set(CACHE_VAR_TYPE :${CACHE_VAR_TYPE})
    endif()
    if(DEFINED SKBUILD_CMAKE_ARGS)
      set(SKBUILD_CMAKE_ARGS "${SKBUILD_CMAKE_ARGS};")
    endif()
    set(SKBUILD_CMAKE_ARGS "${SKBUILD_CMAKE_ARGS}-D${CACHE_VAR}${CACHE_VAR_TYPE}=${${CACHE_VAR}}")
  endif()
endforeach()
set(SKBUILD_CACHE ${SKBUILD_CACHE} CACHE STRING "Scikit-build-core cmake.args" FORCE)
