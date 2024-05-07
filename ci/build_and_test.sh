#!/bin/bash
# Bamboo CI script to build and run tests for all core components
#
# This script expects to be run from the repository root directory

# Debuggging:
# set -e -o pipefail
echo "Loading modules..."

# Set up environment such that module files can be loaded
. /usr/share/Modules/init/sh
module purge
# Load modules:
MODULES=(
    # Required for configure, build, and install
    CMake/3.27.6-GCCcore-13.2.0
    Ninja/1.11.1-GCCcore-13.2.0
    # Required for building the core and backends
    HDF5/1.14.3-gompi-2023b
    Boost/1.83.0-GCC-13.2.0
    UDA/2.7.5-GCC-13.2.0
    # Required for building MDSplus models
    Saxon-HE/12.4-Java-21
    MDSplus/7.132.0-GCCcore-13.2.0
    # Python bindings
    Python/3.11.5-GCCcore-13.2.0
)
module load "${MODULES[@]}"
pip install build
# Debuggging:
echo "Done loading modules"
# set -x

# Create a local git configuration with our access token
if [ "x$bamboo_HTTP_AUTH_BEARER_PASSWORD" != "x" ]; then
    mkdir -p git
    echo "[http \"https://git.iter.org/\"]
        extraheader = Authorization: Bearer $bamboo_HTTP_AUTH_BEARER_PASSWORD" > git/config
    export XDG_CONFIG_HOME=$PWD
    git config -l
fi

# Ensure that the install directory is clean:
rm -rf test-install

# Ensure the build and dist directories are clean:
rm -rf build dist

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
cmake -Bbuild -GNinja "${CMAKE_ARGS[@]}"

# Build and install 
cmake --build build --target install

# Basic import test (assumes AL_PYTHON_BINDINGS=ON)
(
    source $(pwd)/test-install/bin/al_env.sh
    python -c 'import imas_core; print(imas_core._al_lowlevel.get_al_version())'

)

# List installed files
find test-install -not -path "*/numpy/*" -ls

# Pip install imas-core into a bare venv, run unit-tests and generate a clover.xml coverage report. 
module purge
module load Python/3.11.5-GCCcore-13.2.0
python3.11 -m venv build/pip_install 
source build/pip_install/bin/activate 
python3.11 -m pip install --find-links=dist imas-core[test,cov]
pytest --junitxml results.xml --cov imas_core --cov-report xml --cov-report html 
coverage2clover -i coverage.xml -o clover.xml

