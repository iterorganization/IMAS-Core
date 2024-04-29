#!/bin/bash
# Bamboo CI script to build and run tests for all core components
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
    CMake/3.24.3-GCCcore-10.2.0
    # Required for building the core and backends
    Boost/1.74.0-GCC-10.2.0
    HDF5/1.10.7-GCCcore-10.2.0-serial
    MDSplus/7.131.6-GCCcore-10.2.0
    UDA/2.7.5-GCC-10.2.0
    # Required for building MDSplus models
    Saxon-HE/11.4-Java-11
    MDSplus-Java/7.131.6-GCCcore-10.2.0-Java-11
    # Python bindings
    Python/3.8.6-GCCcore-10.2.0
    SciPy-bundle/2020.11-intel-2020b
)
module load "${MODULES[@]}"

# Debuggging:
echo "Done loading modules"
set -x

# Create a local git configuration with our access token
if [ "x$bamboo_HTTP_AUTH_BEARER_PASSWORD" != "x" ]; then
    mkdir -p git
    echo "[http \"https://git.iter.org/\"]
        extraheader = Authorization: Bearer $bamboo_HTTP_AUTH_BEARER_PASSWORD" > git/config
    export XDG_CONFIG_HOME=$PWD
    git config -l
fi

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
    # Build Python bindings
    -D AL_PYTHON_BINDINGS=ON
    # Download dependencies from HTTPS (using an access token):
    -D AL_DOWNLOAD_DEPENDENCIES=ON
    -D AL_COMMON_GIT_REPOSITORY=https://git.iter.org/scm/imas/al-common.git
    -D DD_GIT_REPOSITORY=https://git.iter.org/scm/imas/data-dictionary.git
    # DD version:
    -D DD_VERSION=master/3
)
# Note: we don't set CC or CXX compiler, so CMake will pick the default (GCC) compilers
cmake -B build "${CMAKE_ARGS[@]}"

# Build and install 
cmake --build build --target install

# List installed files
ls -lR test-install

# pip install imas-core into a bare venv, run unit-tests and generate a clover.xml coverage report. 
python -m venv build/pip_install 
. build/pip_install/bin/activate 
module purge
pip install --find_links=build/dist imas-core[test,cov]
pytest --junitxml results.xml --cov imas-core --cov-report xml --cov-report html coverage2clover -i coverage.xml -o clover.xml
deactivate

