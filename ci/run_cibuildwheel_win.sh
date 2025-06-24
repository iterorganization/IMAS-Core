#!/bin/bash
#
# This script mimics running the GitHub Actions workflow for building wheels on Windows using cibuildwheel,
# produces wheelhouse/imas_core-...-cp311-cp311-win_amd64.whl
#
# Usage:
# $ bash ci/run_cibuildwheel_win.sh
#
# expects Python 3.11 to be pre-installed

# set -x

abspath() {
    # resolve absolute path for a file or directory
    cd "$(dirname "$1")"
    printf "%s/%s\n" "$(pwd)" "$(basename "$1")"
}
exit_error() {
    echo "ERROR: $*" >&2
    exit 1
}

PARENT_DIR_OF_THIS_FILE="$(dirname $(dirname $(abspath $0)))"

export PROJECT_DIR="${PROJECT_DIR:-$PARENT_DIR_OF_THIS_FILE}"
export VIRTUALENV_DIR="${VIRTUALENV_DIR:-${PROJECT_DIR}/.venv}"
export VCPKG_ROOT="${VCPKG_ROOT:-C:/vcpkg}"
export VCPKG_TARGET_TRIPLET="${VCPKG_TARGET_TRIPLET:-x64-windows}"

(
    cd ${PROJECT_DIR}

    python -mvenv "${VIRTUALENV_DIR}"
    . "${VIRTUALENV_DIR}"/scripts/activate
    python -m pip install -U pip
    python -m pip install cibuildwheel==3.0.0

    # Debug
    python -c "import sys, pprint, cibuildwheel; pprint.pp(sys.path); pprint.pp(cibuildwheel)"

    # AL_BACKEND_HDF5: AL_BACKEND_HDF5=ON
    # AL_BACKEND_MDSPLUS: AL_BACKEND_MDSPLUS=OFF
    # AL_BACKEND_UDA: AL_BACKEND_UDA=OFF  # FIXME: imas-core w/UDA backend on windows
    # UDA_REF: "2.8.1"
    # FMT_REF: "11.1.4"
    export UDA_REF="2.8.1"
    export CIBW_BUILD="cp311-win_amd64"
    export CIBW_PLATFORM="windows"
    export CIBW_ARCHS="AMD64"
    export CIBW_CONFIG_SETTINGS="cmake.define.AL_BACKEND_HDF5=ON  cmake.define.AL_BACKEND_MDSPLUS=OFF  cmake.define.AL_BACKEND_UDA=OFF"
    # export CIBW_ENVIRONMENT_WINDOWS="PKG_CONFIG_PATH=PWD.replace('\\', '/')

    python -m cibuildwheel --output-dir wheelhouse --config-file pyproject.toml
)