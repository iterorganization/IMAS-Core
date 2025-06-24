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
export WHEELHOUSE="${WHEELHOUSE:-${PROJECT_DIR}/wheelhouse}"
export VIRTUALENV_BUILD="${VIRTUALENV_BUILD:-${PROJECT_DIR}/.venv}"
export VIRTUALENV_TEST="${VIRTUALENV_TEST:-${PROJECT_DIR}/.venv_test}"
export VCPKG_ROOT="${VCPKG_ROOT:-C:/vcpkg}"
export VCPKG_TARGET_TRIPLET="${VCPKG_TARGET_TRIPLET:-x64-windows}"

# cf. .github/workflows/wheels.yml
# AL_BACKEND_HDF5: AL_BACKEND_HDF5=ON
# AL_BACKEND_MDSPLUS: AL_BACKEND_MDSPLUS=OFF
# AL_BACKEND_UDA: AL_BACKEND_UDA=OFF  # FIXME: imas-core w/UDA backend on windows
# UDA_REF: "2.8.1"
# FMT_REF: "11.1.4"
export UDA_REF="2.8.1"
export CIBW_BUILD="cp311-win_amd64"
export CIBW_PLATFORM="windows"
export CIBW_ARCHS="AMD64"
export CIBW_CONFIG_SETTINGS="\
    cmake.define.AL_BACKEND_HDF5=ON \
    cmake.define.AL_BACKEND_MDSPLUS=OFF \
    cmake.define.AL_BACKEND_UDA=OFF \
"


# Clean?
rm -rf ${VIRTUALENV_BUILD}
rm -rf ${VIRTUALENV_TEST}
rm -rf ${WHEELHOUSE}


# Build wheel
(
    cd ${PROJECT_DIR}

    python -mvenv "${VIRTUALENV_BUILD}"
    source "${VIRTUALENV_BUILD}"/scripts/activate
    python -m pip install -U pip
    python -m pip install cibuildwheel==3.0.0

    python -m cibuildwheel --output-dir ${WHEELHOUSE} --config-file pyproject.toml
)

# Test wheel
(
    cd ${PROJECT_DIR}

    python -mvenv .${VIRTUALENV_TEST}
    source ${VIRTUALENV_TEST}/scripts/activate
    python -m pip install -U pip

    # test wheel installation
    WHEEL=$(cd ${WHEELHOUSE} ; ls -1 --sort=t imas_core-*-${CIBW_BUILD}.whl | tail -n1))
    python -m pip install ${WHEELHOUSE}/${WHEEL}
    python -c "import imas_core; print(imas_core.version_tuple)" 

    # run pytest with project's ./tests
    python -m pip install ${WHEELHOUSE}/${WHEEL}[test,cov]
    pytest --junitxml results.xml --cov imas_core --cov-report xml --cov-report html
    coverage2clover -i coverage.xml -o clover.xml
)