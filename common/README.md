# AL-Common

This folder contains shared assets for Access Layer components:

- Shared documentation assets
- Shared CMake assets

## How it works

During the CMake configure step of any of the other Access Layer repositories
(`al-core`, `al-fortran`, etc.) this folder is imported by CMake:

```cmake
# Configuration options for common repos
################################################################################
option( AL_DOWNLOAD_DEPENDENCIES "Automatically download assets from the AL git repository" ON )
set( AL_CORE_GIT_REPOSITORY "git@github.com:iterorganization/IMAS-Core.git" CACHE STRING "Git repository of AL-core" )
set( AL_CORE_VERSION "main" CACHE STRING "Git commit/tag/branch of AL-core" )

include(FetchContent)

################################################################################
if( ${AL_DOWNLOAD_DEPENDENCIES} )
  # Download common assets from the ITER git:
  FetchContent_Declare(
    al-core
    GIT_REPOSITORY "${AL_CORE_GIT_REPOSITORY}"
    GIT_TAG "${AL_CORE_VERSION}"
  )
else()
  FetchContent_Declare(
    al-core
    SOURCE_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/../al-core"
  )
endif()
FetchContent_MakeAvailable( al-core )
add_subdirectory( "${al-core_SOURCE_DIR}/common" _common )
```

Contrary to what the name implies, the `GIT_TAG` can be a branch name, tag
or commit hash. See [this CMake documentation
page](https://cmake.org/cmake/help/latest/module/ExternalProject.html#git).
