---
status: accepted
---

# An absolute field path resolves by segment match against the open context chain

`al_read_data(ctx, fieldpath, …)` documents `fieldpath` as "always relative to
current Context, dataobject absolute path can be specified with a prepended
'/'" (`include/al_lowlevel.h`, read-data block). An absolute field path is not
a filesystem path and does not carry the IDS name:
`/time_slice/constraints/j_phi/reconstructed` names the same stored datum as
`reconstructed` read from the corresponding array-of-structures context.

The HDF5 read path honoured only the relative half. `HDF5Reader::read_ND_Data`
replaced every `/` with `&` (the tensorized separator) and then
*unconditionally* prepended the current AOS context's tensorized path, so an
already-rooted name was appended to the context it was meant to replace:

```text
requested   /time_slice/constraints/j_phi/reconstructed
looked up   time_slice[]&constraints&j_phi[]&&time_slice&constraints&j_phi&reconstructed
stored as   time_slice[]&constraints&j_phi[]&reconstructed
```

`H5Lexists` found nothing, `read_ND_Data` returned 0, and `al_plugin_read_data`
filled the caller's buffer with `EMPTY_DOUBLE` while reporting `code == 0`
(`Lowlevel::setDefaultValue`). Stored data was therefore indistinguishable from
genuinely absent data — a silent wrong answer, not an error (issue #65). The
same defect made a root field unreadable from the operation context itself,
where the leading `&` was the only thing left to strip.

We keep the documented meaning and implement it: an absolute field path is
resolved **by path segment** against the AOS chain the caller currently has
open, retaining the tensorized markers and the indices of every level the
target still sits under, and only those.

## Considered options

**1. Segment-match against the open context chain (chosen).** Implemented in
`HDF5Utils::resolveAbsoluteFieldPath`, called from `read_ND_Data`. An IDS group
is a flat namespace of tensorized dataset names — the DD path with `/` → `&`,
each array-of-structures segment suffixed `[]`, the AOS flattened into a
leading index dimension — so resolving an absolute path means answering two
questions at once, which is why one function answers both:

- **which name to look up.** The stored name keeps the `[]` of every AOS the
  field sits under, so the tensorized prefix of the matched levels is retained
  and only the remaining segments are appended.
- **which indices still apply.** Each retained `[]` level keeps its current
  cursor; levels the target does not sit under are dropped, along with the
  timed-AOS designation if the timed level is among them (the dataset then
  carries no time dimension to select in).

Both fall out of one walk: compare the absolute path's segments against the
open chain's segments and cut at the last AOS boundary that still matches.
Matching is per segment, never by string prefix — `time_slice_extra/…` shares a
textual prefix with `time_slice[]` but no path prefix at all. This is the same
rule ADR 0001 adopted for `al_delete_data`, for the same reason: segment-wise
is what tells a real descendant from a same-prefix stranger.

Worked examples, with `time_slice[]` → `constraints/j_phi[]` open at
`(slice=1, constraint=2)`:

| Requested | Resolved name | Indices applied |
|---|---|---|
| `/time_slice/constraints/j_phi/reconstructed` | `time_slice[]&constraints&j_phi[]&reconstructed` | `(1, 2)` |
| `/time_slice/global_quantities/ip` | `time_slice[]&global_quantities&ip` | `(1)` |
| `/vacuum_toroidal_field/r0` | `vacuum_toroidal_field&r0` | none |
| `/time_slice/constraints/j_phi/reconstructed`, from the `time_slice` context | `time_slice[]&constraints&j_phi&reconstructed` | `(1)` → not found, absent |

**2. Strip the leading `/` and use the rest verbatim.** Rejected. It resolves a
root field correctly and nothing else: the result carries no `[]` for any AOS
above the target, so every path through an array of structures — the case the
issue is actually about — still misses. It also silently duplicates the path
when the target is under the open context.

**3. Skip the context prefix by string length.** Rejected. Cheaper than a
segment walk and wrong twice over: it drops the `[]` markers along with the
prefix, and a purely textual prefix matches same-prefix strangers.

**4. Keep the child's whole cursor regardless of where the target sits.**
Rejected. An ancestor-level dataset carries fewer tensorized dimensions than
the child context has indices, so applying all of them either selects out of
extent or reads the wrong element. This is the mirror image of option 2's
failure and would be the more dangerous one, since it returns a number.

**5. Leave the core alone; have the caller rewrite absolute to relative.**
Rejected. The contract being broken is the core's own, stated in its public
header, so every caller would have to carry the same workaround. It is also not
generally possible: an absolute path may address an *ancestor* or a *root*
field, which no relative spelling from the current context can name, and a
blanket rewrite of the caller's argument would corrupt exactly those cases.

## Scope

The reproduction, diagnosis and fix are HDF5 **reads**. Writes,
`al_delete_data`, `al_begin_arraystruct_action` and the other backends were not
established as affected by this reproduction and are deliberately not changed
here; a concrete finding about any of them belongs in its own issue. The
regression suite writes through relative spellings only, so it never presumes
an answer for the write side.

The change is confined to path/selection computation: no DD-version knowledge,
no schema lookup, no C ABI change. The version knowledge constraint in
`NORTH_STAR.md` is the reason a "just look up whether that segment is an AOS"
resolution is not on the table at all.

## Consequences

- **A target beyond an AOS the caller never opened is reported absent.** There
  is no cursor for that level, and the core holds no schema that would even
  tell it the segment is an array of structures. The resolved name carries no
  `[]` for it, finds nothing, and travels the ordinary missing-data path. That
  is the honest answer; inventing an index is how a wrong value gets returned
  in place of no value. Pinned by
  `Hdf5AbsoluteFieldPaths.TargetBeyondAnUnopenedAosIsReportedAbsent`.
- **Ordinary missing-data semantics are unchanged.** An absolute path that
  resolves but names nothing stored still returns `code == 0` with the type's
  empty sentinel, exactly as the relative spelling does. The fix makes stored
  data findable; it does not make absent data look present.
- **Existence and dataset caches are keyed on the resolved name**
  (`existing_data_sets`, `opened_data_sets`), so the two spellings of one
  address share one cache entry and cannot disagree across repeated reads.
- **A root timebase is now classified correctly.** `/time` from the operation
  context previously resolved to `&time`, which matched the "time dataset
  inside a dynamic AOS" suffix test rather than the homogeneous-timebase one.
  It now resolves to `time` and is classified as the homogeneous timebase.
- **Absolute and relative reads may still differ in *cost*.** Nothing here
  changes the per-node `Backend` contract: an absolute read is resolved and
  looked up one field at a time, like every other read.
