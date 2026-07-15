# Contract-suite traceability matrix (Task 2)

Per **TEST_STRATEGY.md decision D8**, this matrix — not line coverage — is the
**primary definition of "comprehensive."** It has **one row per
`FUNCTIONALITY_INVENTORY.md` capability, across all three Audiences** (User,
Backend implementer, Plugin author), in the inventory's own Part/Cluster order.
A capability with no row here is an omission in *this file*, not an untested
capability that is silently fine — the point of the enumeration is that "done"
is a checkable list.

Status vocabulary:

- **covered** — an intended-contract test asserts the behavior and passes.
- **covered (indirect)** — a Backend-implementer / lower-layer capability with
  no C-ABI seam of its own (decision D1 forbids linking the C++ `Backend`
  classes), asserted through its sole ABI-observable consequence, or a User
  capability exercised as an asserted dependency of another test (e.g. every
  round trip asserts its `al_close_pulse` returns 0). The residual, un-asserted
  modes/paths are named in the row's Test(s)/notes cell.
- **xfail** — a genuine defect; the test asserts the *correct* behavior and is
  expected-fail (decision D2, `DISABLED_` + a paired current-behavior tripwire).
  Flipping to pass means someone fixed it.
- **divergence** (issue #12, Part 4) — not a defect: a legitimate storage-model
  difference where a parity row's synthetic/DD-agnostic test shape doesn't
  transfer to a backend that enforces something the others don't (e.g.
  MDSplus resolving every path against a real DD-baked model tree). The test
  is skipped with an explanation (`GTEST_SKIP()`), not paired with a
  `DISABLED_`/tripwire — there is nothing to "fix," since flipping it would
  require changing the test's own shape, not the backend. Distinct from
  `xfail` (a bug that could be fixed) and from `gap` (not tested at all).
- **gap** — not tested. Either *deferred*, naming the later build-order step
  (TEST_STRATEGY §4) that owns it, or — now that issue #8 (the last build-order
  step) is closing — *terminal*: reachable in principle but out of this
  suite's scope (e.g. needs infrastructure this test target doesn't have), or
  genuinely not observable through the C ABI at all. Each terminal gap says
  which, explicitly, so "gap" here never means "silently fine."

Line coverage (`gcov`/`lcov`) is a secondary signal only (D8): a rewrite may
restructure lines but must still satisfy every row here.

Test IDs are GoogleTest `Suite.Case` names as registered in CTest; run one with
`ctest -R <id>` or `contract_tests --gtest_filter=<id>`. Parametrized cases
carry a `/<Backend>` (or `/<Backend>_<Type>_r<rank>`) suffix per instance.

> **Note on the Unit / Integ. columns.** They classify each test by *substrate*
> per decision D6 (pure lookups and Memory round trips are unit; on-disk
> backends are integration). This taxonomy is now enforced as CTest **labels**
> (`CMakeLists.txt`): every case carries `contract`, plus `unit` or `integration`
> by substrate, so `ctest -L unit` runs the fast hermetic subset (no on-disk
> backend) and `ctest -L integration` runs the on-disk tier. The label split
> needs CMake ≥ 3.22 (`gtest_discover_tests(TEST_FILTER …)`); on the declared
> 3.21 floor it falls back to the single `contract` label.

## Coverage status — as of all six implemented issues (#2 scaffold, #5
## introspection, #3 round-trip matrix, #6 capability-gated, #4 structured
## data, #7 plugins, #8 ownership sweep)

The suite covers the **data-path, introspection, structured-data surface,
plugin-management surface, and Plugin-author audience** end to end and pins
the defects those issues surfaced (twenty xfails, each with a paired
current-behavior tripwire — issue #8 resolved every open ownership/coverage
question by test but found no new genuine defects, so the xfail count is
unchanged). Issue #8 (`6-ownership-sweep.md`) closed every remaining row:

- **Ownership**: `al_context_info`, `al_get_occurrences`'s
  `*occurrences_list`, and `al_list_filled_paths`'s list+strings were already
  pinned malloc/free by issues #4/#6 (`ContextInfo.*`, `Occurrences.*`,
  `CapabilityMatrix.ListFilledPathsPositiveOrRefused`).
  `al_build_uri_from_legacy_parameters`'s `*uri` gets its own explicit pin
  here (`UriOwnership.CallerFreesBuiltUri`), rather than only incidental
  free()s inside the `al_contract::build_uri` test helper.
- **MAXDIM**: resolved empirically — plugin-parameter `dim`/`size` on
  `al_setvalue_parameter_plugin` is **not** bounded by MAXDIM; the core
  passes it through to the plugin unchecked
  (`PluginTest.SetValueGenericAcceptsDimAboveMaxdim`).
- **Plugin-author audience (Part 3)**: low-level reentry (`PluginReentry.*`),
  action-lifecycle data interception
  (`PluginTest.BoundPluginIntercepts{Write,Read}InsteadOfBackend`), provenance
  + `al_write_plugins_metadata`
  (`PluginTest.WritePluginsMetadataStoresBoundPluginProvenance`), and readback
  binding (`ReadbackPlugins.*`, with `test_plugin_fixture.cpp` extended to
  opt into readback capability for one path via
  `AL_CONTRACT_PLUGIN_READBACK_PATH`) are now all covered.
- **Two rows stay `gap`, by design, not omission**: Backend `getVersion`
  drift and `initDataInterpolationComponent` — see their rows in Part 2 for
  why each is a legitimate terminal gap rather than a deferred one.

---

# Part 1 — User (HLI implementer) audience

## Cluster 1 — Pulse lifecycle  (`FUNCTIONALITY_INVENTORY.md:86-149`)

| Capability | Inventory ref | Unit | Integ. | Test(s) — and residual | Status |
|---|---|:--:|:--:|---|---|
| `al_begin_dataentry_action` opens a data entry from a URI | :90-107 | ✓ (Memory) | ✓ (HDF5, ASCII) | `FORCE_CREATE_PULSE` asserted OK in every suite. `OPEN_PULSE` fails when absent / `FORCE_OPEN_PULSE` creates when absent / `OPEN_PULSE` succeeds once present / `CREATE_PULSE` refuses an existing pulse (HDF5, Memory): `Backends/DataEntryModes.*`. MDSplus: joined too — see Part 4 | covered |
| ↳ ASCII's `CREATE_PULSE` has no existence guard — **silently overwrites/truncates an existing pulse** instead of refusing it | :90-107; src/ascii_backend.cpp:107-157 | ✓ | — | `ModeKnownDefects.DISABLED_AsciiCreatePulseFailsWhenAlreadyExists` + tripwire `ModeKnownDefects.AsciiCreatePulseCurrentlyOverwritesSilently` | **xfail** |
| `al_close_pulse` closes (`CLOSE_PULSE`) / erases (`ERASE_PULSE`) | :109-116 | ✓ (Memory) | ✓ (HDF5, ASCII) | `CLOSE_PULSE` asserted `code==0` in every suite | covered (indirect) |
| ↳ `ERASE_PULSE` is not implemented by any always-on backend — **behaves identically to `CLOSE_PULSE`** (none of HDF5/ASCII/Memory's `closePulse` reference their `mode` parameter); a later plain `OPEN_PULSE` on the "erased" pulse still succeeds | :109-116; src/hdf5/{hdf5_reader,hdf5_writer}.cpp closePulse, src/ascii_backend.cpp:165-169, src/memory_backend.h:505-509 | ✓ (Memory) | ✓ (HDF5, ASCII) | `ErasePulseKnownDefects.DISABLED_{Hdf5,Ascii,Memory}EraseMakesPulseUnopenable` + tripwires `ErasePulseKnownDefects.{Hdf5,Ascii,Memory}EraseCurrentlyLeavesPulseOpenable` | **xfail** |
| `al_context_info` describes a context (pulse/operation/arraystruct); **caller frees `*info`** (verified `malloc`-based) | :118-126 | ✓ | ✓ | `ContextInfo.{NullContextReturnsLiteralString, PulseContextDescribesItsUri, OperationContextDescribesDataobjectAndAccessmode, ArraystructContextDescribesPathAndIndex}` (frees via `free()`) | covered |
| `al_get_backendID` returns the active `BACKEND` for a context | :128-135 | — | ✓ | `GetBackendId.ReturnsTheBackendUsedToOpen` (HDF5 only) | covered |
| ↳ no context-type check — a non-pulse context is `static_cast` (not `dynamic_cast`) to `DataEntryContext*` with no validation, so it **silently "succeeds" with an undefined-behavior value** instead of erroring (confirmed empirically: it does not crash) | :128-135; src/al_lowlevel.cpp al_get_backendID | — | ✓ | `GetBackendIdKnownDefects.DISABLED_WrongContextTypeReturnsError` + tripwire `GetBackendIdKnownDefects.WrongContextTypeCurrentlySucceedsViaUnsafeCast` (HDF5 only) | **xfail** |
| `al_build_uri_from_legacy_parameters` builds a URI from legacy params; **caller frees `*uri`** (verified malloc-based, src/al_context.cpp:241-261) | :137-149 | ✓ | ✓ | `al_contract::build_uri` (asserts OK, frees) drives HDF5·Memory·ASCII in all on-disk suites; ownership pinned explicitly by `UriOwnership.CallerFreesBuiltUri` | covered |
| ↳ must also address the always-on FLEXBUFFERS backend — **throws (`getURIBackend` has no case)** | :137-149; src/al_context.cpp:280 | ✓ | — | `RoundTripKnownDefects.DISABLED_BuildUriSupportsFlexbuffers` + tripwire `RoundTripKnownDefects.BuildUriFlexbuffersCurrentlyFails` | **xfail** |

## Cluster 2 — Core data access  (`FUNCTIONALITY_INVENTORY.md:153-314`)

The bulk of the storage contract (issue #3) plus the capability-gated ops
(issue #6). Round-trip oracle = self-consistency (decision D5); synthetic opaque
data, zero DD artifacts. The round-trip matrix is parametrized over the
always-on tier (decision D4) {HDF5, Memory, ASCII, Flexbuffers} × {CHAR,
INTEGER, DOUBLE, COMPLEX} × {scalar → 7-D}; each cell is classified once
(D2/D4) as a plain round trip, a documented refusal (paired-negative), or a
genuine defect (expected-fail).

| Capability | Inventory ref | Unit | Integ. | Test(s) — and residual | Status |
|---|---|:--:|:--:|---|---|
| `al_begin_global_action` starts a GLOBAL read/write op | :159-175 | ✓ (Memory) | ✓ (HDF5, ASCII) | Vehicle for every round trip: `RoundTrip.*`, `RoundTripMatrix.*`, `VersionSentinel.*`, `CapabilityMatrix.*` (WRITE_OP + READ_OP). `datapath` partial-get is UDA-only (out of always-on scope). MDSplus joins `CapabilityMatrix` only, not `RoundTripMatrix`/`VersionSentinel` (opaque synthetic paths) — see Part 4 | covered |
| `al_begin_slice_action` starts a time-slice op (READ interp; WRITE append via `UNDEFINED_TIME`) | :177-194 | ✓ (Memory) | ✓ (HDF5) | HDF5·Memory positive / ASCII·Flexbuffers refused `BACKEND_ERR`: `CapabilityMatrix.SliceReadPositiveOrRefused`, `CapabilityMatrix.SliceWriteBeginPositiveOrRefused`. Interp modes + missing-interp refusal (`CONTEXT_ERR`): `Hdf5TimeDependent.SliceInterpolationModes`, `Hdf5TimeDependent.SliceReadWithoutInterpModeIsRejected`. MDSplus: see Part 4 (real interpolation round trip, plus struct-aware slice interpolation as a unique surface) | covered |
| ↳ WRITE append via `UNDEFINED_TIME` must accumulate — **only the last slice persists in this build** | :177-194; al_lowlevel.h:300-316 | — | ✓ | `Hdf5SliceAppend.DISABLED_AppendedSlicesAllPersist` + tripwire `Hdf5SliceAppend.OnlyLastAppendedSlicePersists_CurrentBehavior` | **xfail** |
| `al_begin_timerange_action` starts a time-range op (with/without resampling `dtime`) | :196-221 | ✓ (Memory) | ✓ (HDF5) | HDF5 positive (no-resample + LINEAR resample): `Hdf5TimeDependent.TimeRangeReadWithoutResampling`, `…WithResampling`; Memory·ASCII·Flexbuffers refused `LOWLEVEL_ERR`: `CapabilityMatrix.TimeRangeReadPositiveOrRefused`. MDSplus: also refused, unconditionally — see Part 4 | covered |
| `al_begin_arraystruct_action` starts an AOS op (top-level + nested), write→iterate→read | :223-233 | ✓ (Memory) | ✓ (HDF5) | `Backends/AosMatrix.{TopLevelWriteIterateRead,NestedWriteIterateRead}/{HDF5,Memory}`; Flexbuffers refused (paired-negative, same read-refusal as the round-trip matrix): `…/Flexbuffers`. MDSplus: joined too, but a divergence (synthetic paths) — see Part 4 | covered |
| ↳ ASCII's AOS **read** always reports size 0, for any AOS, regardless of what was written — `beginAction`'s `READ_OP` setup consumes the whole file into a random-access lookup map before the AOS-size lookup's sequential cursor ever runs | :223-233; src/ascii_backend.cpp:213-235,656-682 | ✓ | — | `AosKnownDefects.DISABLED_AsciiAosReadReportsWrittenSize` + tripwire `AosKnownDefects.AsciiAosReadCurrentlyReportsZero`; matrix cells skip via `Backends/AosMatrix.*/ASCII` | **xfail** |
| `al_end_action` ends any context | :235-242 | ✓ (Memory) | ✓ (HDF5, ASCII) | asserted OK in every suite | covered (indirect) |
| `al_write_data` → `al_read_data` round trip preserves value + shape (INTEGER/DOUBLE/COMPLEX scalar→7-D on HDF5·Memory·ASCII; CHAR scalar/1-D/2-D on HDF5·ASCII, all ranks on Memory) | :244-263 | ✓ (Memory) | ✓ (HDF5, ASCII) | `Backends/RoundTripMatrix.ReadEqualsWrite/*`; generator `al_contract.h` `synth_value`/`synth_buffer`/`shape_for_rank`. MDSplus excluded from this fixture (opaque synthetic paths) — see Part 4's real-DD-path breadth matrix instead | covered |
| ↳ Flexbuffers must-refuse column (serializer, not a pulse store): write accepted, read refused, every datatype × shape | :244-263 / D4 | — | ✓ | `…/Flexbuffers_*_r{0..7}` (paired-negative) | covered |
| ↳ CHAR > 2-D refused (documented "not implemented") on HDF5·ASCII | :244-263 / D4 | — | ✓ | `…/{HDF5,ASCII}_CHAR_r{3..7}` (paired-negative) | covered |
| ↳ CHAR scalar (dim 0) must round-trip — **HDF5 crashes** | :244-263 | — | ✓ | `RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips` + tripwire `RoundTripKnownDefectsDeath.Hdf5CharScalarCurrentlyCrashes` | **xfail** |
| ↳ numeric at MAXDIM (rank 7) must round-trip — **ASCII corrupts (DOUBLE/COMPLEX) / aborts (INTEGER)** | :244-263 | — | ✓ | `RoundTripKnownDefects.DISABLED_Ascii{Integer,Double,Complex}MaxdimRoundTrips` + tripwires `RoundTripKnownDefectsDeath.AsciiIntegerMaxdimCurrentlyAborts`, `RoundTripKnownDefects.Ascii{Double,Complex}MaxdimCurrentlyCorrupts` | **xfail** |
| `al_delete_data` at signal / structure / DATAOBJECT-root granularity | :265-272 | ✓ (Memory) | ✓ (HDF5) | Support is disjoint per backend, none implement all three (see xfail rows below): `Backends/DeleteMatrix.{LeafDeleteRemovesJustTheLeaf,StructureDeleteRemovesWholeSubtree}/Memory` (leaf + structure genuinely work only on Memory), `…/RootDeleteRemovesWholeOccurrence/HDF5` (root genuinely works only on HDF5, which ignores `path` and always removes the whole occurrence). MDSplus: not joined to this fixture — terminal gap, see Part 4 (issue #18) | covered |
| ↳ HDF5 ignores `path` for leaf/structure delete — **any delete call removes the whole occurrence**, not just the named field | :265-272; src/hdf5/hdf5_writer.cpp deleteData (no path parameter at all) | — | ✓ | One tripwire covers both defective granularities, since there is no leaf-vs-structure branch to differ (`HDF5Writer::deleteData` never receives a path at all): `DeleteKnownDefects.DISABLED_Hdf5LeafDeleteLeavesSiblingIntact` + tripwire `DeleteKnownDefects.Hdf5LeafDeleteCurrentlyWipesWholeOccurrence`; matrix cells skip via `Backends/DeleteMatrix.{Leaf,Structure}*/HDF5` | **xfail** |
| ↳ Memory has no code path for DATAOBJECT-root delete — **a root-granularity delete is silently a no-op** | :265-272; src/memory_backend.cpp deleteData (no root/occurrence case) | ✓ | — | `DeleteKnownDefects.DISABLED_MemoryRootDeleteClearsWholeIds` + tripwire `DeleteKnownDefects.MemoryRootDeleteCurrentlyDoesNothing`; matrix cell skips via `Backends/DeleteMatrix.RootDeleteRemovesWholeOccurrence/Memory` | **xfail** |
| ↳ ASCII's `deleteData` is a literal no-op (empty body) at every granularity; Flexbuffers' is too but is not independently ABI-observable (it already refuses every in-session read regardless of delete, so a read-based oracle can't distinguish "delete did nothing" from "this backend never round-trips reads") | :265-272; src/ascii_backend.cpp:648-652, src/flexbuffers_backend.cpp:387-389 | ✓ | ✓ | One tripwire covers every granularity on ASCII, since an empty function body has no path-dependent branch to differ: `DeleteKnownDefects.DISABLED_AsciiDeleteRemovesLeaf` + tripwire `DeleteKnownDefects.AsciiDeleteIsCurrentlyANoOp`; matrix cells skip via `Backends/DeleteMatrix.*/ASCII` and `…/Flexbuffers` | **xfail** |
| `al_iterate_over_arraystruct` advances the AOS cursor | :274-282 | ✓ (Memory) | ✓ (HDF5) | Exercised as an asserted dependency of every AOS write/read in `Backends/AosMatrix.*` (top-level and nested, multi-step) | covered (indirect) |
| `al_get_occurrences` lists non-empty occurrences; caller-frees `*occurrences_list` (verified `malloc`-based on both implementing backends) | :284-292 | ✓ (Memory) | ✓ (HDF5, ASCII) | HDF5 and ASCII implement it for real, each against its own occurrence-naming convention (neither transforms `dataobjectname`, so the caller supplies the backend-native form — HDF5: `"<ids>_<N>"`; ASCII: `"<ids>/<N>"`): `Occurrences.{Hdf5,Ascii}ListsWrittenOccurrences`. Memory/Flexbuffers refuse unconditionally (not implemented): `Occurrences.{Memory,Flexbuffers}Refuses`. MDSplus: also real, same `<ids>/<N>` convention — see Part 4 (`Occurrences.MdsplusListsWrittenOccurrences`) | covered |
| `al_list_filled_paths` lists filled leaf paths; **caller frees list + strings** | :294-314 | ✓ (Memory) | ✓ (HDF5) | HDF5 positive (leaves discoverable, list freed) / Memory·ASCII·Flexbuffers refused `BACKEND_ERR`: `CapabilityMatrix.ListFilledPathsPositiveOrRefused`. MDSplus: refused `BACKEND_ERR` too, same as Memory·ASCII·Flexbuffers — see Part 4 | covered |

### Equilibrium seed (issue #4 — decision D5)

The deterministic, in-repo single-version equilibrium-seed generator (a
scalar, a timebase-carrying 2-D array, and a constraints AOS — no committed
binary blobs; oracle = a content hash recomputed from the generator functions
themselves, `tests/contract/equilibrium_seed.h`), round-tripped on the three
backends where AOS content is actually readable back within a session.

| Capability | Inventory ref | Unit | Integ. | Test(s) | Status |
|---|---|:--:|:--:|---|---|
| Realistic AOS/timebase/HDF5-tensorization shape round-trips exactly | D5 | ✓ (Memory) | ✓ (HDF5, ASCII) | `Backends/EquilibriumSeedMatrix.RoundTripHashMatches/{HDF5,Memory,ASCII}`. MDSplus is instantiated too, but a divergence (the seed's generic shapes don't match the real equilibrium DD layout) — see Part 4 | covered |

## Cluster 3 — Plugin management  (`FUNCTIONALITY_INVENTORY.md:318-430`)

The plugins issue (#7, step 5) — the defect-heavy corner. The register / bind /
unbind / unregister state machine and the `al_setvalue_*` calls are driven
through the C ABI against a real, in-repo loadable plugin
(`test_plugin_fixture.cpp` → `alcontract_plugin.so`, the classic red-green
fixture); every behavior was characterized against the built library, then
classified once (D2). Four defects are pinned as expected-fail with paired
current-behavior tripwires: the two setvalue null-derefs (double + generic,
joining the scaffold's int one), the registered-but-never-bound plugin left
un-destroyed on unregister, and the `dlopen`-failure swallowed assert. The whole
suite is `unit` (in-process registry, no on-disk backend). Readback binding
and `al_write_plugins_metadata`, deferred at the time this paragraph was
written, are now covered — issue #8 extended `test_plugin_fixture.cpp` with
an env-var-gated readback capability and drove a real put→write-metadata→
unregister→bind_readback_plugins cycle over a Memory-backend pulse (see
their rows below and Part 3).

| Capability | Inventory ref | Unit | Integ. | Test(s) — and residual | Status |
|---|---|:--:|:--:|---|---|
| `al_register_plugin` / `al_unregister_plugin` lifecycle (register happy path; register-twice throws; unregister-unknown throws; **framework-gated**) | :325-360 | ✓ | — | `PluginTest.{RegisterMakesPluginRegistered, RegisterTwiceReturnsError, UnregisterNeverRegisteredNameReturnsError, UnregisterBoundPluginDestroysIt, RegisterWithFrameworkDisabledReturnsError}` | covered |
| ↳ unregister of a **registered-but-never-bound** plugin must destroy it — **it is left un-destroyed** (`unregisterPlugin` destroy+erase run only inside the `boundPlugins` loop) | :347-354; src/al_lowlevel.cpp:382-431 | ✓ | — | `PluginTest.DISABLED_UnregisterNeverBoundPluginDestroysIt` + tripwire `PluginTest.UnregisterNeverBoundPluginLeavesItRegistered_CurrentBehavior` | **xfail** |
| ↳ `register` on an existing-but-unloadable `.so` must report the **dlopen** failure — the failure is guarded only by an `assert`, stripped under `-DNDEBUG` (misleading downstream "Cannot load symbol create"; would `SIGABRT` in an assert-enabled build) | :355-360; src/al_lowlevel.cpp:350-357 | ✓ | — | `PluginTest.DISABLED_RegisterUnloadableSharedLibReportsDlopenFailure` + tripwire `PluginTest.RegisterUnloadableSharedLibSwallowsAssert_CurrentBehavior` | **xfail** |
| `al_bind_plugin` / `al_unbind_plugin` (bind registered OK; bind-unregistered throws; double-bind throws; unbind-unbound silent no-op) | :362-376 | ✓ | — | `PluginTest.{BindRegisteredPluginSucceeds, BindUnregisteredPluginReturnsError, DoubleBindSamePathReturnsError, UnbindNeverBoundPathIsSilentNoOp}` | covered |
| `al_bind_readback_plugins` / `al_unbind_readback_plugins` (auto-register + auto-bind from stored metadata before a get; auto-unregister after) | :378-389 | ✓ | — | `ReadbackPlugins.BindReadbackPluginsAutoRegistersAndBindsFromStoredMetadata` — `test_plugin_fixture.cpp` extended to opt into readback capability for one path via `AL_CONTRACT_PLUGIN_READBACK_PATH`; the bind is confirmed indirectly (double-bind now errors) since there is no direct `al_is_plugin_bound` ABI | covered |
| `al_is_plugin_registered` boolean query (true/false; framework-gated) | :391-397 | ✓ | — | `PluginTest.{RegisterMakesPluginRegistered, IsPluginRegisteredIsFalseForNeverRegisteredName, IsPluginRegisteredWithFrameworkDisabledReturnsError}` | covered |
| `al_setvalue_parameter_plugin` (generic typed variant) on a registered plugin | :399-420 | ✓ | — | `PluginTest.SetValueGenericOnRegisteredPluginSucceeds` | covered |
| ↳ generic variant on an **unregistered** name must error, not crash | :399-417 | ✓ | — | `KnownDefects.DISABLED_SetValueGenericUnregisteredPluginReturnsError` + tripwire `KnownDefectsDeath.SetValueGenericUnregisteredPluginCurrentlyCrashes` (SIGSEGV) | **xfail** |
| `al_setvalue_int_scalar_parameter_plugin` on a registered plugin reaches `setParameter` with the right value | :399-417 | ✓ | — | `PluginTest.SetValueIntScalarOnRegisteredPluginReachesPlugin` (asserts the value via the plugin's parameter log) | covered |
| ↳ int-scalar variant on an **unregistered** name must error, not crash | :399-417 | ✓ | — | `KnownDefects.DISABLED_SetValueIntScalarUnregisteredPluginReturnsError` + tripwire `KnownDefectsDeath.SetValueIntScalarUnregisteredPluginCurrentlyCrashes` (SIGSEGV) | **xfail** |
| `al_setvalue_double_scalar_parameter_plugin` on a registered plugin reaches `setParameter` with the right value | :399-417 | ✓ | — | `PluginTest.SetValueDoubleScalarOnRegisteredPluginReachesPlugin` | covered |
| ↳ double-scalar variant on an **unregistered** name must error, not crash | :399-417 | ✓ | — | `KnownDefects.DISABLED_SetValueDoubleScalarUnregisteredPluginReturnsError` + tripwire `KnownDefectsDeath.SetValueDoubleScalarUnregisteredPluginCurrentlyCrashes` (SIGSEGV) | **xfail** |
| `al_write_plugins_metadata` (stores bound put-capable plugins' provenance + any readback contribution under `ids_properties/plugins/node`) | :422-430 | ✓ | — | `PluginTest.WritePluginsMetadataStoresBoundPluginProvenance` (Memory backend; verifies stored `path`, `put_operation[]` provenance fields, and an empty `readback[]` for a non-readback-opted-in bind) | covered |

## Cluster 4 — Introspection / diagnostics  (`FUNCTIONALITY_INVENTORY.md:434-459`)  (issue #5 — the thin unit tier)

Pure lookups: no context, no backend, zero DD artifacts (D5/D6). All **intended
contract** (D2) — nothing here is a defect, including the silent `""`.

| Capability | Inventory ref | Unit | Integ. | Test(s) | Status |
|---|---|:--:|:--:|---|---|
| `const2str(id)` maps a known constant to its symbolic name | :443-444 | ✓ | — | `Introspection.Const2StrMapsKnownConstantsToTheirNames` | covered |
| `const2str` silently returns `""` for an unmapped id (intended) | :451-452 | ✓ | — | `Introspection.Const2StrReturnsEmptyForUnmappedId` | covered |
| `const2str` returns `""` for defined-but-unmapped constants (`TIMERANGE_OP`, `FLEXBUFFERS_BACKEND`) | :75-79, :451-454 | ✓ | — | `Introspection.Const2StrReturnsEmptyForDefinedButUnmappedConstants` | covered |
| `err2str(id)` maps a known error code to its name | :445-446 | ✓ | — | `Introspection.Err2StrMapsKnownErrorCodesToTheirNames` | covered |
| `err2str` silently returns `""` for an unmapped id | :451-452 | ✓ | — | `Introspection.Err2StrReturnsEmptyForUnmappedId` | covered |
| `const2str` / `err2str` are backed by separate maps (two namespaces) | :443-446 | ✓ | — | `Introspection.Const2StrAndErr2StrUseSeparateMaps` | covered |
| `getALVersion()` returns the compiled AL version string | :447 | ✓ | — | `Introspection.GetALVersionReturnsCompiledVersionMacro`, `…IsDottedNumeric` | covered |
| `getDDVersion()` returns the deprecated sentinel `"!!DEPRECATED!!"` (intended) | :455-459 | ✓ | — | `Introspection.GetDDVersionReturnsDeprecatedSentinel` | covered |

### Version sentinel  (issue #5 — decision D5)

The tripwire: the core stores `ids_properties/version_put/data_dictionary` as
opaque data and interprets no DD version. A current-behavior test Task 3's
version-negotiation work will watch.

| Capability | Inventory ref | Unit | Integ. | Test(s) | Status |
|---|---|:--:|:--:|---|---|
| `version_put/data_dictionary` round-trips verbatim; writing it does **not** change `getDDVersion()` | CLAUDE.md; :455-459 | ✓ (Memory) | ✓ (HDF5) | `Backends/VersionSentinel.VersionPutRoundTripsOpaquelyAndIsNotInterpreted` | covered |

---

# Part 2 — Backend implementer audience  (`FUNCTIONALITY_INVENTORY.md:482-690`)

Exercised **through the C ABI** (decision D1 forbids linking the C++ `Backend`
classes), so these mirror Part 1 rows from the storage layer's side. Where a
Part-1 op above already drives the backend method, this is "covered (indirect)"
and points at the same test.

| Cluster / Capability | Inventory ref | Unit | Integ. | Test(s) — and residual | Status |
|---|---|:--:|:--:|---|---|
| **A** `initBackend` factory selects the backend from the URI | :496-514 | ✓ (Memory) | ✓ (HDF5, ASCII) | every always-on backend instantiated via `build_uri` in `RoundTripMatrix`. Residual: the unrecognized-ID throw path (`al_backend.cpp:82-86`) is straightforward by inspection (a plain `else` throwing `ALBackendException`) but untested — low-value relative to the rest of this row, left as a residual rather than a separate row. MDSplus is instantiated too, via its own URI in every Part 4 fixture — see Part 4 | covered (indirect) |
| **B** `getVersion` (installed vs stored, drift check) | :520-540 | — | — | Reachable in principle: `al_begin_dataentry_action`/`al_plugin_begin_global_action`/`al_plugin_begin_slice_action` all compare `backend->getVersion(NULL)` (compiled) against `backend->getVersion(pctx)` (stored) on open/write and throw `LOWLEVEL_ERR` on a mismatch (src/al_lowlevel.cpp:868-883,956-967,1011-1022). Forcing the mismatch needs writing a pulse then overwriting its on-disk HDF5 backend-version attribute directly via the HDF5 C API — out of this issue's scope (no HDF5 include/link wiring in the test target). **Terminal gap**, not deferred to a future step: reachable via the C ABI, but pinning it needs infrastructure this suite does not have. **MDSplus: this gap is closed** — see Part 4's version-drift check (issue #16), which forces the mismatch via the compile-guarded tier's own raw MDSplus C++ API dependency. This row (HDF5) is unchanged — the two backends hit the identical shape of problem, but only MDSplus's already-separate, already-gated tier made the raw-API workaround low-blast-radius | gap |
| **B** `openPulse` / `closePulse` | :542-552 | ✓ (Memory) | ✓ (HDF5, ASCII) | via `al_begin_dataentry_action` / `al_close_pulse` in all suites | covered (indirect) |
| **C** `beginAction` / `endAction` | :558-574 | ✓ (Memory) | ✓ (HDF5, ASCII) | via `al_begin_global_action` / `al_end_action` | covered (indirect) |
| **C** `writeData` / `readData` (0=not-found vs 1=success inner convention) | :576-589 | ✓ (Memory) | ✓ (HDF5, ASCII) | via `al_write_data` / `al_read_data` — `RoundTripMatrix` (+ the CHAR-scalar / ASCII-maxdim xfails above). MDSplus: see Part 4's real-DD-path breadth matrix (issue #15) instead of `RoundTripMatrix` | covered |
| **C** `deleteData` | :591-599 | ✓ (Memory) | ✓ (HDF5) | via `al_delete_data` — see Cluster 2 row above and its three xfail rows. MDSplus: terminal gap — see Part 4 (issue #18) | covered (indirect) |
| **C** `beginArraystructAction` | :601-606 | ✓ (Memory) | ✓ (HDF5) | via `al_begin_arraystruct_action` — see Cluster 2 row above. MDSplus: joined (divergence) plus its own unique struct-aware slice/segment-write surface — see Part 4 | covered (indirect) |
| **D** `get_occurrences` | :612-616 | — | ✓ | via `al_get_occurrences` — `Occurrences.*`. MDSplus: see Part 4 | covered (indirect) |
| **D** `list_filled_paths` (per-backend: HDF5 real, others throw) | :618-640 | ✓ (Memory) | ✓ (HDF5) | via `al_list_filled_paths` — `CapabilityMatrix.ListFilledPathsPositiveOrRefused` pins both the HDF5 impl and the unconditional throw on Memory·ASCII·Flexbuffers. MDSplus: throws unconditionally too, same as Memory·ASCII·Flexbuffers — see Part 4 | covered |
| **E** `supportsTimeDataInterpolation` / `supportsTimeRangeOperation` | :644-690 | ✓ (Memory) | ✓ (HDF5) | asserted through their sole ABI consequence (op accepted vs refused per backend): `CapabilityMatrix.{TimeRange,Slice,SliceWriteBegin}*`. The empirically-corrected fact that **Memory supports slice** (D2 over the issue's assumption) is encoded here. MDSplus: `supportsTimeDataInterpolation` unconditionally `true`, `supportsTimeRangeOperation` unconditionally `false` — see Part 4 | covered (indirect) |
| **E** `initDataInterpolationComponent` (framework-driven) | :644-690 | — | — | Confirmed unconditionally invoked from `Backend::create()` for any backend that supports slice or timerange (src/al_backend.cpp:88-89), i.e. on every relevant `al_begin_dataentry_action` already exercised elsewhere in this suite — but it has no side effect distinguishable through the C ABI beyond enabling the slice/timerange machinery `Hdf5TimeDependent.*`/`CapabilityMatrix.*` already cover. **Terminal gap**: genuinely not separately ABI-observable, not merely untested. Applies identically to MDSplus (also slice-capable) — same terminal gap, not a separate one | gap |

---

# Part 3 — Plugin author audience  (`FUNCTIONALITY_INVENTORY.md:708-895`)

Entirely deferred to the plugins issue (step 5). Listed as explicit gap rows so
the plugins issue has a concrete checklist rather than a blank slate.

| Cluster / Capability | Inventory ref | Unit | Integ. | Test(s) | Status |
|---|---|:--:|:--:|---|---|
| **1** Provenance metadata (`getName`/`getVersion`/`getCommit`/…) | :739-759 | ✓ | — | Read back from stored metadata: `PluginTest.WritePluginsMetadataStoresBoundPluginProvenance` asserts `name`/`version`/`commit`/`repository` match `ContractTestPlugin`'s provenance; `ReadbackPlugins.*` additionally exercises `getReadbackVersion` being checked for self-consistency against `getVersion` | covered |
| **2** Parameter configuration (`setParameter` / `setParameters`) | :763-780 | ✓ | — | `setParameter` is reached (with the exact value) via `al_setvalue_*` on the registered fixture plugin: `PluginTest.SetValue{IntScalar,DoubleScalar}OnRegisteredPluginReachesPlugin` (asserted through the plugin's parameter log). The User-side unregistered-name crash is xfail'd in Part 1 Cluster 3. `setParameters` (bulk) has no C-ABI seam of its own (only the single-parameter `al_setvalue_*` functions exist) — it is invoked internally as part of `al_bind_readback_plugins` (`ReadbackPlugins.*` exercises the call, with an empty `parameters` string) but a caller cannot address it directly, so a residual "test bulk setParameters directly" is not achievable through the C ABI (D1). **Terminal gap** for the direct-call residual, not deferred | covered (indirect) |
| **3** Action lifecycle & data interception (`begin_*_action`, `read_data`/`write_data`, `node_operation`) | :784-824 | ✓ | — | `al_write_data`/`al_read_data` check `LLplugin::getBoundPlugins` before touching the backend (al_lowlevel.cpp:1685-1693,1721-1731): `PluginTest.BoundPluginInterceptsWriteInsteadOfBackend` proves a bound plugin's inert `write_data()` absorbs a write (the value never reaches the Memory backend, contrasted with an unbound sibling field that round-trips normally); `PluginTest.BoundPluginInterceptsReadInsteadOfBackend` proves the symmetric case for `read_data()`. `node_operation`'s PUT/GET classification is exercised as an asserted dependency of `findPutOperationPlugins`/`findGetOperationPlugins` in the `WritePluginsMetadata`/`ReadbackPlugins` tests. `begin_*_action` hooks on a *regular* bound plugin remain inert-only (`ContractTestPlugin`'s are no-ops) — no test drives a plugin that does real work in them, since none of the existing behaviors need one | covered (indirect) |
| **4** Readback metadata (`getReadback*`) | :827-850 | ✓ | — | `ReadbackPlugins.BindReadbackPluginsAutoRegistersAndBindsFromStoredMetadata` — `test_plugin_fixture.cpp`'s `ContractTestPlugin` extended with `AL_CONTRACT_PLUGIN_READBACK_PATH` to opt a single path into readback capability, self-consistent with its own provenance (so the version-match check in `bind_readback_plugins` passes) | covered |
| **5** Low-level reentry (`al_plugin_*`); `al_plugin_begin_timerange_action` is broken (declaration/definition mismatch) | :854-895 | ✓ | — | `PluginReentry.{WriteDataReentersTheBackendDirectly,ReadDataAlsoReentersTheBackendDirectly}` call `al_plugin_begin_global_action`/`al_plugin_write_data`/`al_plugin_read_data`/`al_plugin_end_action` directly (no plugin object needed — they bypass dispatch straight to the backend) and cross-check against the ordinary `al_write_data`/`al_read_data` path. `al_plugin_begin_timerange_action` is excluded — the reentry bug is tracked separately as an upstream GitHub issue, not re-characterized here | covered |

---

## xfail bookkeeping — every xfail now has a paired tripwire

The D2 discipline is "correct-contract `DISABLED_` test **plus** a paired
current-behavior tripwire, so the xfail can't rot." All twenty xfail rows
satisfy it — a fix to any underlying defect turns its tripwire red, forcing
whoever fixed it to enable the paired `DISABLED_` correct-contract test:

| Defect | Correct-contract (`DISABLED_`) | Current-behavior tripwire |
|---|---|---|
| HDF5 CHAR scalar crashes | `RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips` | `RoundTripKnownDefectsDeath.Hdf5CharScalarCurrentlyCrashes` |
| ASCII rank-7 INTEGER aborts | `…DISABLED_AsciiIntegerMaxdimRoundTrips` | `RoundTripKnownDefectsDeath.AsciiIntegerMaxdimCurrentlyAborts` |
| ASCII rank-7 DOUBLE corrupts (or crashes) | `…DISABLED_AsciiDoubleMaxdimRoundTrips` | `RoundTripKnownDefectsDeath.AsciiDoubleMaxdimCurrentlyCorrupts` |
| ASCII rank-7 COMPLEX corrupts (or crashes) | `…DISABLED_AsciiComplexMaxdimRoundTrips` | `RoundTripKnownDefectsDeath.AsciiComplexMaxdimCurrentlyCorrupts` |
| `build_uri` can't address FLEXBUFFERS | `…DISABLED_BuildUriSupportsFlexbuffers` | `RoundTripKnownDefects.BuildUriFlexbuffersCurrentlyFails` |
| slice append doesn't accumulate | `Hdf5SliceAppend.DISABLED_AppendedSlicesAllPersist` | `Hdf5SliceAppend.OnlyLastAppendedSlicePersists_CurrentBehavior` |
| plugin setvalue (int) unregistered-name crash | `KnownDefects.DISABLED_SetValueIntScalarUnregisteredPluginReturnsError` | `KnownDefectsDeath.SetValueIntScalarUnregisteredPluginCurrentlyCrashes` |
| plugin setvalue (double) unregistered-name crash | `KnownDefects.DISABLED_SetValueDoubleScalarUnregisteredPluginReturnsError` | `KnownDefectsDeath.SetValueDoubleScalarUnregisteredPluginCurrentlyCrashes` |
| plugin setvalue (generic) unregistered-name crash | `KnownDefects.DISABLED_SetValueGenericUnregisteredPluginReturnsError` | `KnownDefectsDeath.SetValueGenericUnregisteredPluginCurrentlyCrashes` |
| unregister leaves a never-bound plugin registered | `PluginTest.DISABLED_UnregisterNeverBoundPluginDestroysIt` | `PluginTest.UnregisterNeverBoundPluginLeavesItRegistered_CurrentBehavior` |
| `register` swallows the dlopen failure (NDEBUG-stripped assert) | `PluginTest.DISABLED_RegisterUnloadableSharedLibReportsDlopenFailure` | `PluginTest.RegisterUnloadableSharedLibSwallowsAssert_CurrentBehavior` |
| ASCII `CREATE_PULSE` overwrites instead of refusing | `ModeKnownDefects.DISABLED_AsciiCreatePulseFailsWhenAlreadyExists` | `ModeKnownDefects.AsciiCreatePulseCurrentlyOverwritesSilently` |
| `ERASE_PULSE` == `CLOSE_PULSE` on HDF5 | `ErasePulseKnownDefects.DISABLED_Hdf5EraseMakesPulseUnopenable` | `ErasePulseKnownDefects.Hdf5EraseCurrentlyLeavesPulseOpenable` |
| `ERASE_PULSE` == `CLOSE_PULSE` on ASCII | `…DISABLED_AsciiEraseMakesPulseUnopenable` | `ErasePulseKnownDefects.AsciiEraseCurrentlyLeavesPulseOpenable` |
| `ERASE_PULSE` == `CLOSE_PULSE` on Memory | `…DISABLED_MemoryEraseMakesPulseUnopenable` | `ErasePulseKnownDefects.MemoryEraseCurrentlyLeavesPulseOpenable` |
| `al_get_backendID` accepts a non-pulse context via unchecked `static_cast` | `GetBackendIdKnownDefects.DISABLED_WrongContextTypeReturnsError` | `GetBackendIdKnownDefects.WrongContextTypeCurrentlySucceedsViaUnsafeCast` |
| ASCII AOS read always reports size 0 | `AosKnownDefects.DISABLED_AsciiAosReadReportsWrittenSize` | `AosKnownDefects.AsciiAosReadCurrentlyReportsZero` |
| HDF5 leaf/structure delete wipes the whole occurrence | `DeleteKnownDefects.DISABLED_Hdf5LeafDeleteLeavesSiblingIntact` | `DeleteKnownDefects.Hdf5LeafDeleteCurrentlyWipesWholeOccurrence` |
| Memory root delete is a no-op | `DeleteKnownDefects.DISABLED_MemoryRootDeleteClearsWholeIds` | `DeleteKnownDefects.MemoryRootDeleteCurrentlyDoesNothing` |
| ASCII delete is a no-op at every granularity | `DeleteKnownDefects.DISABLED_AsciiDeleteRemovesLeaf` | `DeleteKnownDefects.AsciiDeleteIsCurrentlyANoOp` |

---

# Part 4 — MDSplus backend (compile-guarded tier)  (issue #12)

**Progress summary (issue #18): zero `gap` rows remain in Part 4 — every one
of its 18 rows is terminal (12 `covered`/`covered (indirect)`, 3
`divergence`, 1 `xfail`, 2 `terminal-gap`).** The one capability with no
MDSplus row at all as of issue #17 — `al_delete_data` — is closed here as an
explicit `terminal-gap` row (see the `DeleteMatrix` row below) rather than
left silently absent; every other row already carried a terminal verdict.
The corresponding shared always-on rows in Part 1/Part 2 above now each
carry a one-line "MDSplus: see Part 4" note.

**Progress: the MDSplus-unique surface landed (issue #17), closing Part 4's
last deferred row, on top of the version-drift check (issue #16), the
real-DD-path datatype × rank breadth (issue #15), the parity sweep (issue
#14), and the tracer bullet (issue #13).** MDSplus is a parameter/case in all
five fixtures issue #14 named — `DataEntryModes`, `CapabilityMatrix`,
`Occurrences`, `AosMatrix`, `EquilibriumSeedMatrix` — with every resulting row
triaged to a terminal status (`covered` or `divergence`; no genuine defect
surfaced, so no `xfail` this round). Issue #15 adds a dedicated real-DD-path
breadth matrix (`test_mdsplus_real_paths.cpp`, `Mdsplus/MdsplusRealPathMatrix.*`)
covering every `{CHAR, INTEGER, DOUBLE, COMPLEX} x rank {0..7}` cell the DD
actually contains — the one parity area `RoundTripMatrix` structurally cannot
join MDSplus for (its opaque synthetic paths have no model-tree node). Issue
#16 adds the version-drift check (`test_mdsplus_version_drift.cpp`,
`MdsplusVersionDrift.*`), pinning both the matching-version and
mismatched-version cases through the public C ABI. Issue #17 adds the
MDSplus-unique surface (`test_mdsplus_unique_surface.cpp`) — struct-aware
slice interpolation (`covered`), timerange resampling (`covered`, but with no
unique surface — see below), segment-backed dynamic writes (`covered`, plus
one genuine `xfail` crash defect it surfaced), and the timebase cache
(`covered`) — see its own subsection below for details.

**Finding (issue #16): this closes Part 2 row B's "gap" for MDSplus, but not
for HDF5.** Part 2 row B left the version-drift check a `gap` because forcing
the mismatch needs writing a different backend-version value into the opened
pulse after creation — for HDF5 that means poking its on-disk backend-version
attribute directly via the HDF5 C API, which that suite has no include/link
wiring for (out of scope there). MDSplus hits the identical shape of problem —
the mismatch needs overwriting the pulse's stored `VERSION:BACK_MAJOR` tree
node after creation, also unreachable through the C ABI — but the MDSplus tier
is already a separate, compile-guarded, Docker-characterized target
(`tests/contract/CMakeLists.txt`'s `AL_BACKEND_MDSPLUS` gate), so adding the
raw MDSplus C++ API (`<mdsobjects.h>`) as a test-only, build-gated dependency
of `contract_tests` was a small, low-blast-radius addition — unlike wiring the
HDF5 C API into the always-on suite. So the gap is closed for MDSplus
specifically because of that pre-existing gate, not because the underlying
problem (a version-drift mismatch cannot be forced through the C ABI alone) is
actually different between the two backends. **Part 2 row B itself is left
unchanged** (HDF5 remains a terminal gap); this is a Part 4 finding about *why*
the two backends diverged, not a fix to Part 2.

Build-gated by `AL_CONTRACT_HAVE_MDSPLUS` (`tests/contract/CMakeLists.txt`,
defined only when `AL_BACKEND_MDSPLUS=ON`), so every MDSplus case in
`test_mdsplus.cpp`, `test_mdsplus_real_paths.cpp`, `test_mdsplus_version_drift.cpp`,
`test_mdsplus_unique_surface.cpp`, `test_pulse_lifecycle.cpp`,
`test_structured_data.cpp`, and `test_capability_gated.cpp` compiles out
entirely otherwise. Runtime-skipped (`GTEST_SKIP()`, never failed, per D4)
when `MDSPLUS_MODELS_PATH` is unset. CTest label `mdsplus` (`ctest -L mdsplus`)
covers the whole parity set plus the breadth matrix, the version-drift check,
and the unique surface (issue #16 extended the label's `TEST_FILTER`
carve-out in `CMakeLists.txt` with `MdsplusVersionDrift*`, and added a
test-only `find_package(MDSplus)` + include/link wiring onto `contract_tests`,
gated behind the same `AL_BACKEND_MDSPLUS` option; issue #17 extends the same
carve-out with `MdsplusStructAwareSliceInterpolation*`,
`MdsplusTimerangeResampling*`, `MdsplusSegmentBackedWrites*`,
`MdsplusSliceWriteKnownDefects*`, `MdsplusSliceWriteKnownDefectsDeath*`, and
`MdsplusTimebaseCache*` -- no new build plumbing needed, since every case in
the file, including its crash-class death test, goes through the public C
ABI only and adds no raw MDSplus C++ API dependency).
Characterization environment: `docker/mdsplus/` (aarch64, DD 4.1.1 baked model
tree — see its README for the characterization-discovered deviation from the
issue's Ubuntu 22.04 starting point).

## Characterization-discovered facts (issue #14)

Resolved empirically against the real baked DD-4.1.1 model tree (issue #12's
explicit open items), not assumed:

- **Occurrence-naming convention**: a *third* convention, distinct from HDF5's
  `<ids>_<N>` and ASCII's `<ids>/<N>` — except it turns out to be the exact
  same slash form as ASCII, `<ids>/<N>`. `mdsconvertPath`'s `SEPARATORS`
  (`src/mdsplus/mdsplus_backend.cpp:25`) tokenizes on `/`, so
  `"equilibrium/2"` resolves to the real, pre-baked structural child node the
  model tree already contains for occurrence 2 — occurrence slots are static
  model structure, never dynamically created. Confirmed:
  `Occurrences.MdsplusListsWrittenOccurrences` writes occurrence 0 and
  occurrence 2 (as `"equilibrium/2"`) and gets both back from
  `al_get_occurrences`.
- **`get_occurrences` is real** (`src/mdsplus/mdsplus_backend.cpp:4887-4954`):
  it walks the IDS node's numeric-named children and reports an occurrence as
  "filled" iff its `ids_properties/homogeneous_time` has nonzero length — so a
  write that sets only a data field, without `homogeneous_time`, is invisible
  to `get_occurrences` even though the data is genuinely there.
- **`list_filled_paths` is NOT real**: it throws unconditionally
  (`src/mdsplus/mdsplus_backend.cpp:4956-4958`, "not implemented in the
  MDSplus Backend") — `BACKEND_ERR`, matching every other non-HDF5 always-on
  backend. Confirmed via `CapabilityMatrix.ListFilledPathsPositiveOrRefused/Mdsplus`.
- **`supportsTimeDataInterpolation()` is unconditionally `true`**
  (`src/mdsplus/mdsplus_backend.h:255-257`) and slice read/write genuinely
  round-trip (confirmed: a `CLOSEST_INTERP` slice read returns the exact
  written value). **`supportsTimeRangeOperation()` is unconditionally `false`**
  (`src/mdsplus/mdsplus_backend.cpp:5444-5446`) → `LOWLEVEL_ERR`, matching
  Memory/ASCII/Flexbuffers.
- **MDSplus requires real, DD-conformant paths for anything beyond a plain
  scalar** (issue #12 Q2's premise, now with concrete evidence): a synthetic
  field name with no corresponding model-tree node throws `%TREE-W-NNF, Node
  Not Found` — immediately for a plain (non-timed) `al_write_data`, but
  **deferred to `al_end_action`** for a timed write or an AOS flush (the value
  is buffered in an in-memory `MDSplus::Apd`/segment first). This is why
  `AosMatrix`'s and `EquilibriumSeedMatrix`'s synthetic-path bodies fail
  MDSplus at flush time even though their early write calls report success —
  see the divergence rows below. A real DD-conformant AOS
  (`equilibrium/time_slice/global_quantities/ip`, confirmed separately during
  characterization) round-trips through the identical begin/write/iterate/end
  sequence without error, so this is specific to non-conformant paths, not a
  general MDSplus AOS limitation.

| Cluster / Capability | Test(s) | Status |
|---|---|---|
| Tracer bullet: one equilibrium scalar (`vacuum_toroidal_field/r0`, reusing `equilibrium_seed.h`) write→read self-consistency round trip through the C ABI against a real MDSplus tree | `MdsplusTracerBullet.ScalarSurvivesWriteThenRead` | covered |
| Parity: `DataEntryModes` mode matrix (`OPEN_PULSE` absent/present, `FORCE_OPEN_PULSE`, `CREATE_PULSE` guard) — `openPulse`'s mode switch guards `CREATE_PULSE` against an existing `ids_001.tree` exactly like HDF5/Memory (`src/mdsplus/mdsplus_backend.cpp:4507-4510`); no ASCII-style defect | `Backends/DataEntryModes.*/Mdsplus` (4 cases) | covered |
| Parity: `CapabilityMatrix` — slice read/write positive (real interpolation round trip), timerange refused (`LOWLEVEL_ERR`), `list_filled_paths` refused (`BACKEND_ERR`) | `Backends/CapabilityMatrix.{TimeRangeReadPositiveOrRefused,SliceReadPositiveOrRefused,SliceWriteBeginPositiveOrRefused,ListFilledPathsPositiveOrRefused}/Mdsplus` (4 cases) | covered |
| Parity: `Occurrences` — real implementation, `<ids>/<N>` naming convention (see characterization facts above) | `Occurrences.MdsplusListsWrittenOccurrences` | covered |
| Parity: `AosMatrix` (top-level + nested write→iterate→read) — **divergence, not a defect**: MDSplus resolves every path against its real DD-baked model tree, so this fixture's synthetic field names (`"elements"`/`"val"`, `"outer"`/`"inner"`) have no corresponding node; `al_begin_arraystruct_action` and the per-element writes report success (buffered), but the flush at `al_end_action` throws `%TREE-W-NNF, Node Not Found`. Real DD-conformant AOS paths round-trip through the identical sequence (see characterization facts above) — the fixture's synthetic-path design simply cannot transfer to a model-tree-backed backend, the same reason `RoundTripMatrix` already excludes MDSplus (issue #12 Q5) | `Backends/AosMatrix.{TopLevelWriteIterateRead,NestedWriteIterateRead}/Mdsplus` (both `GTEST_SKIP()`, `AosExpect::Divergence`) | divergence |
| Parity: `EquilibriumSeedMatrix` (composite scalar + timebase-carrying 2-D array + `constraints` AOS, hash oracle) — **divergence, not a defect**: the scalar sub-shape round-trips (already proven by the tracer bullet above), but the seed's flat-tensorized `profiles_1d/psi` write and its generic `constraints`/`{measured,weight}` AOS shape don't match the real equilibrium DD layout MDSplus enforces (real `profiles_1d` is a genuine dynamic AOS, not a flat dataset at that path; real `constraints` is a fixed container of specific constraint sub-objects, not a generic AOS) — both throw `%TREE-W-NNF, Node Not Found`. MDSplus **is** instantiated in `kSeedBackends[]` (mirroring `AosMatrix` above), and the case is skipped in-place via `GTEST_SKIP()` rather than run and fail | `Backends/EquilibriumSeedMatrix.RoundTripHashMatches/Mdsplus` (`GTEST_SKIP()`) | divergence |
| Parity: `DeleteMatrix` (leaf / structure / DATAOBJECT-root granularity) — **terminal gap, closed here as an explicit row rather than left silently absent (issue #18)**: MDSplus is not instantiated in `kDeleteBackends[]` at all. Joining it would hit the same wall as `AosMatrix`/`RoundTripMatrix` above — the fixture's synthetic paths (`leaf_a`, `block`, top-level `""`) have no corresponding model-tree node, so even the divergence-and-skip pattern used for `AosMatrix`/`EquilibriumSeedMatrix` doesn't cleanly apply (unlike those, `MDSplusBackend::deleteData` — `mdsplus_backend.cpp:2136` — genuinely implements path-scoped leaf/structure delete, unlike HDF5's ignore-`path` defect, so a real characterization would be informative, not a structural no-op). A dedicated real-DD-path delete test mirroring `MdsplusRealPathMatrix` (issue #15) was not built in this closing pass — left as a deliberate, explained terminal gap for a future step, not a defect and not a silent omission | none — see reasoning | terminal-gap |

### Real-DD-path datatype × rank breadth (issue #15)

Where the rows above join MDSplus to the *existing* fixtures' synthetic
shapes, this matrix instead curates one **real** DD-4.1.1 path per
`{CHAR, INTEGER, DOUBLE, COMPLEX} x rank {0..7}` cell — the axis
`RoundTripMatrix` already sweeps on the always-on tier, but against opaque
paths MDSplus cannot resolve (issue #12 Q2/Q5). Path curation method: the
`imas-dd` MCP tool CLAUDE.md names was not present in this session's MCP
configuration, so the same question it would answer was resolved by walking
the actual baked-from artifact directly —
`build-mdsplus/_deps/data-dictionary-src/IDSDef.xml` (the DD-4.1.1 source
`ALBuildDataDictionary.cmake` downloads and the model tree is compiled from)
— which is still empirical characterization against the real shipped
artifact (D2), just queried by hand instead of through the MCP wrapper. The
18 covered cells collectively span 10 distinct real IDSs (`equilibrium`,
`b_field_non_axisymmetric`, `amns_data`, `magnetics`, `temporary`,
`balance_of_plant`, `bolometer`, `gyrokinetics_local`, `waves`,
`runaway_electrons` — some, like `gyrokinetics_local`, backing more than one
cell), confirming the model tree's whole-DD bake, not one container.

**Characterization-discovered facts:**

- **Five (type, rank) cells exist in the DD only inside a struct_array** —
  INTEGER rank 3, DOUBLE rank 6, and COMPLEX ranks 1/3/5. Each is reachable
  through the identical `al_begin_arraystruct_action` idiom `AosMatrix`
  already proved round-trips against MDSplus for real DD-conformant paths (a
  single element is written, no breadth beyond one element — AOS breadth
  itself stays `AosMatrix`/#14's job): INTEGER r3 and DOUBLE r6 use the DD's
  own generic self-test IDS, `temporary` (`constant_integer3d/value`,
  `constant_float6d/value`, one AOS level); COMPLEX r3/r5 use
  `runaway_electrons` (`distribution/markers/orbit_integrals[_instant]/values`,
  one AOS level, since `distribution` itself is a plain structure, not an
  AOS); COMPLEX r1 is the deepest real case found anywhere in the whole DD —
  `waves`' `coherent_wave/full_wave/e_field/plus/values` needs **three**
  nested AOS levels (`coherent_wave`, `full_wave`, `e_field/plus`) — and
  round-trips through the same idiom generalized to three descents. All five
  passed as plain round trips; no genuine defect surfaced from the AOS
  descent itself.
- **CHAR's rank axis is one off from the DD's own `STR_ND` suffix**: the
  C-ABI's `dim` parameter for CHAR_DATA models a *string* at dim=1 (DD
  `STR_0D`) and an *array of strings* at dim=2 (DD `STR_1D`) — dim=0 (a bare
  scalar char) has no DD analogue at all. Confirmed empirically by CHAR r0's
  divergence below.
- **CHAR r0 against a real STR_0D node is a divergence, not a crash**:
  writing a dim=0 (bare scalar char) value to `equilibrium`'s `code/name`
  (a real STR_0D node) and reading it back at dim=0 fails with "Wrong
  dimension of Data returned by backend: expected CHAR_DATA in 0D but got
  CHAR_DATA in 1D" — MDSplus's real text node is inherently string-shaped
  (1-D), so it correctly refuses the mismatched rank instead of silently
  misreading it. This contrasts with HDF5's genuine CHAR-scalar crash
  (`RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips`): MDSplus's
  behavior is a clean, typed refusal against a real DD node, and RoundTripMatrix's
  synthetic "dim=0 CHAR" shape simply has no DD counterpart to transfer to —
  the same divergence pattern as `AosMatrix`/`EquilibriumSeedMatrix` above,
  just surfacing on a single scalar cell instead of a whole fixture.
- **Thirteen (type, rank) cells are terminal-gap — the DD contains no field
  of that type at that rank, at any nesting depth, anywhere in DD 4.1.1**:
  CHAR ranks 3–7 (only `STR_0D`/`STR_1D` exist), INTEGER ranks 4–7 (max is
  `INT_3D`), DOUBLE rank 7 (max is `FLT_6D`), COMPLEX rank 0 (no scalar
  complex type at all) and ranks 6–7 (max is `CPX_5D`). Each cell's own test
  instance records this via `GTEST_SKIP()` with the specific empirical
  reason, rather than being silently absent from the matrix.

| Cluster / Capability | Test(s) | Status |
|---|---|---|
| Real-path breadth: 13 (type, rank) cells with a plain leaf field (no AOS needed) round-trip against real DD-4.1.1 paths spanning `equilibrium`, `b_field_non_axisymmetric`, `amns_data`, `magnetics`, `balance_of_plant`, `bolometer`, `gyrokinetics_local` — CHAR r1/r2, INTEGER r0–r2, DOUBLE r0–r5, COMPLEX r2/r4 | `Mdsplus/MdsplusRealPathMatrix.RealPathRoundTrip/{CHAR_r1,CHAR_r2,INTEGER_r0,INTEGER_r1,INTEGER_r2,DOUBLE_r0..DOUBLE_r5,COMPLEX_r2,COMPLEX_r4}` (13 cases; the fixture curates 5 more AOS-nested cells separately below, 18 covered in total) | covered |
| Real-path breadth: 5 (type, rank) cells reachable only inside a struct_array — INTEGER r3, DOUBLE r6 (`temporary`, 1 AOS level), COMPLEX r3/r5 (`runaway_electrons`, 1 AOS level), COMPLEX r1 (`waves`, 3 nested AOS levels, the deepest real case in the whole DD) — round-trip via the same `al_begin_arraystruct_action` idiom `AosMatrix` already proved against MDSplus | `Mdsplus/MdsplusRealPathMatrix.RealPathRoundTrip/{INTEGER_r3,DOUBLE_r6,COMPLEX_r1,COMPLEX_r3,COMPLEX_r5}` | covered |
| ↳ CHAR r0 (a bare scalar char) against a real STR_0D node (`equilibrium`'s `code/name`) — **divergence, not a defect**: MDSplus's real text node is inherently string-shaped (1-D) and correctly refuses a dim=0 read-back ("expected CHAR_DATA in 0D but got CHAR_DATA in 1D") instead of silently misreading it; contrast HDF5's genuine crash on the same synthetic shape (see characterization facts above) | `Mdsplus/MdsplusRealPathMatrix.RealPathRoundTrip/CHAR_r0` (`GTEST_SKIP()`) | divergence |
| ↳ 13 (type, rank) cells the DD contains nowhere at any nesting depth: CHAR r3–r7, INTEGER r4–r7, DOUBLE r7, COMPLEX r0, COMPLEX r6–r7 (see characterization facts above for the per-type ceiling each hits) | `Mdsplus/MdsplusRealPathMatrix.RealPathRoundTrip/{CHAR_r3..CHAR_r7,INTEGER_r4..INTEGER_r7,DOUBLE_r7,COMPLEX_r0,COMPLEX_r6,COMPLEX_r7}` (13 cases, each `GTEST_SKIP()` with its specific reason) | terminal-gap |
| Unique: struct-aware slice interpolation, timerange resampling, segments, timebase cache | see the "MDSplus-unique surface" subsection below | covered / xfail (issue #17) |

### MDSplus-unique surface (issue #17)

`test_mdsplus_unique_surface.cpp` — behavior with no HDF5-tier analogue,
because it depends on MDSplus's real segment-based time-series storage, not
just its real-DD-path model tree. Real path used throughout:
`equilibrium/time_slice` (a `timebasepath="time"` struct_array — each element
carries its own `time` leaf, matching DD 4.1.1's own `coordinate1
"time_slice(itime)/time"`) and its `global_quantities/ip` leaf, the same path
named as the round-tripping proof in the `AosMatrix`/`EquilibriumSeedMatrix`
divergence notes above and in `test_mdsplus_real_paths.cpp`'s header.

- **Struct-aware slice interpolation**: `al_begin_arraystruct_action` on a
  SLICE_OP context reaches `MDSplusBackend::readSliceApd`
  (`mdsplus_backend.cpp:3359`) instead of the plain-leaf `readSlice`; for
  `LINEAR_INTERP` it calls `interpolateStruct` (`mdsplus_backend.cpp:3675`),
  which interpolates every numeric leaf of the struct, not just one —
  confirmed by writing two leaves (`time`, `global_quantities/ip`) inside the
  same AOS element and observing both come back correctly interpolated from
  one call. HDF5's slice mode has no such capability (it only ever
  interpolates a single scalar leaf).
- **Timerange resampling**: no unique MDSplus surface exists.
  `supportsTimeRangeOperation()` is unconditionally `false`
  (`mdsplus_backend.cpp:5444-5446`), so `al_lowlevel.cpp`'s capability gate
  refuses `al_begin_timerange_action` with `LOWLEVEL_ERR` before the backend
  is reached at all — already pinned as parity by
  `CapabilityMatrix.TimeRangeReadPositiveOrRefused/Mdsplus`
  (`test_capability_gated.cpp`). This file re-asserts the same refusal
  directly against its own real DD path so the finding is self-contained.
- **Segment-backed dynamic writes**: `MDSplusBackend::beginWriteArraystruct`
  dispatches to `writeDynamicApd` (`mdsplus_backend.cpp:2688`) with
  `append=true` specifically for a SLICE_OP write against a timebase-carrying
  AOS (`mdsplus_backend.cpp:5257,5264,5317,5324`) — genuinely appending one
  more element's serialized `MDSplus::Apd` as a new segment, rather than the
  whole-container rewrite a GLOBAL_OP write does
  (`mdsplus_backend.cpp:5243,5245,5292,5295`). Confirmed by appending five
  elements one WRITE session at a time and reading every one back
  independently — in contrast to HDF5's own scalar slice-append defect
  (`Hdf5SliceAppend.OnlyLastAppendedSlicePersists_CurrentBehavior`,
  `test_capability_gated.cpp`), MDSplus keeps all of them. One caveat found
  empirically: the very first element of a brand-new AOS must still be
  established via an ordinary GLOBAL write — appending it via SLICE_OP hits
  the same first-segment fragility as the crash defect below (writeDynamicApd
  can't create a segment from nothing under `append=true`); once the AOS has
  its first segment, further SLICE_OP appends succeed. A narrower shape — a
  bare top-level scalar leaf slice-appended against a *separate* external
  timebase field, mirroring `Hdf5SliceAppend`'s own idiom verbatim — was tried
  first and empirically throws `%TREE-E-NOSEGMENTS` the first time either
  field is written with no pre-existing segment in the other; dropped in
  favor of the AOS-append form, which is the one this codebase's write
  dispatch already treats as the deliberate, first-class append path. Whether
  the scalar-leaf shape is a distinct, fixable defect is left unresolved —
  out of scope for this pass.
- **New defect found (xfail)**: `MDSplusBackend::writeData`'s SLICE_OP branch
  (`mdsplus_backend.cpp:4586-4591`) dereferences `size[0]` unconditionally —
  `if(size[0] > 1) writeTimedData(...); else writeSlice(...);` — but the C ABI
  passes `size == nullptr` for any dim=0 (scalar) write. Writing **any**
  scalar field inside a SLICE_OP write action therefore segfaults MDSplus,
  unconditionally — discovered when this file's first draft tried the exact
  idiom `Hdf5SliceAppend` uses to seed `homogeneous_time` inside its
  slice-write loop (fine on HDF5, crashes on MDSplus). Pinned per D2:
  `MdsplusSliceWriteKnownDefects.DISABLED_ScalarWriteWithinSliceActionSucceeds`
  (correct contract) +
  `MdsplusSliceWriteKnownDefectsDeath.ScalarWriteWithinSliceActionCurrentlyCrashes`
  (current-behavior death-test tripwire).
- **Timebase cache**: `MDSplusBackend` caches computed segment/timebase-index
  mappings per node (`segmentIdxMap`, `mdsplus_backend.h:283`), populated by
  `getSegmentIdxFromSliceIdx` (`mdsplus_backend.cpp:3221`) and reused by every
  subsequent `getApdSliceAt` call (e.g. both lookups a single `LINEAR_INTERP`
  struct-aware read needs). Every touch site shows it is invalidated
  unconditionally (a whole-map clear, not scoped to the written node) at the
  *start* of every write dispatch (`writeData`, `mdsplus_backend.cpp:4577`;
  `beginWriteArraystruct`, `mdsplus_backend.cpp:4634`) and again at the *end*
  of an AOS write (`mdsplus_backend.cpp:5342`) and at `deleteData`
  (`mdsplus_backend.cpp:2141`) — so no write path through the C ABI can leave
  a stale segment map for a later read. Confirmed by warming the cache with a
  slice-interpolated read, forcing an intervening write (an unrelated GLOBAL
  scalar write, then a SLICE_OP append growing the very AOS whose segment map
  was cached), and showing a repeated slice read afterwards is neither
  corrupted by the invalidation nor stale to the new layout.

| Cluster / Capability | Test(s) | Status |
|---|---|---|
| Struct-aware slice interpolation (CLOSEST/PREVIOUS/LINEAR, whole-struct) | `MdsplusStructAwareSliceInterpolation.{ClosestPicksNearerElement,PreviousPicksLastElementAtOrBelow,LinearInterpolatesWholeStruct}` | covered |
| Timerange resampling — no unique MDSplus surface, refused unconditionally | `MdsplusTimerangeResampling.RefusedWithNoUniqueSurface` | covered |
| Segment-backed dynamic writes — incremental AOS appends all persist | `MdsplusSegmentBackedWrites.AppendedElementsAllPersist` | covered |
| ↳ a dim=0 scalar `al_write_data` inside a SLICE_OP action segfaults MDSplus (`size[0]` dereferenced unconditionally against a `nullptr` size) | `MdsplusSliceWriteKnownDefects.DISABLED_ScalarWriteWithinSliceActionSucceeds` + tripwire `MdsplusSliceWriteKnownDefectsDeath.ScalarWriteWithinSliceActionCurrentlyCrashes` | **xfail** |
| Timebase/segment-index cache survives an intervening invalidating write without staleness or corruption | `MdsplusTimebaseCache.SurvivesInterveningWritesWithoutStaleness` | covered |

### Version-drift check (issue #16)

`al_begin_dataentry_action` (`src/al_lowlevel.cpp:868-883`) compares
`backend->getVersion(NULL)` (the compiled backend's own version) against
`backend->getVersion(pctx)` (the version stored in the pulse being opened) on
`OPEN_PULSE`/`FORCE_OPEN_PULSE` and throws `LOWLEVEL_ERR` on a mismatch. For
MDSplus, the stored value is `VERSION:BACK_MAJOR`/`BACK_MINOR`
(`src/mdsplus/mdsplus_backend.cpp`'s `saveVersion`, called once at pulse
creation from the compiled `MDSPLUS_BACKEND_MAJOR`/`MINOR` constants, currently
1/1). Forcing the mismatch case needs a value in the pulse different from
what today's build would write — done here by opening the pulse's "ids" tree
directly through the raw MDSplus C++ API (`<mdsobjects.h>`, not the C ABI) and
overwriting `VERSION:BACK_MAJOR` after creation, mirroring the
`ids_path`/`setDataEnv` environment-variable dance the backend itself uses
internally. See the finding above on why this was achievable for MDSplus but
stays a `gap` for HDF5 (Part 2 row B).

| Cluster / Capability | Test(s) | Status |
|---|---|---|
| Matching case: a pulse created and reopened by the same compiled backend carries the version it just wrote, so the drift check's comparison is a no-op and `OPEN_PULSE` succeeds | `MdsplusVersionDrift.MatchingVersionOpensCleanly` | covered |
| Mismatching case: `VERSION:BACK_MAJOR` is overwritten (via the raw MDSplus tree API) to a value the compiled backend can never match; reopening through the C ABI hits `al_lowlevel.cpp`'s `(ver.first != sver.first)` branch and fails with `LOWLEVEL_ERR` (message contains "Compatibility"), never a silent open | `MdsplusVersionDrift.MismatchedBackendVersionRefusesOpen` | covered |
