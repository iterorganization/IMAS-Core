# FindPThreads4W.cmake - Minimal version
# Find the PThreads-Win32 library

message(STATUS "FindPThreads4W: CMAKE_PREFIX_PATH = ${CMAKE_PREFIX_PATH}")
message(STATUS "FindPThreads4W: VCPKG_INSTALLED_DIR = ${VCPKG_INSTALLED_DIR}")
message(STATUS "FindPThreads4W: VCPKG_TARGET_TRIPLET = ${VCPKG_TARGET_TRIPLET}")
message(STATUS "FindPThreads4W: CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "FindPThreads4W: CMAKE_CURRENT_BINARY_DIR = ${CMAKE_CURRENT_BINARY_DIR}")

# Try to determine vcpkg installed directory from build directory structure
set(_POSSIBLE_VCPKG_PATHS "")
if(CMAKE_CURRENT_BINARY_DIR MATCHES "(.*/build)/")
    set(_BUILD_DIR "${CMAKE_MATCH_1}")
    list(APPEND _POSSIBLE_VCPKG_PATHS 
        "${_BUILD_DIR}/vcpkg_installed/x64-windows"
        "${_BUILD_DIR}/vcpkg_installed/x86-windows"
    )
    message(STATUS "FindPThreads4W: Detected build dir: ${_BUILD_DIR}")
endif()

# Find include directory and library
find_path(PThreads4W_INCLUDE_DIR
    NAMES pthread.h
    HINTS 
        ${PThreads4W_DIR}
        ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}
        ${CMAKE_PREFIX_PATH}
        ${_POSSIBLE_VCPKG_PATHS}
    PATH_SUFFIXES include
)

find_library(PThreads4W_LIBRARY
    NAMES pthreadVC3 pthreadVCE3 pthreadVSE3 pthread pthreads
    HINTS
        ${PThreads4W_DIR}
        ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}
        ${CMAKE_PREFIX_PATH}
        ${_POSSIBLE_VCPKG_PATHS}
    PATH_SUFFIXES lib
)

message(STATUS "FindPThreads4W: PThreads4W_INCLUDE_DIR = ${PThreads4W_INCLUDE_DIR}")
message(STATUS "FindPThreads4W: PThreads4W_LIBRARY = ${PThreads4W_LIBRARY}")

# Use standard CMake handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PThreads4W
    REQUIRED_VARS PThreads4W_INCLUDE_DIR PThreads4W_LIBRARY
)

if(PThreads4W_FOUND)
    set(PThreads4W_INCLUDE_DIRS ${PThreads4W_INCLUDE_DIR})
    set(PThreads4W_LIBRARIES ${PThreads4W_LIBRARY})
    
    # Create imported target
    if(NOT TARGET PThreads4W::PThreads4W)
        add_library(PThreads4W::PThreads4W UNKNOWN IMPORTED)
        set_target_properties(PThreads4W::PThreads4W PROPERTIES
            IMPORTED_LOCATION "${PThreads4W_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${PThreads4W_INCLUDE_DIR}"
        )
        if(WIN32)
            set_target_properties(PThreads4W::PThreads4W PROPERTIES
                INTERFACE_LINK_LIBRARIES "ws2_32"
            )
        endif(WIN32)
    endif(NOT TARGET PThreads4W::PThreads4W)
endif(PThreads4W_FOUND)

mark_as_advanced(PThreads4W_INCLUDE_DIR PThreads4W_LIBRARY)
