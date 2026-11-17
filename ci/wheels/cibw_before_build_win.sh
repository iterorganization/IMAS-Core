set -xe

export VCPKG_ROOT="${VCPKG_ROOT:-$2}"
export VCPKG_TARGET_TRIPLET="${VCPKG_TARGET_TRIPLET:-$3}"

# vcpkg setup
# ####################################################################

if ! test -d ${VCPKG_ROOT} ;then
    git clone --depth 1 https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT}
fi
if ! test -f ${VCPKG_ROOT}/vcpkg.exe ;then
    ${VCPKG_ROOT}/bootstrap-vcpkg.sh -disableMetrics
fi

# bash shell has issues with "C:/" in path
abspath() {
    # resolve absolute path for a file or directory
    cd "$(dirname "$1")"
    printf "%s/%s\n" "$(pwd)" "$(basename "$1")"
}
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
# Only needed when the wheel is built with the UDA backend. The project's own
# dependencies (vcpkg.json) are installed by the vcpkg CMake toolchain.
AL_BACKEND_UDA="${AL_BACKEND_UDA:-ON}"
case "${AL_BACKEND_UDA^^}" in
    OFF|FALSE|NO|0)
        echo "AL_BACKEND_UDA=${AL_BACKEND_UDA}: skipping UDA dependencies and build"
        python -m pip install delvewheel wheel
        exit 0
        ;;
esac

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


# Skip the UDA fetch+build when the client is already installed (uda-cpp.pc
# is the last artifact the install produces), e.g. when this script is re-run
# in the same job.
if test -f "${USEFUL_CMAKE_INSTALL_PREFIX}/lib/pkgconfig/uda-cpp.pc" ;then
    echo "UDA already installed under ${USEFUL_CMAKE_INSTALL_PREFIX} (restored cache) - skipping UDA build"
else
# Reuse the UDA source archive from vcpkg's downloads directory when present
# (C:\vcpkg\downloads may be restored from a CI cache). A fresh download is
# copied back there so a cache saved afterwards includes it.
# Every archive is checked with `tar tzf` before use, so an HTML error page or
# a truncated file (from a previous attempt or a cache) is discarded and
# re-fetched instead of failing every retry the same way.
UDA_TARBALL="${UDA_REF}.tar.gz"
UDA_TARBALL_CACHE="${VCPKG_ROOT}/downloads/UDA-${UDA_REF}.tar.gz"
valid_tarball() { test -s "$1" && tar tzf "$1" >/dev/null 2>&1; }
if ! valid_tarball "${UDA_TARBALL}" ;then
    rm -f "${UDA_TARBALL}"
    if valid_tarball "${UDA_TARBALL_CACHE}" ;then
        cp "${UDA_TARBALL_CACHE}" "${UDA_TARBALL}"
    else
        rm -f "${UDA_TARBALL_CACHE}" "${UDA_TARBALL}.part"
        curl -fL --retry 3 -o "${UDA_TARBALL}.part" \
            "https://github.com/ukaea/UDA/archive/refs/tags/${UDA_REF}.tar.gz"
        valid_tarball "${UDA_TARBALL}.part"
        mv "${UDA_TARBALL}.part" "${UDA_TARBALL}"
        mkdir -p "${VCPKG_ROOT}/downloads"
        cp "${UDA_TARBALL}" "${UDA_TARBALL_CACHE}"
    fi
fi
# Always extract into a fresh tree: a previous attempt may have left a
# partially extracted or partially built UDA-${UDA_REF}/ behind.
rm -rf "UDA-${UDA_REF}"
tar zxf "${UDA_TARBALL}"
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
    cmake --build extlib/build --config Release
    cmake --install extlib/build --config Release
# build UDA
    cmake -Bbuild . ${UDA_CMAKE_ARGS[@]}
    cmake --build build -j --config Release
# patch faulty .pc file for windows builds
    find ./ -name "uda-cpp.pc" | xargs sed -i -e "s| capnp||g;s| -std=c++17| /std:c++20|"
# install UDA
    cmake --install build --config Release
)
fi
fi
# delvewheel is the equivalent of delocate/auditwheel for windows.
python -m pip install delvewheel wheel