"""Regression test for issue #40.

When master.h5 is world-writable but the IDS file (e.g. equilibrium.h5) is
read-only for the current user, reading used to fail with:

    ALBackendException = Unable to open HDF5 group: equilibrium

This was because openMasterFile opened master.h5 in H5F_ACC_RDWR (because
the user had write access), and then H5Gopen2 tried to follow the external
link in RDWR mode — conflicting with the RDONLY handle for the IDS file.
"""

import os

import numpy as np
import pytest

import imas_core

ll = imas_core._al_lowlevel
defs = imas_core.al_defs


def _hdf5_available() -> bool:
    """Return True if a working HDF5 backend is available."""
    uri = f"imas:hdf5?path=dummy"
    try:
        imas_core.exception.raise_error_flag = True
        status, ctx = ll.al_begin_dataentry_action(uri, defs.OPEN_PULSE)
        imas_core.exception.raise_error_flag = False
        return True
    except imas.exception.ImasCoreBackendException as iex:
        if "not available" in str(iex.message):
            return False
        pass


pytestmark = pytest.mark.skipif(
    not _hdf5_available(), reason="HDF5 backend not available"
)


def _write_ids(tmpdir: str) -> None:
    """Create an HDF5 pulse and write a minimal equilibrium IDS."""
    uri = f"imas:hdf5?path={tmpdir}"
    status, ctx = ll.al_begin_dataentry_action(uri, defs.CREATE_PULSE)
    assert status == 0, f"Failed to create pulse: status={status}"

    status, opctx = ll.al_begin_global_action(ctx, "equilibrium", defs.WRITE_OP)
    assert status == 0, f"Failed to begin write action: status={status}"

    ll.al_write_data(opctx, "ids_properties/homogeneous_time", "", 2)
    ll.al_write_data(opctx, "ids_properties/comment", "", "TEST")
    ll.al_end_action(opctx)
    ll.al_close_pulse(ctx, defs.CLOSE_PULSE)


def _read_ids(tmpdir: str) -> None:
    """Open the HDF5 pulse and read back the equilibrium IDS."""
    uri = f"imas:hdf5?path={tmpdir}"
    status, ctx = ll.al_begin_dataentry_action(uri, defs.OPEN_PULSE)
    assert status == 0, f"Failed to open pulse: status={status}"

    status, opctx = ll.al_begin_global_action(ctx, "equilibrium", defs.READ_OP)
    assert status == 0, f"Failed to begin read action: status={status}"

    result = ll.al_read_data(opctx, "ids_properties/comment", "", defs.CHAR_DATA, 1)
    assert result == (0,"TEST"), f"Failed to read back comment: {result}"
    
    ll.al_end_action(opctx)
    ll.al_close_pulse(ctx, defs.CLOSE_PULSE)
    return result


def test_read_with_writable_master_and_readonly_ids(tmp_path):
    """Reading must succeed when master.h5 is world-writable but IDS is read-only.

    This reproduces issue #40: a user reads an IDS they don't own where the
    owner left master.h5 world-writable (e.g. 0o666) but the IDS file is
    only readable (e.g. 0o444).
    """
    _write_ids(tmp_path)

    master_path = os.path.join(tmp_path, "master.h5")
    ids_path = os.path.join(tmp_path, "equilibrium.h5")

    assert os.path.exists(master_path), "master.h5 not created"
    assert os.path.exists(ids_path), "equilibrium.h5 not created"

    # Simulate the problematic permission setup from issue #40:
    # master is world-writable, IDS is read-only for everyone.
    os.chmod(master_path, 0o666)
    os.chmod(ids_path, 0o444)

    # This must not raise an exception.
    result = _read_ids(tmp_path)
    assert result is not None


def test_write_to_readonly_ids_raises(tmp_path):
    """Writing to a read-only IDS file must fail, not silently delete and
    recreate the file with modified permissions.

    Regression test for the side-effect of the issue #40 fix: when master.h5
    is writable but equilibrium.h5 is read-only, a PUT operation must return
    an error status, and the file permissions must not change.
    """
    _write_ids(tmp_path)

    master_path = os.path.join(tmp_path, "master.h5")
    ids_path = os.path.join(tmp_path, "equilibrium.h5")

    os.chmod(master_path, 0o666)  # world-writable master
    os.chmod(ids_path, 0o444)     # read-only IDS

    uri = f"imas:hdf5?path={tmp_path}"
    status, ctx = ll.al_begin_dataentry_action(uri, defs.OPEN_PULSE)
    assert status == 0

    # First do a successful read (reproduces the scenario from issue #40)
    status, opctx = ll.al_begin_global_action(ctx, "equilibrium", defs.READ_OP)
    assert status == 0, f"GET should succeed: status={status}"
    result = ll.al_read_data(opctx, "ids_properties/comment", "", defs.CHAR_DATA, 1)
    assert result == (0,"TEST")
    ll.al_end_action(opctx)

    # Now try to write — must fail (permission denied), not change permissions
    status, opctx = ll.al_begin_global_action(ctx, "equilibrium", defs.WRITE_OP)
    assert status != 0, (
        "PUT to read-only IDS should fail, but returned status={status}"
    )

    ll.al_close_pulse(ctx, defs.CLOSE_PULSE)

    # Permissions must be unchanged
    assert oct(os.stat(ids_path).st_mode & 0o777) == oct(0o444), (
        "equilibrium.h5 permissions were modified during failed write"
    )


def test_read_with_readonly_master_and_readonly_ids(tmp_path):
    """Reading must also succeed when both master and IDS are read-only."""
    _write_ids(tmp_path)

    master_path = os.path.join(tmp_path, "master.h5")
    ids_path = os.path.join(tmp_path, "equilibrium.h5")

    os.chmod(master_path, 0o444)
    os.chmod(ids_path, 0o444)

    try:
        result = _read_ids(tmp_path)
        assert result is not None
    finally:
        # Restore write permissions so tempdir cleanup works.
        os.chmod(master_path, 0o644)
        os.chmod(ids_path, 0o644)


def test_put_with_readonly_master_and_existing_rw_ids(tmp_path):
    """PUT must succeed when master.h5 is read-only but the IDS file already
    exists with a link in master and has read-write permissions.

    This represents the case where a user wants to prevent creation of new IDS
    types in a data-entry (by making master.h5 read-only) while still allowing
    writes to already-linked IDS files.
    """
    _write_ids(tmp_path)

    master_path = os.path.join(tmp_path, "master.h5")
    ids_path = os.path.join(tmp_path, "equilibrium.h5")

    # master is read-only (no new links can be added), IDS is read-write
    os.chmod(master_path, 0o444)
    # equilibrium.h5 stays at default writable permissions

    uri = f"imas:hdf5?path={tmp_path}"
    status, ctx = ll.al_begin_dataentry_action(uri, defs.OPEN_PULSE)
    assert status == 0

    # PUT must succeed: the link already exists in master and IDS is writable
    status, opctx = ll.al_begin_global_action(ctx, "equilibrium", defs.WRITE_OP)
    assert status == 0, (
        f"PUT to existing RW IDS should succeed with RDONLY master, "
        f"got status={status}"
    )
    ll.al_write_data(opctx, "time", "", np.array([1.0, 2.0, 3.0]))
    ll.al_end_action(opctx)
    ll.al_close_pulse(ctx, defs.CLOSE_PULSE)

    # master permissions must not have changed
    assert oct(os.stat(master_path).st_mode & 0o777) == oct(0o444), (
        "master.h5 permissions were modified during write to existing IDS"
    )

    # Restore so tempdir cleanup works
    os.chmod(master_path, 0o644)


def test_put_with_readonly_master_and_no_ids_link_raises(tmp_path):
    """PUT must fail when master.h5 is read-only and the target IDS has no
    existing link in master.

    A read-only master prevents adding new external links, so writing to an
    IDS type that was never stored in this data-entry must raise an error.
    """
    # Only write 'equilibrium'; leave 'core_profiles' absent
    _write_ids(tmp_path)

    master_path = os.path.join(tmp_path, "master.h5")
    os.chmod(master_path, 0o444)

    uri = f"imas:hdf5?path={tmp_path}"
    status, ctx = ll.al_begin_dataentry_action(uri, defs.OPEN_PULSE)
    assert status == 0

    # PUT to a new IDS type must fail: master is RDONLY so no link can be added
    status, _ = ll.al_begin_global_action(ctx, "core_profiles", defs.WRITE_OP)
    assert status != 0, (
        "PUT to a new IDS type with RDONLY master should fail"
    )

    ll.al_close_pulse(ctx, defs.CLOSE_PULSE)

    # Restore so tempdir cleanup works
    os.chmod(master_path, 0o644)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
