if(SKBUILD)
  add_subdirectory(python)
  unset(SKBUILD CACHE)
  return()
endif()

find_package(Python 3.8 REQUIRED COMPONENTS Interpreter Development.Module)
list(FILTER SKBUILD_CMAKE_ARGS EXCLUDE REGEX "CMAKE_INSTALL_PREFIX")
string(REPLACE ";" " " CMAKE_ARGS "${SKBUILD_CMAKE_ARGS}")
cmake_path(SET Python_EXECUTABLE ${Python_EXECUTABLE})


if(${AL_PYTHON_BINDINGS} MATCHES "^e$|^editable$")
  install(CODE "execute_process(
    COMMAND 
      ${CMAKE_COMMAND} -E env 
        CMAKE_ARGS=${CMAKE_ARGS} 
	SKBUILD_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/{wheel_tag} 
	SKBUILD_EDITABLE_REBUILD=true
	SKBUILD_EDITABLE_VERBOSE=false
        ${Python_EXECUTABLE} 
          -m pip install 
          --no-build-isolation 
          --editable=${CMAKE_CURRENT_SOURCE_DIR}
	  )")
  return()
endif()


set(PIP_OPTIONS "--no-deps" "--verbose") 
if(${AL_PYTHON_BINDINGS} MATCHES "^no-build-isolation$")
  list(APPEND PIP_OPTIONS "--no-build-isolation")
elseif(NOT ${AL_PYTHON_BINDINGS} MATCHES "[Oo][Nn]")
  message(FATAL_ERROR "AL_PYTHON_BINDINGS=${AL_PYTHON_BINDINGS} not e|editable|no-build-isolation|[Oo][Nn]")
endif()

add_custom_command(
  TARGET al POST_BUILD
    COMMAND 
      ${CMAKE_COMMAND} -E env 
      CMAKE_ARGS=${CMAKE_ARGS}
      SKBUILD_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/{wheel_tag} 
      ${Python_EXECUTABLE}
        -m pip wheel 
        ${CMAKE_CURRENT_SOURCE_DIR}
	${PIP_OPTIONS}
	--wheel-dir ${CMAKE_CURRENT_BINARY_DIR}/dist/
)
add_custom_target(al-python-bindings DEPENDS al)
set_target_properties(
  al-python-bindings PROPERTIES DIST_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/dist/)
install(CODE "execute_process(COMMAND ${Python_EXECUTABLE} 
-m pip install imas_core
--prefix=${CMAKE_INSTALL_PREFIX}
--find-links ${CMAKE_CURRENT_BINARY_DIR}/dist/
)"
DEPENDS al-python-bindings
)

