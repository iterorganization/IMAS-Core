# AL-Common

This repository contains shared assets for Access Layer components:

- Shared documentation assets
- Shared CMake assets

## How it works

During the CMake configure step of any of the other Access Layer repositories
(`al-core`, `al-fortran`, etc.) this repository is automatically cloned and
imported by CMake (if the option `AL_DOWNLOAD_DEPENDENCIES=ON`, which it is by
default). Look for the following lines in the CMakeLists.txt in each of
the AL components:

```cmake
# Configuration options for common assets
################################################################################
option( AL_DOWNLOAD_DEPENDENCIES "Automatically download common assets from the AL git repository" ON )
set( AL_COMMON_GIT_REPOSITORY "ssh://git@git.iter.org/imas/al-common.git" CACHE STRING "Git repository of AL-common" )
set( AL_COMMON_VERSION "main" CACHE STRING "Git commit/tag/branch of AL-common" )

include(FetchContent)

# Load common assets
################################################################################
if( ${AL_DOWNLOAD_DEPENDENCIES} )
  # Download common assets from the ITER git:
  FetchContent_Declare(
    al-common
    GIT_REPOSITORY "${AL_COMMON_GIT_REPOSITORY}"
    GIT_TAG "${AL_COMMON_VERSION}"
  )
else()
  # Look in $AL_COMMON_PATH or ../al-common for common files
  if( DEFINED ENV{AL_COMMON_PATH} )
    set( AL_COMMON_PATH $ENV{AL_COMMON_PATH} )
  else()
    set( AL_COMMON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../al-common" )
  endif()
  FetchContent_Declare(
    al-common
    SOURCE_DIR      "${AL_COMMON_PATH}"
  )
endif()
FetchContent_MakeAvailable( al-common )
```

Contrary to what the name implies, the `GIT_TAG` can be a branch name, tag
or commit hash. See [this CMake documentation
page](https://cmake.org/cmake/help/latest/module/ExternalProject.html#git).
