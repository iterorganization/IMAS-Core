set -xe

WHEEL="$1"
DEST_DIR="$2"

# --ignore-existing: unless set, delvewheel does not check whether a DLL is already in the wheel and tries to find it from an external source.
# CIBW_REPAIR_WHEEL_COMMAND_WINDOWS: "delvewheel repair --ignore-existing -w {dest_dir} {wheel} --add-path ${{github.workspace}}/install/lib"
delvewheel repair --ignore-existing -w ${DEST_DIR} ${WHEEL}
