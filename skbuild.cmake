if(SKBUILD)
  add_subdirectory(python)
  unset(SKBUILD CACHE)
  return()
endif()

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
list(FILTER SKBUILD_CMAKE_ARGS EXCLUDE REGEX "CMAKE_INSTALL_PREFIX")
string(REPLACE ";" " " CMAKE_ARGS "${SKBUILD_CMAKE_ARGS}")
cmake_path(SET Python_EXECUTABLE ${Python_EXECUTABLE})

set(PYTHON_VENV "${CMAKE_CURRENT_BINARY_DIR}/venv")
set(PYTHON_VENV_BIN "${PYTHON_VENV}/bin")

if(${AL_PYTHON_BINDINGS} MATCHES "^[Ee]$|^[Ee][Dd]$|EDITABLE|[Ee]ditable")
  add_custom_command(
    TARGET al POST_BUILD
      COMMAND
        ${Python_EXECUTABLE}
          -m venv --system-site-packages ${PYTHON_VENV}
      COMMAND
        ${PYTHON_VENV_BIN}/pip install --upgrade pip wheel
      COMMAND
        ${CMAKE_COMMAND} -E env CMAKE_ARGS=${CMAKE_ARGS}
        ${PYTHON_VENV_BIN}/pip install
          --no-build-isolation
          --config-settings=editable.rebuild=true
          --verbose
          -Cbuild-dir=${CMAKE_CURRENT_BINARY_DIR}/{wheel_tag}
          --editable=${CMAKE_CURRENT_SOURCE_DIR}
  )
  return()
endif()


add_custom_command(
  TARGET al POST_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E env CMAKE_ARGS=${CMAKE_ARGS}
      ${Python_EXECUTABLE}
        -m venv --system-site-packages ${PYTHON_VENV}
    COMMAND
      ${PYTHON_VENV_BIN}/pip install --upgrade pip wheel
    COMMAND
      ${CMAKE_COMMAND} -E env CMAKE_ARGS=${CMAKE_ARGS}
      ${PYTHON_VENV_BIN}/pip wheel
        ${CMAKE_CURRENT_SOURCE_DIR}
        --no-deps
        --verbose
        -Cbuild-dir=${CMAKE_CURRENT_BINARY_DIR}/{wheel_tag}
        --wheel-dir ${CMAKE_CURRENT_BINARY_DIR}/dist/
)
add_custom_target(al-python-bindings DEPENDS al)
set_target_properties(
  al-python-bindings PROPERTIES DIST_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/dist/)
install(CODE "execute_process(COMMAND ${Python_EXECUTABLE}
-m pip install imas_core
--no-deps
--prefix=${CMAKE_INSTALL_PREFIX}
--find-links ${CMAKE_CURRENT_BINARY_DIR}/dist/
--force-reinstall
)"
DEPENDS al-python-bindings
)

