"""HDF5 read buffering must not change results, for any kind of read.

Regression test for get_sample() loading every *_SHAPE dataset in full (all time
slices) when read buffering is enabled. Buffering is now skipped for partial reads
of time-dependent data and for datasets above ``hdf5_read_buffer_max``; the data
returned must be the same in all cases.

Uses IMAS-Python to write and read IDSs; skipped if it is not installed.
"""

import numpy as np
import pytest

imas = pytest.importorskip("imas")

from imas.ids_defs import CLOSEST_INTERP, LINEAR_INTERP, PREVIOUS_INTERP  # noqa: E402

N_TIMES = 50
TIMES = np.linspace(0.0, 4.9, N_TIMES)

BUFFERING_OPTIONS = {
    "default": "",
    "no_buffering": ";hdf5_read_buffering=no",
    # tiny limit: every dataset is too large to be buffered in full
    "tiny_buffer_limit": ";hdf5_read_buffer_max=0.000001",
}


def _write(path):
    with imas.DBEntry(f"imas:hdf5?path={path}", "w") as db:
        cp = db.factory.core_profiles()
        cp.ids_properties.homogeneous_time = imas.ids_defs.IDS_TIME_MODE_HOMOGENEOUS
        cp.code.name = "test"
        cp.time = TIMES
        cp.global_quantities.ip = 1e6 * (1 + TIMES)
        cp.profiles_1d.resize(N_TIMES)
        for i, p1d in enumerate(cp.profiles_1d):
            n = 3 + i % 4  # the profile length changes in time
            p1d.grid.rho_tor_norm = np.linspace(0, 1, n)
            p1d.electrons.temperature = (i + 1.0) * np.arange(n)
            p1d.ion.resize(1 + i % 3)  # so does the number of ions
            for k, ion in enumerate(p1d.ion):
                ion.name = f"ion{k}_{i}"
                ion.z_ion = k + 1.0
                ion.density = (k + 1.0) * np.ones(n) * i
        db.put(cp)

        mg = db.factory.magnetics()
        mg.ids_properties.homogeneous_time = imas.ids_defs.IDS_TIME_MODE_HOMOGENEOUS
        mg.time = TIMES
        mg.flux_loop.resize(3)  # static AOS with dynamic data
        for k, fl in enumerate(mg.flux_loop):
            fl.name = f"loop{k}"
            fl.flux.data = k + TIMES**2
        db.put(mg)


def _read_all(uri):
    """All supported kinds of reads, each on a newly opened entry and all on one entry."""
    explicit_times = np.array([0.0, 0.33, 2.5, 4.9])
    reads = {}
    for name in ("core_profiles", "magnetics"):
        calls = {"get": lambda db: db.get(name, autoconvert=False)}
        for interp in (CLOSEST_INTERP, PREVIOUS_INTERP, LINEAR_INTERP):
            calls[f"get_slice_{interp}"] = lambda db, i=interp: db.get_slice(
                name, 2.45, i, autoconvert=False
            )
            calls[f"get_sample_explicit_{interp}"] = lambda db, i=interp: db.get_sample(
                name, 0, 0, explicit_times, i, autoconvert=False
            )
            calls[f"get_sample_uniform_{interp}"] = lambda db, i=interp: db.get_sample(
                name, 0.5, 3.0, 0.5, i, autoconvert=False
            )
        calls["get_sample_range"] = lambda db: db.get_sample(
            name, 1.0, 3.0, autoconvert=False
        )
        for key, call in calls.items():
            with imas.DBEntry(uri, "r") as db:
                reads[f"{name}/{key}"] = call(db)
        # Same calls on one open entry: cached datasets must not leak state between reads
        with imas.DBEntry(uri, "r") as db:
            for key, call in calls.items():
                reads[f"{name}/same_entry/{key}"] = call(db)
    return reads


@pytest.fixture(scope="module")
def pulse(tmp_path_factory):
    path = tmp_path_factory.mktemp("hdf5_read_buffering")
    _write(path)
    return path


@pytest.fixture(scope="module")
def reference(pulse):
    return _read_all(f"imas:hdf5?path={pulse}{BUFFERING_OPTIONS['no_buffering']}")


@pytest.mark.parametrize("option", ["default", "tiny_buffer_limit"])
def test_reads_do_not_depend_on_buffering(pulse, reference, option):
    reads = _read_all(f"imas:hdf5?path={pulse}{BUFFERING_OPTIONS[option]}")
    assert reads.keys() == reference.keys()
    for key, ids in reads.items():
        differences = list(imas.util.idsdiffgen(ids, reference[key]))
        assert not differences, f"{key} ({option}): {differences[:3]}"


def test_reference_content(reference):
    """Sanity check that the reads return what was written."""
    full = reference["core_profiles/get"]
    assert len(full.profiles_1d) == N_TIMES
    assert full.profiles_1d[5].ion[2].name == "ion2_5"
    sample = reference[f"core_profiles/get_sample_explicit_{CLOSEST_INTERP}"]
    # with an explicit time base, the returned time vector is the requested one
    assert np.allclose(sample.time, [0.0, 0.33, 2.5, 4.9])
    assert len(sample.profiles_1d) == 4
    # closest stored slices are 0, 3, 25 and 49, with 1 + i % 3 ions
    assert [len(p.ion) for p in sample.profiles_1d] == [1, 1, 2, 2]
    loops = reference[f"magnetics/get_sample_explicit_{CLOSEST_INTERP}"].flux_loop
    assert [fl.name.value for fl in loops] == ["loop0", "loop1", "loop2"]
    assert np.allclose(loops[1].flux.data, 1 + np.array([0.0, 0.3, 2.5, 4.9]) ** 2)
