set -xe

WHEEL="$1"
DEST_DIR="$2"

delvewheel repair --ignore-existing -w ${DEST_DIR} ${WHEEL}
