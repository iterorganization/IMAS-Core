# IMAS-Core

[![Build Status](https://github.com/iterorganization/IMAS-Core/actions/workflows/cmake.yaml)](https://github.com/iterorganization/IMAS-Core/actions)
[![License](https://github.com/iterorganization/IMAS-Python/blob/develop/LICENSE.txt)](LICENSE)


IMAS-Core is part of the Integrated Modelling and Analysis Suite at ITER. It provides a low-level library to read and write MDSplus ([Introduction](https://www.mdsplus.org/index.php/Introduction)) and HDF5 ([HDF5](https://www.hdfgroup.org/solutions/hdf5/)) formats. It also supports reading and writing [FlexBuffers](https://flatbuffers.dev/flexbuffers/). For debugging purposes, it allows writing ASCII files. Notably, for keeping data in-memory when exchanging data between physics codes, it supports writing into memory.

The IMAS-Core library includes Python bindings, making it useful for working with Python. It can be installed on different operating systems, including macOS and Windows. IMAS-Core is a versatile library that helps users write fusion data in a standard format used by various users and facilitates easy data exchange.

Most of the code is written in C++, with wrappers on existing libraries for HDF5, MDSplus, and FlexBuffers.

## Features

- C lowlevel interface used by the various High Level Interfaces
- Python bindings to the lowlevel interface
- Backends for reading and writing IMAS data
- Logic for creating the models required by the MDS+ backend

## Installation

To build the IMAS-Core you need:

-   Git
-   A C++11 compiler (tested with GCC and Intel compilers)
-   CMake (3.16 or newer)
-   Saxon-HE XSLT processor
-   Boost C++ libraries (1.66 or newer)
-   PkgConfig

The following dependencies are only required for some of the components:

-   Backends

    -   **HDF5 backend**: HDF5 C/C++ libraries (1.8.12 or newer)
    -   **MDSplus backend**: MDSplus libraries (7.84.8 or newer)
    -   **UDA backend**: `UDA <https://github.com/ukaea/UDA/>`__ libraries
	(2.7.5 or newer) [#uda_install]_

Detailed instruction you can find here: TODO: Provide link Building and installing the IMAS-Core

```markdown
```bash

git clone git@github.com:iterorganization/IMAS-Core.git

cd IMAS-Core

CMAKE_ARGS=(${CMAKE_ARGS[@]}
    -D"CMAKE_INSTALL_PREFIX=$(pwd)/test-install/"
    # Enable all backends
    -DAL_BACKEND_HDF5=ON
    -DAL_BACKEND_MDSPLUS=ON
    -DAL_BACKEND_UDA=ON
    # Build MDSplus models
    -DAL_BUILD_MDSPLUS_MODELS=ON
    # Build Python bindings
    -DAL_PYTHON_BINDINGS=no-build-isolation
    # Download dependencies from HTTPS (using an access token):
    -DAL_DOWNLOAD_DEPENDENCIES=ON
    -DDD_GIT_REPOSITORY=https://github.com/iterorganization/IMAS-Data-Dictionary.git
    # DD version:
    -DDD_VERSION=$DD_VERSION
    # Work around Boost linker issues on 2020b toolchain
    -DBoost_NO_BOOST_CMAKE=ON
)
echo "CMake args:"
echo ${CMAKE_ARGS[@]} | tr ' ' '\n'

# Note: we don't set CC or CXX compiler, so CMake will pick the default (GCC) compilers
cmake -Bbuild -GNinja "${CMAKE_ARGS[@]}"

# Build and install
cmake --build build --target install
```

