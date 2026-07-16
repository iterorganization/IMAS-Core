#!/usr/bin/env bash
# check_label_partition.sh — CI smoke check for the contract-suite CTest
# labels (issue #31).
#
# An empty `ctest -L unit` exits 0, so a label regression (version-dependent
# property forwarding, filter drift, a renamed suite) is silent unless
# something checks the selection itself. This verifies, machine-readably:
#   1. `ctest -N -L unit` and `ctest -N -L integration` both select a nonzero
#      number of tests;
#   2. the two selections are disjoint (no test carries both labels);
#   3. the unit selection is hermetic — no filesystem-backed backend case.
#
# Usage: check_label_partition.sh <contract-tests-build-dir>
#   e.g. check_label_partition.sh build/tests/contract
set -euo pipefail

build_dir=${1:?usage: check_label_partition.sh <contract-tests-build-dir>}

list_names() {
    # `ctest -N` right-aligns test numbers ("Test  #9:" vs "Test #99:"), so the
    # whitespace before '#' is variable; strip the "(Disabled)" suffix too.
    ctest --test-dir "${build_dir}" -N -L "$1" \
        | sed -n 's/^ *Test *#[0-9]*: //p' | sed 's/ (Disabled)$//'
}

unit=$(list_names '^unit$')
integration=$(list_names '^integration$')
unit_count=$(printf '%s' "${unit}" | grep -c . || true)
integration_count=$(printf '%s' "${integration}" | grep -c . || true)
echo "unit: ${unit_count} tests, integration: ${integration_count} tests"

fail=0
if [ "${unit_count}" -eq 0 ]; then
    echo "FAIL: 'ctest -L unit' selects zero tests — the fast hermetic tier is empty (issue #31)."
    fail=1
fi
if [ "${integration_count}" -eq 0 ]; then
    echo "FAIL: 'ctest -L integration' selects zero tests — the on-disk tier is empty (issue #31)."
    fail=1
fi

overlap=$(comm -12 <(printf '%s\n' "${unit}" | sort -u) \
                   <(printf '%s\n' "${integration}" | sort -u) | grep . || true)
if [ -n "${overlap}" ]; then
    echo "FAIL: tests carry both 'unit' and 'integration' labels:"
    printf '%s\n' "${overlap}"
    fail=1
fi

# -i: gtest pretty names spell backends differently per CMake version
# ("(HDF5, ..." on new CMake, "#GetParam()=ASCII" on 3.21, "Hdf5..." in
# suite names).
nonhermetic=$(printf '%s\n' "${unit}" | grep -iE 'hdf5|ascii|flexbuffers|mdsplus|uda' || true)
if [ -n "${nonhermetic}" ]; then
    echo "FAIL: the unit selection contains filesystem-/server-backed cases:"
    printf '%s\n' "${nonhermetic}"
    fail=1
fi

if [ "${fail}" -ne 0 ]; then
    exit 1
fi
echo "OK: unit/integration labels are nonempty, disjoint, and the unit tier is hermetic."
