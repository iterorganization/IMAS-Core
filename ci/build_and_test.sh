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

# Check for TOOLCHAIN
TOOLCHAIN=${TOOLCHAIN:-foss-2023b}
# Load modules that correspond to toolchain
case "$TOOLCHAIN" in
  *-2020b)
echo "... 2020b"
MODULES=(
    CMake/3.24.3-GCCcore-10.2.0
    Ninja/1.10.1-GCCcore-10.2.0
    Boost/1.74.0-GCC-10.2.0
    Saxon-HE/10.3-Java-11
    Blitz++/1.0.2-GCCcore-10.2.0
    Python/3.8.6-GCCcore-10.2.0
    MDSplus/7.131.6-GCCcore-10.2.0
    MDSplus-Java/7.131.6-GCCcore-10.2.0-Java-11
    UDA/2.7.5-GCC-10.2.0
)
  ;;&
  *foss-2020b)
echo "... foss-2020b"
MODULES=(${MODULES[@]}
    SciPy-bundle/2020.11-foss-2020b
    HDF5/1.10.7-gompi-2020b
    build/0.10.0-foss-2020b
)
CMAKE_ARGS=(${CMAKE_ARGS[@]}
    # Work around Boost linker issues on 2020b toolchain
    -D Boost_NO_BOOST_CMAKE=ON
)
  ;;&
  *intel-2020b)
echo "... intel-2020b"
MODULES=(${MODULES[@]}
    iccifort/2020.4.304
    SciPy-bundle/2020.11-intel-2020b
    HDF5/1.10.7-iimpi-2020b
    build/0.10.0-intel-2020b
)
  ;;
  *-2023b)
echo "... 2023b"
MODULES=(
    CMake/3.27.6-GCCcore-13.2.0
    Ninja/1.11.1-GCCcore-13.2.0
    Boost/1.83.0-GCC-13.2.0
    UDA/2.7.5-GCC-13.2.0
    Saxon-HE/12.4-Java-21
    Blitz++/1.0.2-GCCcore-13.2.0
    MDSplus/7.132.0-GCCcore-13.2.0
    Python/3.11.5-GCCcore-13.2.0
    build/1.0.3-GCCcore-13.2.0
    scikit-build/0.17.6-GCCcore-13.2.0
    Python/3.11.5-GCCcore-13.2.0
    Python-bundle-PyPI/2023.10-GCCcore-13.2.0
)
MODULES_TEST=(
    Python/3.11.5-GCCcore-13.2.0
)
  ;;&
  *foss-2023b)
echo "... foss-2023b"
MODULES=(${MODULES[@]}
    HDF5/1.14.3-gompi-2023b
    SciPy-bundle/2023.11-gfbf-2023b
)
  ;;&
  *intel-2023b)
echo "... intel-2023b"
MODULES=(${MODULES[@]}
    intel/2023b
    HDF5/1.14.3-iimpi-2023b
    SciPy-bundle/2023.12-iimkl-2023b
)
MODULES_TEST=(${MODULES_TEST[@]}
    intel/2023b
)
  ;;
esac
echo "${MODULES[@]}" | tr " " "\n"

module load "${MODULES[@]}"

# Debuggging:
echo "Done loading modules:"
module list
#set -x

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

# Ensure the build directory is clean:
rm -rf build 

# CMake configuration:
CMAKE_ARGS=(${CMAKE_ARGS[@]}
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

echo "Loading modules for test..."
module purge
echo "${MODULES_TEST[@]}" | tr " " "\n"
module load "${MODULES_TEST[@]}"

echo "Begin test..."
# Pip install imas-core into a bare venv, run unit-tests and generate a clover.xml coverage report. 
python3 -m venv build/pip_install 
source build/pip_install/bin/activate 
python3 -m pip install --find-links=build/dist imas-core[test,cov]
pytest --junitxml results.xml --cov imas_core --cov-report xml --cov-report html 
coverage2clover -i coverage.xml -o clover.xml

