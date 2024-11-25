#!/bin/bash
# Bamboo CI script to build and run tests for all core components
#
# This script expects to be run from the repository root directory

# Debuggging:
set -e -o pipefail
echo "Loading modules..."

# Set up environment such that module files can be loaded
if test -f /etc/profile.d/modules.sh ;then
. /etc/profile.d/modules.sh
else
. /usr/share/Modules/init/sh
fi
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
    MDSplus/7.131.6-GCCcore-10.2.0
    MDSplus-Java/7.131.6-GCCcore-10.2.0-Java-11
    UDA/2.7.5-GCC-10.2.0
    Python/3.8.6-GCCcore-10.2.0
    # scikit_build_core
    Cython/3.0.10-GCCcore-10.2.0
    cython-cmake/0.1.0-GCCcore-10.2.0
    # setuptools_scm
)
MODULES_TEST=(
    Python/3.8.6-GCCcore-10.2.0
)
  ;;&
  *foss-2020b)
echo "... foss-2020b"
MODULES=(${MODULES[@]}
    HDF5/1.10.7-gompi-2020b
)
CMAKE_ARGS=(${CMAKE_ARGS[@]}
    -DCMAKE_C_COMPILER=${CC:-gcc}
    -DCMAKE_CXX_COMPILER=${CXX:-g++}
)
  ;;&
  *intel-2020b)
echo "... intel-2020b"
MODULES=(${MODULES[@]}
    iccifort/2020.4.304
    HDF5/1.10.7-iimpi-2020b
)
CMAKE_ARGS=(${CMAKE_ARGS[@]}
    -DCMAKE_C_COMPILER=${CC:-icc}
    -DCMAKE_CXX_COMPILER=${CXX:-icpc}
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
    scikit-build-core/0.9.3-GCCcore-13.2.0
    Cython/3.0.10-GCCcore-13.2.0
    cython-cmake/0.2.0-GCCcore-13.2.0
    setuptools-scm/8.1.0-GCCcore-13.2.0
    typing-extensions
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
CMAKE_ARGS=(${CMAKE_ARGS[@]}
    -DCMAKE_C_COMPILER=${CC:-gcc}
    -DCMAKE_CXX_COMPILER=${CXX:-g++}
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
CMAKE_ARGS=(${CMAKE_ARGS[@]}
    -DCMAKE_C_COMPILER=${CC:-icx}
    -DCMAKE_CXX_COMPILER=${CXX:-icpx}
)
  ;;
esac
echo "${MODULES[@]}" | tr " " "\n"

module load "${MODULES[@]}"


if [ "$TOOLCHAIN" == "intel-2020b" ] ;then
    # ibimf.so isn't on the LD_LIBRARY_PATH for intel/2020b, so add a path from iccifort to find it:
    LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$EBROOTICCIFORT/compilers_and_libraries_2020.4.304/linux/compiler/lib/intel64_lin"
fi


# Debuggging:
echo "Done loading modules:"
module list
set -x

# Create a local git configuration with our access token
if [ "x$bamboo_HTTP_AUTH_BEARER_PASSWORD" != "x" ]; then
    mkdir -p git
    echo "[http \"https://git.iter.org/\"]
        extraheader = Authorization: Bearer $bamboo_HTTP_AUTH_BEARER_PASSWORD" > git/config
    export XDG_CONFIG_HOME=$PWD
    git config -l |cat
fi

# Default DD version unless specified in the plan
DD_VERSION=${DD_VERSION:-main}

# Ensure that the install directory is clean:
rm -rf test-install

# Ensure the build directory is clean:
rm -rf build

# CMake configuration:
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
    -DDD_GIT_REPOSITORY=https://git.iter.org/scm/imas/data-dictionary.git
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

# List installed files
find test-install -not -path "*/numpy/*" -ls

set +x

echo "Loading modules for test..."
module purge
echo "${MODULES_TEST[@]}" | tr " " "\n"
module load "${MODULES_TEST[@]}"

echo "Begin test..."
# Pip install imas-core into a bare venv, run unit-tests and generate a clover.xml coverage report.
python3 -m venv build/pip_install
source build/pip_install/bin/activate
pip install --upgrade pip wheel
pip install "numpy<2" 
set -x
pip install --find-links=build/dist imas-core[test,cov]
pytest --junitxml results.xml --cov imas_core --cov-report xml --cov-report html
coverage2clover -i coverage.xml -o clover.xml
set +x
deactivate


