# Contract-suite traceability matrix (Task 2)

Per **TEST_STRATEGY.md decision D8**, this matrix — not line coverage — is the
**primary definition of "comprehensive."** One row per
`FUNCTIONALITY_INVENTORY.md` capability reachable through the public C ABI,
with the test(s) that cover it and a status:

- **covered** — an intended-contract test asserts the behavior and passes.
- **xfail** — a genuine defect; the test asserts the *correct* behavior and is
  expected-fail (decision D2). Flipping to pass means someone fixed it.
- **gap** — not yet tested; a later build-order step (TEST_STRATEGY §4) fills it.

Line coverage (`gcov`/`lcov`) is a secondary signal only (D8): a rewrite may
restructure lines but must still satisfy every row here.

Test IDs are GoogleTest `Suite.Case` names as registered in CTest; run one with
`ctest -R <id>` or `contract_tests --gtest_filter=<id>`.

> **Note on the Unit / Integ. columns.** They classify each test by *substrate*
> per decision D6 (pure lookups and Memory round trips are unit; on-disk
> backends are integration) — the intended tier taxonomy. The CTest `unit` /
> `integration` **labels** that would let `ctest -L unit` select them are
> deferred (see `CMakeLists.txt`); today every case carries the single
> `contract` label. Until the split lands, address the unit subset with
> `ctest -R Introspection`.

---

## Cluster 4 — Introspection / diagnostics  (issue #5 — the thin unit tier)

`FUNCTIONALITY_INVENTORY.md:434-459`. Pure lookups: no context, no backend,
zero DD artifacts (decisions D5/D6). All **intended contract** (D2) — nothing
here is a defect, including the silent `""`, which the inventory documents as
deliberate.

| Capability | Inventory ref | Unit | Integ. | Test(s) | Status |
|---|---|:--:|:--:|---|---|
| `const2str(id)` maps a known constant to its symbolic name | :434-444 | ✓ | — | `Introspection.Const2StrMapsKnownConstantsToTheirNames` | covered |
| `const2str` silently returns `""` for an unmapped id (intended, not an error) | :451-452 | ✓ | — | `Introspection.Const2StrReturnsEmptyForUnmappedId` | covered |
| `const2str` returns `""` for defined-but-unmapped constants (`TIMERANGE_OP`, `FLEXBUFFERS_BACKEND`) | :75-79, :453-454 | ✓ | — | `Introspection.Const2StrReturnsEmptyForDefinedButUnmappedConstants` | covered |
| `err2str(id)` maps a known error code to its name | :445-446 | ✓ | — | `Introspection.Err2StrMapsKnownErrorCodesToTheirNames` | covered |
| `err2str` silently returns `""` for an unmapped id | :451-452 | ✓ | — | `Introspection.Err2StrReturnsEmptyForUnmappedId` | covered |
| `const2str`/`err2str` are backed by separate maps (two namespaces) | :443-446 | ✓ | — | `Introspection.Const2StrAndErr2StrUseSeparateMaps` | covered |
| `getALVersion()` returns the compiled AL version string | :447 | ✓ | — | `Introspection.GetALVersionReturnsCompiledVersionMacro`, `Introspection.GetALVersionIsDottedNumeric` | covered |
| `getDDVersion()` returns the deprecated sentinel `"!!DEPRECATED!!"` (intended) | :455-459 | ✓ | — | `Introspection.GetDDVersionReturnsDeprecatedSentinel` | covered |

## Version sentinel  (issue #5 — decision D5)

The tripwire: the core stores `ids_properties/version_put/data_dictionary` as
opaque data and interprets no DD version. A current-behavior test that Task 3's
version-negotiation work will watch.

| Capability | Inventory ref | Unit | Integ. | Test(s) | Status |
|---|---|:--:|:--:|---|---|
| `version_put/data_dictionary` round-trips verbatim; writing it does **not** change `getDDVersion()` | CLAUDE.md; :455-459 | ✓ (Memory) | ✓ (HDF5) | `Backends/VersionSentinel.VersionPutRoundTripsOpaquelyAndIsNotInterpreted` | covered |

---

## Already covered by the scaffold slice (#2 — TEST_STRATEGY §4 step 1)

| Capability | Inventory ref | Unit | Integ. | Test(s) | Status |
|---|---|:--:|:--:|---|---|
| `al_write_data`→`al_read_data` round trip (INTEGER scalar + CHAR array) | Clusters 1-2 | ✓ (Memory) | ✓ (HDF5) | `Backends/RoundTrip.IntScalarAndCharArraySurviveWriteThenRead` | covered |
| `al_setvalue_int_scalar_parameter_plugin` on an unregistered name must return an error, not crash | :410-417 | — | ✓ | `KnownDefects.SetValueIntScalarUnregisteredPluginReturnsError` (DISABLED_) | **xfail** |
| ↳ current-behavior guard: that call crashes by SIGSEGV today | :410-417 | — | ✓ | `KnownDefectsDeath.SetValueIntScalarUnregisteredPluginCurrentlyCrashes` | covered (tripwire) |

## Cluster 2 — Core data access: `al_write_data` / `al_read_data` (issue #3 — TEST_STRATEGY §4 step 2)

The synthetic write→read round-trip matrix — the bulk of the storage contract.
Oracle = round-trip self-consistency (decision D5); synthetic opaque data, zero
DD artifacts. Parametrized over the always-on tier (decision D4)
{HDF5, Memory, ASCII, Flexbuffers} × {CHAR, INTEGER, DOUBLE, COMPLEX} ×
{scalar → 7-D}. Each cell is classified once (D2/D4): a plain round trip, a
documented refusal (paired-negative), or a genuine defect (expected-fail).
Flexbuffers is a serializer, not a pulse store, so every one of its cells is a
must-refuse column (it accepts the write but refuses the read-back — D4's
prescription for an unsupported operation).

| Capability | Inventory ref | Unit | Integ. | Test(s) | Status |
|---|---|:--:|:--:|---|---|
| `al_write_data`→`al_read_data` round trip preserves value + shape, INTEGER/DOUBLE/COMPLEX, scalar→7-D, on HDF5·Memory·ASCII (ASCII: scalar→6-D) | Cluster 2 | ✓ (Memory) | ✓ (HDF5, ASCII) | `Backends/RoundTripMatrix.ReadEqualsWrite/{HDF5,Memory,ASCII}_{INTEGER,DOUBLE,COMPLEX}_r{0..7}` | covered |
| Round trip for CHAR — scalar & 1-D (string) & 2-D (array-of-strings) on HDF5·ASCII; all ranks on Memory (raw bytes) | Cluster 2 | ✓ (Memory) | ✓ (HDF5, ASCII) | `…/{Memory}_CHAR_r{0..7}`, `…/{HDF5,ASCII}_CHAR_r{1,2}` | covered |
| Deterministic synthetic-data generator: datatype × shape (scalar→7-D), values clear of the EMPTY sentinels | D5 | ✓ | ✓ | `al_contract.h` `synth_value`/`synth_buffer`/`shape_for_rank`; exercised by every `RoundTripMatrix` cell | covered |
| Flexbuffers must-refuse column: accepts the write but refuses the read-back (serializer, not a pulse store), every datatype × shape | Cluster 2 / D4 | — | ✓ | `Backends/RoundTripMatrix.ReadEqualsWrite/Flexbuffers_*_r{0..7}` (paired-negative) | covered |
| CHAR data > 2-D is refused (documented "not implemented"): HDF5 & ASCII | Cluster 2 / D4 | — | ✓ | `Backends/RoundTripMatrix.ReadEqualsWrite/{HDF5,ASCII}_CHAR_r{3..7}` (paired-negative) | covered |
| A CHAR scalar (dim 0) must round-trip, not crash — **HDF5 crashes** | Cluster 2 | — | ✓ | `RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips` + `RoundTripKnownDefectsDeath.Hdf5CharScalarCurrentlyCrashes` | **xfail** |
| Numeric data at the MAXDIM boundary (rank 7) must round-trip — **ASCII corrupts (DOUBLE/COMPLEX) / aborts (INTEGER)** | Cluster 2 | — | ✓ | `RoundTripKnownDefects.DISABLED_Ascii{Integer,Double,Complex}MaxdimRoundTrips` | **xfail** |
| `al_build_uri_from_legacy_parameters` must address the always-on FLEXBUFFERS backend — **it throws (getURIBackend has no case)** | src/al_context.cpp:280 | ✓ | — | `RoundTripKnownDefects.DISABLED_BuildUriSupportsFlexbuffers` | **xfail** |

> **Deferred (recorded):** a full Flexbuffers **serialize→deserialize** round
> trip over the datatype × shape space needs the HLI `<buffer>` protocol and
> belongs to the serialize seam; the matrix only pins its must-refuse pulse-
> lifecycle behavior. Occurrences, `al_delete_data`, AOS, and the pulse
> lifecycle (Cluster 1) remain with issue #4 (structured data). CHAR>2-D on
> Memory round-trips incidentally (raw bytes) and is asserted as such; it is not
> part of the CHAR contract.

---

## Cluster E — capability-gated ops (issue #6 — TEST_STRATEGY §4 step 4)

slice / timerange / `list_filled_paths` — positive where the backend advertises
the capability, **paired-negative** (documented refusal) where it does not
(decision D4). The support columns were established empirically and follow D2,
not the issue's stated assumptions: **Memory genuinely supports slice** (full
`SLICE_OP` handling in `src/memory_backend.cpp`), so it is positive for slice and
negative only for time-range/list. Refusal codes are the stable contract:
timerange → `LOWLEVEL_ERR` (`src/al_lowlevel.cpp:1053`); slice / list_filled_paths
→ `BACKEND_ERR` (backend throws `ALBackendException`). Because decision D1 forbids
linking the C++ `Backend` flags, `supportsTimeRangeOperation()` /
`supportsTimeDataInterpolation()` are asserted through their sole ABI-observable
consequence — whether the op is accepted or refused per backend — which is what
puts the Cluster-E matrix itself under test.

| Capability | Inventory ref | Unit | Integ. | Test(s) | Status |
|---|---|:--:|:--:|---|---|
| `al_begin_timerange_action` (READ) returns the stored slices in `[tmin,tmax]` (no-resample) on a supporting backend — HDF5 | Cluster E | — | ✓ | `Backends/CapabilityMatrix.TimeRangeReadPositiveOrRefused/HDF5`, `Hdf5TimeDependent.TimeRangeReadWithoutResampling` | covered |
| `al_begin_timerange_action` resamples onto an explicit `dtime` grid (LINEAR) — HDF5 | Cluster E | — | ✓ | `Hdf5TimeDependent.TimeRangeReadWithResampling` | covered |
| `al_begin_timerange_action` must refuse where `supportsTimeRangeOperation()==false` (`LOWLEVEL_ERR`) — Memory·ASCII·Flexbuffers | Cluster E / D4 | ✓ (Memory) | ✓ | `Backends/CapabilityMatrix.TimeRangeReadPositiveOrRefused/{Memory,ASCII,Flexbuffers}` (paired-negative) | covered |
| `al_begin_slice_action` (READ) selects a slice by CLOSEST/PREVIOUS/LINEAR interp — HDF5 | Cluster E | — | ✓ | `Hdf5TimeDependent.SliceInterpolationModes`, `Backends/CapabilityMatrix.SliceReadPositiveOrRefused/HDF5` | covered |
| `al_begin_slice_action` (READ, CLOSEST) is supported on Memory (slice storage, no interpolation gate) | Cluster E | ✓ (Memory) | — | `Backends/CapabilityMatrix.SliceReadPositiveOrRefused/Memory` | covered |
| `al_begin_slice_action` (WRITE, append via `UNDEFINED_TIME`) accepted on a slicing backend — HDF5·Memory | Cluster E | ✓ (Memory) | ✓ | `Backends/CapabilityMatrix.SliceWriteBeginPositiveOrRefused/{HDF5,Memory}` | covered |
| `al_begin_slice_action` must refuse on a non-slicing backend (`BACKEND_ERR`) — ASCII·Flexbuffers | Cluster E / D4 | — | ✓ | `Backends/CapabilityMatrix.{SliceReadPositiveOrRefused,SliceWriteBeginPositiveOrRefused}/{ASCII,Flexbuffers}` (paired-negative) | covered |
| `al_begin_slice_action` (READ) with `UNDEFINED_INTERP` is refused (an interp mode is required; `CONTEXT_ERR`) | src/al_context.cpp:347 | — | ✓ | `Hdf5TimeDependent.SliceReadWithoutInterpModeIsRejected` | covered |
| `al_list_filled_paths` returns the filled leaf paths (caller frees list + strings) — HDF5 | :494-505 | — | ✓ | `Backends/CapabilityMatrix.ListFilledPathsPositiveOrRefused/HDF5` | covered |
| `al_list_filled_paths` must refuse where unimplemented (`BACKEND_ERR`, "only tensorizing backends") — Memory·ASCII·Flexbuffers | :496-497 / D4 | ✓ (Memory) | ✓ | `Backends/CapabilityMatrix.ListFilledPathsPositiveOrRefused/{Memory,ASCII,Flexbuffers}` (paired-negative) | covered |
| Slice **append** via `UNDEFINED_TIME`: repeated WRITE slices must accumulate — **only the last-written slice persists in this build** | Cluster E; al_lowlevel.h:300-316 | — | ✓ | `Hdf5SliceAppend.AppendedSlicesAllPersist` (DISABLED_) | **xfail** |
| ↳ current-behavior tripwire: two appends leave exactly one stored slice today | Cluster E | — | ✓ | `Hdf5SliceAppend.OnlyLastAppendedSlicePersists_CurrentBehavior` | covered (tripwire) |

> **Note on append vs read.** Because append does not accumulate (the xfail
> above), the interpolation/resampling oracles seed their multi-point signal via
> a single global write and exercise the slice/timerange **read** machinery over
> it — the read path is independent of how the samples reached the store. The
> write-lifecycle fix itself is owned by issue #3.

## Remaining clusters — gaps to be filled by later build-order steps

Per TEST_STRATEGY.md §4, still open (not covered by this issue):

- **Cluster 1 — Pulse lifecycle**: open/info/close/erase, occurrences (step 2).
- **Cluster 2 — Data access**: `al_delete_data`, AOS write/iterate/read (step 2).
- **Cluster 3 — Plugins**: register/bind/unbind/unregister, parameter-setting,
  state-machine quirks and the remaining defect death-tests (step 5).
- Backend-implementer / plugin-author capabilities reachable through the C ABI,
  swept until no row is blank (step 6).
