# Configure CMAKE_TOOLCHAIN_FILE 
option(VCPKG "Use VCPKG to manage third-party dependancies" OFF)
if(NOT VCPKG AND DEFINED ENV{VCPKG})
  set(VCPKG $ENV{VCPKG})
endif() 
if(VCPKG)
  if(NOT DEFINED ENV{VCPKG_ROOT}) 
    message(FATAL_ERROR "Use of VCPKG:ON requres VCPKG_ROOT environment variable to set CMAKE_TOOLCHAIN_FILE.")
  endif()
  cmake_path(SET CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
endif()

