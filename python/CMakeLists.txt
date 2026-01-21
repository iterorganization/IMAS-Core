find_package(Python REQUIRED COMPONENTS Interpreter Development.Module NumPy)
find_package(Cython MODULE REQUIRED VERSION 3.0)
include(UseCython)
cython_transpile(imas_core/_al_lowlevel.pyx LANGUAGE CXX OUTPUT_VARIABLE _al_lowlevel_source)
cython_transpile(imas_core/al_defs.pyx LANGUAGE CXX OUTPUT_VARIABLE al_defs_source)
python_add_library(_al_lowlevel MODULE ${_al_lowlevel_source} WITH_SOABI)
python_add_library(al_defs MODULE ${al_defs_source} WITH_SOABI)
target_link_libraries(_al_lowlevel PRIVATE Python::NumPy al)
target_link_libraries(al_defs PRIVATE Python::NumPy al)

# Handling RPATH in macOS:
if(APPLE)
  set_target_properties(al PROPERTIES INSTALL_RPATH "@loader_path")
  set_target_properties(_al_lowlevel al_defs PROPERTIES INSTALL_RPATH "@loader_path/../imas_core.libs")
else()
  set_target_properties(al PROPERTIES INSTALL_RPATH $ORIGIN)
  set_target_properties(_al_lowlevel al_defs PROPERTIES INSTALL_RPATH $ORIGIN/../imas_core.libs)
endif()
            
install(FILES $<TARGET_RUNTIME_DLLS:al> DESTINATION imas_core.libs)
install(FILES $<TARGET_RUNTIME_DLLS:_al_lowlevel> DESTINATION imas_core.libs)
install(FILES $<TARGET_RUNTIME_DLLS:al_defs> DESTINATION imas_core.libs)
if(DEFINED ENV{LD_LIBRARY_PATH})
  string(REPLACE ":" ";" LIBRARY_DIRS $ENV{LD_LIBRARY_PATH})
else()
  set(LIBRARY_DIRS)
endif()

# Add vcpkg directories on Windows
if(WIN32)
  message(STATUS "Searching for vcpkg directories...")
  message(STATUS "  CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
  message(STATUS "  CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
  message(STATUS "  VCPKG_INSTALLED_DIR: ${VCPKG_INSTALLED_DIR}")
  message(STATUS "  VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")
  
  # Method 1: Try VCPKG_INSTALLED_DIR variable
  if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    if(EXISTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
      list(APPEND LIBRARY_DIRS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
      list(APPEND LIBRARY_DIRS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin")
      message(STATUS "  Found via VCPKG_INSTALLED_DIR: ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
    endif()
  endif()
  
  # Method 2: Try extracting from CMAKE_TOOLCHAIN_FILE
  if(DEFINED CMAKE_TOOLCHAIN_FILE)
    get_filename_component(VCPKG_ROOT "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
    get_filename_component(VCPKG_ROOT "${VCPKG_ROOT}" DIRECTORY)
    get_filename_component(VCPKG_ROOT "${VCPKG_ROOT}" DIRECTORY)
    if(EXISTS "${VCPKG_ROOT}/installed/x64-windows/bin")
      list(APPEND LIBRARY_DIRS "${VCPKG_ROOT}/installed/x64-windows/bin")
      list(APPEND LIBRARY_DIRS "${VCPKG_ROOT}/installed/x64-windows/debug/bin")
      message(STATUS "  Found via CMAKE_TOOLCHAIN_FILE: ${VCPKG_ROOT}/installed/x64-windows")
    endif()
  endif()
  
  # Method 3: Search parent directories for vcpkg_installed (FetchContent/ExternalProject builds)
  set(SEARCH_DIR "${CMAKE_BINARY_DIR}")
  foreach(i RANGE 6)
    get_filename_component(SEARCH_DIR "${SEARCH_DIR}" DIRECTORY)
    if(EXISTS "${SEARCH_DIR}/vcpkg_installed/x64-windows/bin")
      list(APPEND LIBRARY_DIRS "${SEARCH_DIR}/vcpkg_installed/x64-windows/bin")
      list(APPEND LIBRARY_DIRS "${SEARCH_DIR}/vcpkg_installed/x64-windows/debug/bin")
      message(STATUS "  Found via parent search: ${SEARCH_DIR}/vcpkg_installed")
      break()
    endif()
  endforeach()
  
  # Remove duplicates
  if(LIBRARY_DIRS)
    list(REMOVE_DUPLICATES LIBRARY_DIRS)
  endif()
  
  # Ensure we have at least some directories (install-time fallback)
  if(NOT LIBRARY_DIRS)
    message(WARNING "Could not find vcpkg directories at configure time. Adding PATH as fallback.")
    # This will be resolved at install time from system PATH
    list(APPEND LIBRARY_DIRS "$ENV{PATH}")
  endif()
  
  message(STATUS "Python wheel LIBRARY_DIRS: ${LIBRARY_DIRS}")
endif()

# Build POST_EXCLUDE_REGEXES list based on platform
set(POST_EXCLUDE_PATTERNS
  ".*system32/.*\\.dll" # Windows system DLLs
  "^/(lib|usr/lib|usr/local/lib)" # Unix system libraries
  "iccifort/.*/lib/intel64/lib" # Redundant iccifort
)

# Add Linux-specific exclusions
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND POST_EXCLUDE_PATTERNS
    ".*/imkl/.*/compiler/.*/linux/compiler/lib" # Intel MKL compiler libraries
    ".*/intel-compilers/.*/compiler/.*/linux/compiler/lib" # Intel compilers libraries
    "^libomp\\.so" # OpenMP libraries
    "^libsvml\\.so" # Intel SVML math library
    "^libirng\\.so" # Intel RNG library
    "^libintlc\\.so" # Intel intlc library
    "^libimf\\.so" # Intel IMF library (math functions)
  )
endif()

install(
  TARGETS al
  DESTINATION imas_core.libs
  RUNTIME_DEPENDENCIES	
    DIRECTORIES ${LIBRARY_DIRS}
    PRE_EXCLUDE_REGEXES   
      "api-ms-" # VC Redistibutable DLLs
      "ext-ms-" # Windows extension DLLs
    POST_EXCLUDE_REGEXES ${POST_EXCLUDE_PATTERNS}
)
install(TARGETS _al_lowlevel al_defs DESTINATION imas_core)
