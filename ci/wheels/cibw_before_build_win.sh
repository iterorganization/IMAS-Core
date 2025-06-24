set -xe

PROJECT_DIR="$1"
export VCPKG_ROOT="${VCPKG_ROOT:-$2}"
export VCPKG_TARGET_TRIPLET="${VCPKG_TARGET_TRIPLET:-$3}"

abspath() {
    # resolve absolute path for a file or directory
    cd "$(dirname "$1")"
    printf "%s/%s\n" "$(pwd)" "$(basename "$1")"
}

# vcpkg setup
# ####################################################################

if ! test -d ${VCPKG_ROOT} ;then
    git clone --depth 0 https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT}
fi
if ! test -f ${VCPKG_ROOT}/vcpkg.exe ;then
    ${VCPKG_ROOT}/bootstrap-vcpkg.sh -disableMetrics
fi

# bash shell has issues with "C:/" in path
PATH="$(abspath ${VCPKG_ROOT}):${PATH}"
which vcpkg

CMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE:-${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake}"

# Explicitly state derivatives
USEFUL_CMAKE_INSTALL_PREFIX="${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}"
CMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
CMAKE_PREFIX_PATH="${USEFUL_CMAKE_INSTALL_PREFIX};${USEFUL_CMAKE_INSTALL_PREFIX}/share;"
PKG_CONFIG_PATH="${USEFUL_CMAKE_INSTALL_PREFIX}/lib/pkgconfig"
printenv | sort

# UDA installation (until installation is available packaged...)
# ####################################################################
UDA_REF="${UDA_REF:-2.8.1}"
UDA_DEPS=(
    pkgconf
    pthreads
    hdf5
    libxml2
    capnproto
    boost-program-options
    boost-algorithm
    boost-filesystem
    boost-format
    boost-multi-array
    openssl
    dlfcn-win32
    spdlog
)
UDA_CMAKE_ARGS=(
    -Wno-deprecated
    -DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}"
    -DCMAKE_INSTALL_PREFIX="${USEFUL_CMAKE_INSTALL_PREFIX}"
    -DCMAKE_GENERATOR_PLATFORM=x64
    -DCMAKE_BUILD_TYPE=Release
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DSSLAUTHENTICATION:BOOL=ON
    -DCLIENT_ONLY:BOOL=ON
    -DENABLE_CAPNP:BOOL=ON
    #  --debug-find
    #  --debug-output
)

# manually install UDA dependencies (if missing) (using --classic ignores project vcpkg.json manifest file)
vcpkg install --classic ${UDA_DEPS[@]}

# N.B. all the deps in vcpkgs.json will be installed (if missing) via vcpkg's cmake toolchain file


if ! test -f ${UDA_REF}.tar.gz ;then
curl -LO https://github.com/ukaea/UDA/archive/refs/tags/${UDA_REF}.tar.gz
fi
if ! test -d UDA-${UDA_REF} ;then
tar zxf ${UDA_REF}.tar.gz
fi
if test -d UDA-${UDA_REF} ;then
(
    cd UDA-${UDA_REF}
# build portablexdr
    cmake -Bextlib/build ./extlib \
        -DCMAKE_INSTALL_PREFIX="${USEFUL_CMAKE_INSTALL_PREFIX}" \
        -DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DCMAKE_GENERATOR_PLATFORM=x64 \
        -DBUILD_SHARED_LIBS=OFF
    cmake --build extlib/build --config Debug
    cmake --install extlib/build
# build UDA
    cmake -Bbuild . ${UDA_CMAKE_ARGS[@]}
    cmake --build build -j --config Debug
# patch faulty .pc file for windows builds
    find ./ -name "uda-cpp.pc" | xargs sed -i -e "s| capnp||g;s| -std=c++17| /std:c++20|"
# install UDA
    cmake --install build --config Debug
)
fi
# delvewheel is the equivalent of delocate/auditwheel for windows.
python -m pip install delvewheel wheel