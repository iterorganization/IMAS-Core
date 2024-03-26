#!/bin/bash
# Bamboo CI script to build and run tests for all lowlevel components
#
# This script expects to be run from the repository root directory

# Debuggging:
set -e -o pipefail
echo "Loading modules..."

# Set up environment such that module files can be loaded
. /usr/share/Modules/init/sh
module purge
# Load modules:
MODULES=(
    CMake/3.18.4-GCCcore-10.2.0
    # Required for building the lowlevel and backends
    Boost/1.74.0-GCCcore-10.2.0
    HDF5/1.10.7-GCCcore-10.2.0-serial
    MDSplus/7.131.6-GCCcore-10.2.0
    UDA/2.7.5-GCCcore-10.2.0
    # Required for building MDSplus models
    Saxon-HE/11.4-Java-11
    MDSplus-Java/7.131.6-GCCcore-10.2.0-Java-11
    # Python lowlevel bindings
    Python/3.8.6-GCCcore-10.2.0
    SciPy-bundle/2020.11-intel-2020b
)
module load "${MODULES[@]}"

# Debuggging:
echo "Done loading modules"
set -x


# Ensure the build directory is clean:
rm -rf build

# CMake configuration:
CMAKE_ARGS=(
    -D "CMAKE_INSTALL_PREFIX=$(pwd)/test-install/"
    # Enable all backends
    -D AL_BACKEND_HDF5=ON
    -D AL_BACKEND_MDSPLUS=ON
    -D AL_BACKEND_UDA=ON
    # Build MDSplus models
    -D AL_BUILD_MDSPLUS_MODELS=ON
    # "Download" dependencies from repos checked out by Bamboo in the repos/ folder:
    -D AL_DOWNLOAD_DEPENDENCIES=ON
    -D "AL_COMMON_GIT_REPOSITORY=$(pwd)/repos/al-common/"
    -D "DD_GIT_REPOSITORY=$(pwd)/repos/data-dictionary/"
    # DD version:
    -D DD_VERSION=master/3
)
# Note: we don't set CC or CXX compiler, so CMake will pick the default (GCC) compilers
cmake -B build "${CMAKE_ARGS[@]}"

# Build
make -C build -j8 all

# Test install
make -C build install

# List installed files
ls -lR test-install
