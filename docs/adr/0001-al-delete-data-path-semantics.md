---
status: accepted
---

# `al_delete_data`'s `path` selects a node and its subtree, never the occurrence

`al_delete_data(ctx, path)` has always documented `path` as "the data structure
element to delete (suppress the whole subtree)", but the HDF5 backend dropped
the argument on the floor: `HDF5Backend::deleteData` did not forward it and
`HDF5Writer::deleteData` had no parameter to receive it, so **any** non-empty
path deleted the whole IDS occurrence — the per-IDS pulse file and the master
file's external link to it — and returned success (issue #63). We keep the
documented meaning and implement it: a non-empty `path` deletes that node and
its subtree only, an empty `path` addresses the whole DATAOBJECT, and deleting
an absent path is a successful no-op.

## Considered options

**1. Per-path delete is intended (chosen).** Implemented in
`HDF5Writer::deleteSubtree`. An IDS group is a flat namespace of "tensorized"
dataset names — the DD path with `/` → `&`, array-of-structures nodes suffixed
`[]`, one `_SHAPE` companion per non-scalar leaf, one `…[]&AOS_SHAPE` per
dynamic AOS — so the subtree rooted at a DD path is exactly the set of names
that are the mangled path, its `_SHAPE` companion, or that continue with `&` (a
structure child) or `[]` (an AOS element index). The continuation character is
what keeps `time` from taking `time_slice[]&x` with it.

**2. Occurrence-wide delete is intended: reject a non-empty `path`.** Rejected.
It is the cheaper change, but it is not what the ABI says and not what the other
backends do — and it would break every HDF5 write through the HLI. imas-python's
`delete_children()` walks the DD and issues one `al_delete_data` per leaf
(`imas/backends/imas_core/db_entry_helpers.py`), and `DBEntry.put()` runs that
whole traversal before writing (`db_entry_al.py`), so rejecting non-empty paths
would fail every non-slice `put()` and `DBEntry.delete_data()` on HDF5. Today
that traversal *appears* to work only because the first forwarded delete
destroys the file and the following ones silently do nothing (the first call
closes the IDS group, after which every later call returns early) — and the put
then rewrites everything from scratch. That accident is what hid the defect.

The third state the code was actually in — argument accepted, ignored, success
reported — is what neither option tolerates, and it is not merely untidy. It is
the mechanism behind a silent data corruption downstream: the
IMAS-Multiversion-DD-Loader refuses to delete an occurrence's DD-version stamp
so a converted pulse cannot hold undateable numbers, but inside a full put the
traversal's first forwarded delete has already taken the whole file, stamp
included, and the rest of the put is written untranslated
([IMAS-Multiversion-DD-Loader#139](https://github.com/yohannmarguier/IMAS-Multiversion-DD-Loader/issues/139)).

## Per-backend behaviour, as of this ADR

Checked by reading each `deleteData` and pinned in `tests/contract/`
(`test_structured_data.cpp` `DeleteMatrix` / `Hdf5Delete`,
`test_mdsplus_delete.cpp`, `test_uda_unique_surface.cpp`); the ledger is
`tests/contract/TRACEABILITY.md`.

| Backend | node | subtree | whole DATAOBJECT (`path == ""`) |
|---|---|---|---|
| HDF5 | ✅ | ✅ | ✅ — deletes `<ids>.h5` *and* the master-file link |
| Memory | ✅ | ✅ (AOS node) | ❌ no code path for it — xfail |
| MDSplus | ✅ | ❌ `%TREE-W-NNF` — xfail | ❌ `%TREE-W-NNF` — xfail |
| ASCII | ❌ empty body | ❌ | ❌ |
| Flexbuffers | ❌ empty body | ❌ | ❌ |
| UDA | forwards `path` (remote: no server handler → clean refusal; local/fetch: delegates to the local backend) | " | " |

So HDF5 was the only backend that *widened* `path`; the rest either honour it or
ignore the call entirely. None of them reports "I cannot honour this path",
which is why the header now says so explicitly rather than leaving an HLI to
discover it.

`MemoryBackend::deleteData` also carries a dead `ctx->getType() ==
CTX_ARRAYSTRUCT_TYPE` branch: the parameter is an `OperationContext*` and
`al_delete_data` `dynamic_cast`s to one, so an arraystruct context can never
reach it. A plain (non-AOS) structure path on Memory is not characterized.

## Consequences

- **An emptied occurrence still exists on HDF5.** `DBEntry.delete_data()` walks
  the DD and deletes leaf by leaf, so `<ids>.h5` and its master-file link
  survive with no datasets left in them, and `al_get_occurrences` keeps
  reporting the occurrence. That is the honest consequence of `path` meaning
  what it says; an HLI that wants the occurrence gone should pass `path == ""`,
  which is now the only call that removes it.
- **A delete never shrinks `<ids>.h5`.** `H5Ldelete` unlinks; the space is
  reclaimed only by repacking the file.
- **The master-file link now follows the file.** A whole-DATAOBJECT delete
  removes the external link as well, so the master file no longer advertises an
  occurrence whose backing file is gone — the second half of what issue #63
  measured.
- A whole-IDS delete lists the IDS group's link names once per delete rather
  than once per leaf (`HDF5Writer::groupMembers`, invalidated by anything that
  can add a dataset). Without that cache the HLI's leaf-by-leaf traversal would
  be quadratic in the size of the IDS.
