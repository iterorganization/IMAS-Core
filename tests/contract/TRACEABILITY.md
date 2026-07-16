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
| `al_begin_dataentry_action` opens a data entry from a URI | :90-107 | ✓ (Memory) | ✓ (HDF5, ASCII) | `FORCE_CREATE_PULSE` asserted OK in every suite. `OPEN_PULSE` fails when absent / `FORCE_OPEN_PULSE` creates when absent / `OPEN_PULSE` succeeds once present / `CREATE_PULSE` refuses an existing pulse (HDF5, Memory): `Backends/DataEntryModes.*`. MDSplus: joined too — see Part 4. UDA: all four modes now have the same lifecycle verdict (`UdaBreadthTest.{OpenPulse*,ForceOpenPulse*,CreatePulse*,ForceCreatePulse*}`) — see Part 5 | covered |
| ↳ ASCII's `CREATE_PULSE` has no existence guard — **silently overwrites/truncates an existing pulse** instead of refusing it | :90-107; src/ascii_backend.cpp:107-157 | ✓ | — | `ModeKnownDefects.DISABLED_AsciiCreatePulseFailsWhenAlreadyExists` + tripwire `ModeKnownDefects.AsciiCreatePulseCurrentlyOverwritesSilently` | **xfail** |
| `al_close_pulse` closes (`CLOSE_PULSE`) / erases (`ERASE_PULSE`) | :109-116 | ✓ (Memory) | ✓ (HDF5, ASCII) | `CLOSE_PULSE` asserted `code==0` in every suite. UDA: `CLOSE_PULSE` retains an openable remote pulse; read-bearing sessions separately leak a server-side handle through a URI mismatch — see Part 5 | covered (indirect) |
| ↳ `ERASE_PULSE` is not implemented by any always-on backend — **behaves identically to `CLOSE_PULSE`** (none of HDF5/ASCII/Memory's `closePulse` reference their `mode` parameter); a later plain `OPEN_PULSE` on the "erased" pulse still succeeds | :109-116; src/hdf5/{hdf5_reader,hdf5_writer}.cpp closePulse, src/ascii_backend.cpp:165-169, src/memory_backend.h:505-509 | ✓ (Memory) | ✓ (HDF5, ASCII) | `ErasePulseKnownDefects.DISABLED_{Hdf5,Ascii,Memory}EraseMakesPulseUnopenable` + tripwires `ErasePulseKnownDefects.{Hdf5,Ascii,Memory}EraseCurrentlyLeavesPulseOpenable`. UDA carries the same defect across the remote boundary: `UdaBreadthTest.DISABLED_ErasePulseMakesRemotePulseUnopenable` + tripwire `UdaBreadthTest.ErasePulseCurrentlyLeavesRemotePulseOpenable` (Part 5) | **xfail** |
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
| `al_begin_global_action` starts a GLOBAL read/write op | :159-175 | ✓ (Memory) | ✓ (HDF5, ASCII) | Vehicle for every round trip: `RoundTrip.*`, `RoundTripMatrix.*`, `VersionSentinel.*`, `CapabilityMatrix.*` (WRITE_OP + READ_OP). `datapath` partial-get is UDA-only (out of always-on scope). MDSplus joins `CapabilityMatrix` only, not `RoundTripMatrix`/`VersionSentinel` (opaque synthetic paths) — see Part 4. UDA: real-DD-path breadth matrix, cache-mode/`datapath` unique surface — see Part 5 | covered |
| `al_begin_slice_action` starts a time-slice op (READ interp; WRITE append via `UNDEFINED_TIME`) | :177-194 | ✓ (Memory) | ✓ (HDF5) | HDF5·Memory positive / ASCII·Flexbuffers refused `BACKEND_ERR`: `CapabilityMatrix.SliceReadPositiveOrRefused`, `CapabilityMatrix.SliceWriteBeginPositiveOrRefused`. Interp modes + missing-interp refusal (`CONTEXT_ERR`): `Hdf5TimeDependent.SliceInterpolationModes`, `Hdf5TimeDependent.SliceReadWithoutInterpModeIsRejected`. MDSplus: see Part 4 (real interpolation round trip, plus struct-aware slice interpolation as a unique surface). UDA: `CLOSEST_INTERP` on a real DD dynamic leaf round-trips correctly in remote mode — see Part 5 | covered |
| ↳ WRITE append via `UNDEFINED_TIME` must accumulate — **only the last slice persists in this build** | :177-194; al_lowlevel.h:300-316 | — | ✓ | `Hdf5SliceAppend.DISABLED_AppendedSlicesAllPersist` + tripwire `Hdf5SliceAppend.OnlyLastAppendedSlicePersists_CurrentBehavior` | **xfail** |
| `al_begin_timerange_action` starts a time-range op (with/without resampling `dtime`) | :196-221 | ✓ (Memory) | ✓ (HDF5) | HDF5 positive (no-resample + LINEAR resample): `Hdf5TimeDependent.TimeRangeReadWithoutResampling`, `…WithResampling`; Memory·ASCII·Flexbuffers refused `LOWLEVEL_ERR`: `CapabilityMatrix.TimeRangeReadPositiveOrRefused`. MDSplus: also refused, unconditionally — see Part 4. UDA: capability genuinely granted (server 1.8.0 > 1.4.0) but the read then fails — `OperationContext`'s TIMERANGE_OP ctor never sets base `interpmode` — xfail, see Part 5 | covered |
| `al_begin_arraystruct_action` starts an AOS op (top-level + nested), write→iterate→read | :223-233 | ✓ (Memory) | ✓ (HDF5) | `Backends/AosMatrix.{TopLevelWriteIterateRead,NestedWriteIterateRead}/{HDF5,Memory}`; Flexbuffers refused (paired-negative, same read-refusal as the round-trip matrix): `…/Flexbuffers`. MDSplus: joined too, but a divergence (synthetic paths) — see Part 4. UDA: traversal mechanism (size + iteration) is covered, but a *dynamic* leaf nested inside a struct_array returns the HDF5 absent-scalar sentinel instead of what was written — xfail, see Part 5 | covered |
| ↳ ASCII's AOS **read** always reports size 0, for any AOS, regardless of what was written — `beginAction`'s `READ_OP` setup consumes the whole file into a random-access lookup map before the AOS-size lookup's sequential cursor ever runs | :223-233; src/ascii_backend.cpp:213-235,656-682 | ✓ | — | `AosKnownDefects.DISABLED_AsciiAosReadReportsWrittenSize` + tripwire `AosKnownDefects.AsciiAosReadCurrentlyReportsZero`; matrix cells skip via `Backends/AosMatrix.*/ASCII` | **xfail** |
| `al_end_action` ends any context | :235-242 | ✓ (Memory) | ✓ (HDF5, ASCII) | asserted OK in every suite | covered (indirect) |
| `al_write_data` → `al_read_data` round trip preserves value + shape (INTEGER/DOUBLE/COMPLEX scalar→7-D on HDF5·Memory·ASCII; CHAR scalar/1-D/2-D on HDF5·ASCII, all ranks on Memory) | :244-263 | ✓ (Memory) | ✓ (HDF5, ASCII) | `Backends/RoundTripMatrix.ReadEqualsWrite/*`; generator `al_contract.h` `synth_value`/`synth_buffer`/`shape_for_rank`. MDSplus excluded from this fixture (opaque synthetic paths) — see Part 4's real-DD-path breadth matrix instead. UDA likewise excluded, for the same reason — see Part 5's real-DD-path breadth matrix | covered |
| ↳ Flexbuffers must-refuse column (serializer, not a pulse store): write accepted, read refused, every datatype × shape | :244-263 / D4 | — | ✓ | `…/Flexbuffers_*_r{0..7}` (paired-negative) | covered |
| ↳ CHAR > 2-D refused (documented "not implemented") on HDF5·ASCII | :244-263 / D4 | — | ✓ | `…/{HDF5,ASCII}_CHAR_r{3..7}` (paired-negative) | covered |
| ↳ CHAR scalar (dim 0) must round-trip — **HDF5 crashes** | :244-263 | — | ✓ | `RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips` + tripwire `RoundTripKnownDefectsDeath.Hdf5CharScalarCurrentlyCrashes` | **xfail** |
| ↳ numeric at MAXDIM (rank 7) must round-trip — **ASCII corrupts (DOUBLE/COMPLEX) / aborts (INTEGER)** | :244-263 | — | ✓ | `RoundTripKnownDefects.DISABLED_Ascii{Integer,Double,Complex}MaxdimRoundTrips` + tripwires `RoundTripKnownDefectsDeath.AsciiIntegerMaxdimCurrentlyAborts`, `RoundTripKnownDefects.Ascii{Double,Complex}MaxdimCurrentlyCorrupts` | **xfail** |
| `al_delete_data` at signal / structure / DATAOBJECT-root granularity | :265-272 | ✓ (Memory) | ✓ (HDF5) | Support is disjoint per backend, none implement all three (see xfail rows below): `Backends/DeleteMatrix.{LeafDeleteRemovesJustTheLeaf,StructureDeleteRemovesWholeSubtree}/Memory` (leaf + structure genuinely work only on Memory), `…/RootDeleteRemovesWholeOccurrence/HDF5` (root genuinely works only on HDF5, which ignores `path` and always removes the whole occurrence). MDSplus: not joined to this fixture — terminal gap, see Part 4 (issue #18). UDA: remote delete is cleanly refused (the reference plugin has no `deleteData` handler); remote write over the same path falsely reports success instead of the same refusal — xfail, see Part 5 | covered |
| ↳ HDF5 ignores `path` for leaf/structure delete — **any delete call removes the whole occurrence**, not just the named field | :265-272; src/hdf5/hdf5_writer.cpp deleteData (no path parameter at all) | — | ✓ | One tripwire covers both defective granularities, since there is no leaf-vs-structure branch to differ (`HDF5Writer::deleteData` never receives a path at all): `DeleteKnownDefects.DISABLED_Hdf5LeafDeleteLeavesSiblingIntact` + tripwire `DeleteKnownDefects.Hdf5LeafDeleteCurrentlyWipesWholeOccurrence`; matrix cells skip via `Backends/DeleteMatrix.{Leaf,Structure}*/HDF5` | **xfail** |
| ↳ Memory has no code path for DATAOBJECT-root delete — **a root-granularity delete is silently a no-op** | :265-272; src/memory_backend.cpp deleteData (no root/occurrence case) | ✓ | — | `DeleteKnownDefects.DISABLED_MemoryRootDeleteClearsWholeIds` + tripwire `DeleteKnownDefects.MemoryRootDeleteCurrentlyDoesNothing`; matrix cell skips via `Backends/DeleteMatrix.RootDeleteRemovesWholeOccurrence/Memory` | **xfail** |
| ↳ ASCII's `deleteData` is a literal no-op (empty body) at every granularity; Flexbuffers' is too but is not independently ABI-observable (it already refuses every in-session read regardless of delete, so a read-based oracle can't distinguish "delete did nothing" from "this backend never round-trips reads") | :265-272; src/ascii_backend.cpp:648-652, src/flexbuffers_backend.cpp:387-389 | ✓ | ✓ | One tripwire covers every granularity on ASCII, since an empty function body has no path-dependent branch to differ: `DeleteKnownDefects.DISABLED_AsciiDeleteRemovesLeaf` + tripwire `DeleteKnownDefects.AsciiDeleteIsCurrentlyANoOp`; matrix cells skip via `Backends/DeleteMatrix.*/ASCII` and `…/Flexbuffers` | **xfail** |
| `al_iterate_over_arraystruct` advances the AOS cursor | :274-282 | ✓ (Memory) | ✓ (HDF5) | Exercised as an asserted dependency of every AOS write/read in `Backends/AosMatrix.*` (top-level and nested, multi-step) | covered (indirect) |
| `al_get_occurrences` lists non-empty occurrences; caller-frees `*occurrences_list` (verified `malloc`-based on both implementing backends) | :284-292 | ✓ (Memory) | ✓ (HDF5, ASCII) | HDF5 and ASCII implement it for real, each against its own occurrence-naming convention (neither transforms `dataobjectname`, so the caller supplies the backend-native form — HDF5: `"<ids>_<N>"`; ASCII: `"<ids>/<N>"`): `Occurrences.{Hdf5,Ascii}ListsWrittenOccurrences`. Memory/Flexbuffers refuse unconditionally (not implemented): `Occurrences.{Memory,Flexbuffers}Refuses`. MDSplus: also real, same `<ids>/<N>` convention — see Part 4 (`Occurrences.MdsplusListsWrittenOccurrences`). UDA: also real over remote mode, HDF5's `<ids>_<N>` convention — see Part 5 | covered |
| `al_list_filled_paths` lists filled leaf paths; **caller frees list + strings** | :294-314 | ✓ (Memory) | ✓ (HDF5) | HDF5 positive (leaves discoverable, list freed) / Memory·ASCII·Flexbuffers refused `BACKEND_ERR`: `CapabilityMatrix.ListFilledPathsPositiveOrRefused`. MDSplus: refused `BACKEND_ERR` too, same as Memory·ASCII·Flexbuffers — see Part 4. UDA: real over remote mode, restricted to `backend=hdf5` — see Part 5 | covered |

### Equilibrium seed (issue #4 — decision D5)

The deterministic, in-repo single-version equilibrium-seed generator (a
scalar, a timebase-carrying 2-D array, and a constraints AOS — no committed
binary blobs; oracle = a content hash recomputed from the generator functions
themselves, `tests/contract/equilibrium_seed.h`), round-tripped on the three
backends where AOS content is actually readable back within a session.

| Capability | Inventory ref | Unit | Integ. | Test(s) | Status |
|---|---|:--:|:--:|---|---|
| Realistic AOS/timebase/HDF5-tensorization shape round-trips exactly | D5 | ✓ (Memory) | ✓ (HDF5, ASCII) | `Backends/EquilibriumSeedMatrix.RoundTripHashMatches/{HDF5,Memory,ASCII}`. MDSplus is instantiated too, but a divergence (the seed's generic shapes don't match the real equilibrium DD layout) — see Part 4. UDA: same divergence, same root cause (the DD-schema-validating backend has no counterpart for the seed's synthetic shapes) — see Part 5 | covered |

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
| **A** `initBackend` factory selects the backend from the URI | :496-514 | ✓ (Memory) | ✓ (HDF5, ASCII) | every always-on backend instantiated via `build_uri` in `RoundTripMatrix`. The unrecognized-backend refusal has an active verdict (issue #47): `BackendFactory.*` (test_backend_factory.cpp) pins the exact C-ABI status for an unknown backend name in a URI (`UNKNOWN_ERR`, all four open modes, no context handed out, no filesystem artifact) and for an unknown positive legacy numeric ID via `al_build_uri_from_legacy_parameters` (`LOWLEVEL_ERR`, no URI synthesized). MDSplus is instantiated too, via its own URI in every Part 4 fixture — see Part 4 | covered |
| **B** `getVersion` (installed vs stored, drift check) | :520-540 | — | ✓ (HDF5) | Tested (issue #36): `Hdf5VersionDrift.*` (test_hdf5_version_drift.cpp) — matching stored version opens; stored `999.0` (major) and `1.999` (minor boundary) are refused with the exact observed status. The mismatched fixture is produced out-of-band by the isolated `hdf5_fixture_tool` (rewrites master.h5's `HDF5_BACKEND_VERSION`); `contract_tests` itself stays HDF5-API-free and asserts only through the C ABI. Empirical nuance the pins record: for HDF5 the generic `LOWLEVEL_ERR` comparison (src/al_lowlevel.cpp) is **shadowed** — `HDF5BackendFactory` accepts exactly "1.0" and `openPulse` throws `BACKEND_ERR` ("No backend writer with version: …") for any other stored value, before the generic check runs; every non-compiled stored version is thereby refused (stricter than the generic rule). **MDSplus: also closed** — see Part 4's version-drift check (issue #16, reworked by #35 to keep the raw-MDSplus mutation in a fixture tool) | covered |
| **B** `openPulse` / `closePulse` | :542-552 | ✓ (Memory) | ✓ (HDF5, ASCII) | via `al_begin_dataentry_action` / `al_close_pulse` in all suites | covered (indirect) |
| **C** `beginAction` / `endAction` | :558-574 | ✓ (Memory) | ✓ (HDF5, ASCII) | via `al_begin_global_action` / `al_end_action` | covered (indirect) |
| **C** `writeData` / `readData` (0=not-found vs 1=success inner convention) | :576-589 | ✓ (Memory) | ✓ (HDF5, ASCII) | via `al_write_data` / `al_read_data` — `RoundTripMatrix` (+ the CHAR-scalar / ASCII-maxdim xfails above). MDSplus: see Part 4's real-DD-path breadth matrix (issue #15) instead of `RoundTripMatrix`. UDA: same — see Part 5's real-DD-path breadth matrix instead of `RoundTripMatrix` | covered |
| **C** `deleteData` | :591-599 | ✓ (Memory) | ✓ (HDF5) | via `al_delete_data` — see Cluster 2 row above and its three xfail rows. MDSplus: terminal gap — see Part 4 (issue #18). UDA: remote delete cleanly refused, remote write falsely reports success — xfail, see Part 5 | covered (indirect) |
| **C** `beginArraystructAction` | :601-606 | ✓ (Memory) | ✓ (HDF5) | via `al_begin_arraystruct_action` — see Cluster 2 row above. MDSplus: joined (divergence) plus its own unique struct-aware slice/segment-write surface — see Part 4. UDA: traversal mechanism covered, a dynamic leaf nested inside a struct_array returns the wrong value — xfail, see Part 5 | covered (indirect) |
| **D** `get_occurrences` | :612-616 | — | ✓ | via `al_get_occurrences` — `Occurrences.*`. MDSplus: see Part 4. UDA: see Part 5 | covered (indirect) |
| **D** `list_filled_paths` (per-backend: HDF5 real, others throw) | :618-640 | ✓ (Memory) | ✓ (HDF5) | via `al_list_filled_paths` — `CapabilityMatrix.ListFilledPathsPositiveOrRefused` pins both the HDF5 impl and the unconditional throw on Memory·ASCII·Flexbuffers. MDSplus: throws unconditionally too, same as Memory·ASCII·Flexbuffers — see Part 4. UDA: real over remote mode, restricted to `backend=hdf5` — see Part 5 | covered |
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
attribute directly via the HDF5 C API, which that suite originally had no
include/link wiring for. MDSplus hits the identical shape of problem — the
mismatch needs overwriting the pulse's stored `VERSION:BACK_MAJOR` tree node
after creation, also unreachable through the C ABI. Both are now solved with
the same producer/observer split: all raw-API fixture mutation lives in
standalone, build-gated setup tools (`mdsplus_fixture_tool` for the tree
node, issue #35; `hdf5_fixture_tool` for the HDF5 attribute, issue #36) that
the tests invoke as subprocesses, so `contract_tests` itself carries no
MDSplus or HDF5 API dependency and every asserted behavior stays on the
public C ABI. **Part 2 row B is closed for HDF5 too** (see Part 2).

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
  general MDSplus AOS Limit.

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
paths MDSplus cannot resolve (issue #12 Q2/Q5). The curated set is the shared
`al_contract::real_dd::catalog()` (`real_dd_path_catalog.{h,cpp}`, issue #46):
the IDS, struct_array chain, leaf, and terminal-gap DD facts are identical for
MDSplus and UDA and live in exactly one place, so the two matrices cannot
silently diverge; only each backend's own storage-model verdicts stay local
(`RealDdPathCatalog.*` plus the per-backend `*RealPathCatalog.CoversEveryCatalogKeyOnce`
meta-tests fail if a matrix drops a cell). Path curation method: the
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
what today's build would write — done by the standalone, build-gated
`mdsplus_fixture_tool` (issue #35), which opens the pulse's "ids" tree
through the raw MDSplus C++ API (`<mdsobjects.h>`) and overwrites
`VERSION:BACK_MAJOR` after creation, mirroring the `ids_path`/`setDataEnv`
environment-variable dance the backend itself uses internally. The test
invokes the tool as a subprocess and asserts exclusively through `al_*`
APIs: `contract_tests` has no direct MDSplus include or link dependency, so
a rewrite satisfying the C ABI is judged by exactly the C ABI. The identical
split now also closes the HDF5 side (Part 2 row B, issue #36).

| Cluster / Capability | Test(s) | Status |
|---|---|---|
| Matching case: a pulse created and reopened by the same compiled backend carries the version it just wrote, so the drift check's comparison is a no-op and `OPEN_PULSE` succeeds | `MdsplusVersionDrift.MatchingVersionOpensCleanly` | covered |
| Mismatching case: `VERSION:BACK_MAJOR` is overwritten (via the raw MDSplus tree API) to a value the compiled backend can never match; reopening through the C ABI hits `al_lowlevel.cpp`'s `(ver.first != sver.first)` branch and fails with `LOWLEVEL_ERR` (message contains "Compatibility"), never a silent open | `MdsplusVersionDrift.MismatchedBackendVersionRefusesOpen` | covered |

---

# Part 5 — UDA backend (compile-guarded tier)  (PRD #21)

Same shape as Part 4: a compile-guarded characterization tier for a backend the
core has no compile-time coupling to, exercised entirely through the public C
ABI (decision D1). Unlike MDSplus, UDA is a **remote client** to a reference
stack it does not embed (a real `uda_server` running the `IMAS` server plugin,
linked against a server-side IMAS-Core); the tier's tests build and drive that
stack from `docker/uda/`, never asserting on server internals — the subject
under test is always the IMAS-Core UDA client. And unlike MDSplus, UDA is
read-only in remote mode (the reference `IMAS` server plugin has no
`writeData`/`deleteData` handler — confirmed empirically, see
`docker/uda/README.md`), so parity arrives via a seed-then-reopen fixture
(seed through the plain HDF5 backend, reopen the identical path through UDA)
rather than a write→read round trip.

Build-gated by `AL_CONTRACT_HAVE_UDA` (`tests/contract/CMakeLists.txt`, defined
when `AL_BACKEND_UDA` or `AL_BACKEND_UDAFAT` is `ON`), so every UDA case in
`test_uda.cpp` compiles out entirely otherwise. Runtime-skipped
(`GTEST_SKIP()`, never failed, per D4) when `UDA_HOST` is unset — an ordinary
local build or an always-on CI leg without the reference stack standing behind
it never sees these tests fail. CTest label `uda` (`ctest -L uda`) selects the
whole tier.

## Pinned reference stack (issue #23's spike, `docker/uda/`)

| Component | Source | Pin |
|-----------|--------|-----|
| UDA server | [`ukaea/uda`](https://github.com/ukaea/uda) | tag **2.9.3** |
| `IMAS` UDA server plugin | [`iterorganization/UDA-Plugins`](https://github.com/iterorganization/UDA-Plugins) | tag **1.8.0**, commit **`ede25b921081d8fc2d66c5b5ca152c664b50ee78`** |
| Data Dictionary | `imas-data-dictionary` (PyPI wheel) | **4.1.1** |
| Base image | `ubuntu:24.04` | arm64 |

Gate decision (issue #23): **FULL-STACK TIER CONFIRMED** — the `IMAS` server
plugin is built and registered linked against the workspace's own IMAS-Core
(`docker/uda/run.sh` fails loudly on the mapping-only `NO_IMAS` degradation the
PRD forbids), not a stub. The fallback tier (client-side-only, remote rows
blocked-by-environment) was not invoked.

## Characterization-discovered facts (issue #24)

- **UDA validates every remote read path against the DD schema it loads at
  startup** (`$IDSDEF_PATH`/`$IMAS_PREFIX/include/IDSDef.xml`,
  `src/uda/uda_xml.cpp`), the same shape of constraint MDSplus enforces via its
  baked model tree (`CLAUDE.md`, TRACEABILITY.md Part 4) — but arrived at
  differently: MDSplus resolves against a compiled binary tree, UDA against the
  DD XML text at runtime. A real DD-conformant path (`vacuum_toroidal_field/r0`)
  round-trips through the seed-then-reopen fixture; a synthetic/generic one does
  not — see the divergence row below.
- **`ids_properties/homogeneous_time` is a hard precondition for any remote
  read** (`UDABackend::readData`'s `cache_mode=none` path,
  `src/uda/uda_backend.cpp:646-665`, calls `get_homogeneous_flag` before
  resolving any other field and throws if it is absent) — both fixtures below
  seed it explicitly ahead of the field(s) under test, a fixture-setup detail,
  not part of either test's assertion.

| Capability | Test(s) | Status |
|---|---|---|
| Tracer bullet (issue #23): one equilibrium scalar (`vacuum_toroidal_field/r0`), seeded through the plain HDF5 backend, reads back byte-identical through the UDA backend in remote mode, across the real reference stack | `UdaSmokeRoundTrip.ScalarSeededViaHdf5ReadsBackThroughUda` | covered |
| Read-only parity fixture (issue #24): the full equilibrium-seed composite (scalar + timebase-carrying 2-D array + constraints AOS, `equilibrium_seed.h`, issue #4/D5), seeded through HDF5 and reopened through UDA, asserted via the seed's own unmodified FNV-1a hash oracle — **divergence, not a defect**: the generator's flat `profiles_1d/psi` leaf and generic `constraints` AOS have no counterpart in equilibrium's real DD-4.1.1 layout (`profiles_1d` is itself a struct_array, not a plain field). Confirmed empirically against the reference stack: `al_plugin_read_data` fails immediately with "cannot find node equilibrium/profiles_1d/psi in data dictionary (profiles_1d not found)". The same wall MDSplus hits on its own `EquilibriumSeedMatrix`/`Mdsplus` row (Part 4) — nothing to fix, the fixture's synthetic composite shape simply cannot transfer to a DD-schema-validating backend; the scalar sub-shape alone is real DD and is already covered by the tracer bullet above. The seed-then-reopen attempt genuinely runs every time (unlike EquilibriumSeedMatrix's per-backend skip, this suite has no other instance to keep it exercised): the test asserts the *specific* failure signature before `GTEST_SKIP()`'ing, so a future change that resolves the divergence fails the assertion loudly instead of this row silently going stale | `UdaEquilibriumSeedParity.HdfSeededReadsBackThroughUda` | divergence |

**Progress (issue #25, parity breadth): zero blank rows remain in Part 5.**
Every C-ABI-reachable read-side capability from `FUNCTIONALITY_INVENTORY.md`
Part 1 now has a UDA verdict at a terminal status. `test_uda_real_paths.cpp`
adds the real-DD-path datatype × rank breadth matrix (issue #15's curated set,
reused as-is, adapted to the seed(HDF5)-then-reopen(UDA) shape); `test_uda_
breadth.cpp` adds AOS traversal, occurrences, `list_filled_paths`, pulse
open-mode/error-behavior parity, and slice/time-range reads. Two genuine new
defects were discovered and pinned (xfail, D2 correct-contract + tripwire),
not fixed — both explained in full below.

## Real-DD-path datatype × rank breadth (issue #25, reusing issue #15's set)

`test_uda_real_paths.cpp`, `Uda/UdaRealPathMatrix.*` — the UDA analogue of
`test_mdsplus_real_paths.cpp`: **the identical curated path/rank/aos_chain set,
now the shared `al_contract::real_dd::catalog()`** (`real_dd_path_catalog.{h,cpp}`,
issue #46 — one source of truth for both matrices, same 32 cells, same 10
IDSs), adapted to seed(HDF5)-then-reopen(UDA) instead of MDSplus's direct
write→read. `terminal_gap_reason` lives in the shared catalog (a DD-4.1.1 fact,
independent of backend); UDA-only `divergence_reason`/`known_defect_reason`
verdicts stay local to `test_uda_real_paths.cpp` where the two backends diverge
for different reasons on the same cell. `UdaRealPathCatalog.CoversEveryCatalogKeyOnce`
fails if the UDA matrix ever drops or duplicates a catalog cell.

**Characterization-discovered facts:**

- **13 terminal-gap cells carry over unchanged** from the MDSplus matrix (CHAR
  r3–r7, INTEGER r4–r7, DOUBLE r7, COMPLEX r0, COMPLEX r6–r7) — the DD itself
  contains no field of that (type, rank) at any nesting depth, independent of
  backend.
- **CHAR r0 is a terminal gap, blocked by fixture setup**: UDA's fixture never
  reaches the remote-backend question because seeding this cell requires
  writing a dim=0 CHAR value through the *plain HDF5 backend*. That reproduces
  the pre-existing, independently pinned
  `RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips` /
  `RoundTripKnownDefectsDeath.Hdf5CharScalarCurrentlyCrashes` defect
  (TRACEABILITY.md Part 1) on this real DD leaf too. The UDA cell is not run,
  to avoid crashing before UDA participates; that makes this row an explicit
  terminal gap rather than a legitimate storage-model divergence.
- **18 cells round-trip cleanly**, including all 5 AOS-nested cells that are
  *not* dynamic leaves (INTEGER r3, DOUBLE r6 via `temporary`'s static
  self-test fields) and every plain (non-AOS) cell of every datatype,
  including COMPLEX r2/r4 — confirming UDA's remote get() correctly forwards
  static, non-AOS, and statically-nested-in-AOS fields alike.
- **NEW DEFECT discovered (xfail): a *dynamic* (time-varying) DD leaf nested
  inside a struct_array silently returns empty/absent data through UDA remote
  mode + `backend=hdf5`, instead of what was written.** Confirmed empirically
  against the reference stack by comparing the request trace's `dynamic_flags`
  field across passing and failing cells: `temporary/constant_integer3d/value`
  and `temporary/constant_float6d/value` (both AOS-nested, `dynamic_flags=0`)
  round-trip cleanly, while `waves/coherent_wave/full_wave/e_field/plus/values`
  (COMPLEX r1, 3 AOS levels), `runaway_electrons/distribution/markers/
  orbit_integrals_instant/values` (COMPLEX r3) and `…/orbit_integrals/values`
  (COMPLEX r5) — all `dynamic_flags=1` — come back with `read_shape={}` /
  zero elements. The same wall independently reproduces on a DOUBLE leaf
  outside this matrix, `equilibrium/time_slice/global_quantities/ip`
  (`test_uda_breadth.cpp`'s `UdaAosKnownDefects`), which additionally shows
  the *scalar* failure mode: a rank-0 dynamic leaf inside an AOS comes back
  with `code==0` but the HDF5 "absent" sentinel value (`-9.0e40`) instead of
  what was written, rather than an empty array. Root cause is not yet
  isolated to a specific line (out of this issue's "pin, don't fix" scope),
  but the shape strongly suggests the server-side `IMAS` plugin's HDF5-backend
  translation of a struct_array element index does not compose with a
  dynamic (time-tensorized) leaf's own leading time-index dimension. Each of
  the three COMPLEX cells is pinned against its exact real DD path, rank, and
  AOS nesting by the parameterized pair `Uda/UdaRealPathMatrixKnownDefects.
  DISABLED_DynamicComplexLeafInsideAosRoundTrips/{COMPLEX_r1,COMPLEX_r3,
  COMPLEX_r5}` + tripwires `Uda/UdaRealPathMatrixKnownDefects.
  DynamicComplexLeafInsideAosCurrentlyReadsEmpty/{COMPLEX_r1,COMPLEX_r3,
  COMPLEX_r5}` (`test_uda_real_paths.cpp`). The DOUBLE-scalar
  `UdaAosKnownDefects` pair remains an independent value-transport pin, not a
  substitute for these exact matrix cases or the active traversal proof below.

| Cluster / Capability | Test(s) | Status |
|---|---|---|
| Real-path breadth: 15 (type, rank) cells with a plain leaf field (no AOS) or a *static* AOS-nested field round-trip against real DD-4.1.1 paths spanning `equilibrium`, `b_field_non_axisymmetric`, `amns_data`, `magnetics`, `balance_of_plant`, `bolometer`, `gyrokinetics_local`, `temporary` — CHAR r1/r2, INTEGER r0–r3, DOUBLE r0–r6, COMPLEX r2/r4 | `Uda/UdaRealPathMatrix.RealPathRoundTrip/{CHAR_r1,CHAR_r2,INTEGER_r0..INTEGER_r3,DOUBLE_r0..DOUBLE_r6,COMPLEX_r2,COMPLEX_r4}` (15 cases) | covered |
| ↳ CHAR r0 (a bare scalar char) — the UDA verdict cannot be exercised because fixture setup would reproduce the already-pinned HDF5 CHAR-scalar crash (`RoundTripKnownDefects`/Part 1) before UDA participates | `Uda/UdaRealPathMatrix.RealPathRoundTrip/CHAR_r0` (`GTEST_SKIP()`, not run) | terminal-gap |
| ↳ 13 (type, rank) cells the DD contains nowhere at any nesting depth (same facts as MDSplus's Part 4 breadth matrix): CHAR r3–r7, INTEGER r4–r7, DOUBLE r7, COMPLEX r0, COMPLEX r6–r7 | `Uda/UdaRealPathMatrix.RealPathRoundTrip/{CHAR_r3..CHAR_r7,INTEGER_r4..INTEGER_r7,DOUBLE_r7,COMPLEX_r0,COMPLEX_r6,COMPLEX_r7}` (13 cases, each `GTEST_SKIP()`) | terminal-gap |
| ↳ 3 (type, rank) cells hit the new dynamic-leaf-inside-AOS defect: COMPLEX r1 (`waves`, 3 AOS levels), COMPLEX r3/r5 (`runaway_electrons`, 1 AOS level) | Breadth cells: `Uda/UdaRealPathMatrix.RealPathRoundTrip/{COMPLEX_r1,COMPLEX_r3,COMPLEX_r5}` (`GTEST_SKIP()`, known-defect). Exact correct-contract pair: `Uda/UdaRealPathMatrixKnownDefects.DISABLED_DynamicComplexLeafInsideAosRoundTrips/{COMPLEX_r1,COMPLEX_r3,COMPLEX_r5}` + tripwires `Uda/UdaRealPathMatrixKnownDefects.DynamicComplexLeafInsideAosCurrentlyReadsEmpty/{COMPLEX_r1,COMPLEX_r3,COMPLEX_r5}` | **xfail** |

## AOS traversal, occurrences, list_filled_paths, pulse modes/errors, slice/time-range (issue #25)

`test_uda_breadth.cpp` — every remaining C-ABI-reachable read-side capability
and pulse-lifecycle mode from `FUNCTIONALITY_INVENTORY.md` Part 1 not already
covered by the tracer bullet, the equilibrium-seed parity fixture, or the
real-path matrix above. Read cases use the seed(HDF5)-then-reopen(UDA) shape;
the lifecycle cases drive the remote UDA URI directly through the C ABI.

**Characterization-discovered facts:**

- **AOS traversal (top-level) has a discriminating active C-ABI proof**:
  `temporary/constant_integer3d` is a real DD-4.1.1 struct_array with a static
  `value` leaf known to round-trip through UDA. Three elements with distinct,
  order-sensitive values are seeded through HDF5 in physical index order
  0→2→1 (so setup does not mirror the asserted traversal), then read in order
  0→1→2 around successive `al_iterate_over_arraystruct(..., 1)` calls after
  reopening through UDA.
  `UdaBreadthTest.AosTraversalAdvancesAcrossDistinctStaticElements` asserts the
  exact sequence, so a successful no-op, repeated element, or reordered
  element fails. It also ends the AOS action and verifies through the same
  public function that the invalid context returns an error status.
- **The dynamic-leaf-inside-AOS value defect remains separately pinned**:
  `equilibrium/time_slice` correctly reports its written size (3), but every
  element's `global_quantities/ip` value comes back as the HDF5 absent-scalar
  sentinel (`-9.0e40`) instead of what was written. The canonical D2 pair is
  `UdaAosKnownDefects.DISABLED_DynamicLeafInsideAosRoundTrips` + tripwire
  `UdaAosKnownDefects.DynamicLeafInsideAosCurrentlyReturnsSentinel`. Nested
  (multi-level) traversal is also exercised by the real-path matrix's static
  AOS cells (INTEGER r3, DOUBLE r6).
- **`al_get_occurrences` is real through UDA remote mode** (the reference
  `IMAS` server plugin implements `getOccurrences`, confirmed via the PRD's
  static finding): occurrence 0 and occurrence 2 (HDF5's own `<ids>_<N>`
  naming convention, seeded via HDF5 exactly like
  `Occurrences.Hdf5ListsWrittenOccurrences`) both list correctly through the
  UDA client, ownership (`malloc`'d list, caller frees) unchanged.
- **`al_list_filled_paths` is real through UDA remote mode**, restricted to
  `backend=hdf5` (`UDABackend::list_filled_paths` throws for any other
  backend value — this fixture already uses `backend=hdf5` throughout, so the
  restriction is never hit): every seeded leaf (`ids_properties/
  homogeneous_time`, `vacuum_toroidal_field/r0`) is discoverable, ownership
  (caller frees the list and every string) unchanged.
- **`OPEN_PULSE` genuinely round-trips to the server at open time** — this
  corrects an initial assumption from static reading that UDA's `openPulse`
  is purely local URI/env parsing. The debug trace shows `al_begin_
  dataentry_action` itself issuing `IMAS::open(...)`; when the remote pulse
  dir does not exist, the server-side HDF5 backend refuses immediately
  ("HDF5 master file not found"), which the UDA client re-throws through the
  C ABI as a non-zero `al_status_t`. This is genuine **parity** with the
  on-disk backends' `DataEntryModes.OpenPulseFailsWhenAbsent` contract
  (Part 1) — UDA remote mode does not weaken it.
- **All four open modes are forwarded with their documented lifecycle
  distinctions through UDA remote mode.** `FORCE_OPEN_PULSE` creates an absent
  remote HDF5 pulse that a later `OPEN_PULSE` can reopen; `CREATE_PULSE`
  refuses an existing pulse; and `FORCE_CREATE_PULSE` accepts an existing
  pulse. Each lifecycle check closes through `CLOSE_PULSE` and reopens through
  the same UDA URI, using only the public C ABI.
- **NEW DEFECT discovered (xfail): `ERASE_PULSE` behaves exactly like
  `CLOSE_PULSE` through the reference stack.** `al_close_pulse(...,
  ERASE_PULSE)` reports success, but a later `OPEN_PULSE` on the same remote
  URI also succeeds. This carries the server-side HDF5 backend's already-known
  ignore-the-close-mode defect across the UDA boundary; it is pinned here as
  an exact UDA verdict by `UdaBreadthTest.
  DISABLED_ErasePulseMakesRemotePulseUnopenable` + tripwire `UdaBreadthTest.
  ErasePulseCurrentlyLeavesRemotePulseOpenable`.
- **Reading a real DD leaf that was never written (but whose IDS/occurrence
  was seeded) succeeds and returns the HDF5 absent-scalar sentinel**
  (`-9.0e40`), exactly matching the local HDF5 backend's own documented
  contract (`al_lowlevel.cpp`'s `Lowlevel::setDefaultValue`, the same
  sentinel `RoundTripMatrix`'s generator is careful never to *write*,
  TRACEABILITY.md Part 1) — transparent parity, not a UDA-specific behavior,
  since the remote `get()` forwards straight through to the server-side
  HDF5 backend's own read semantics.
- **Reading a real DD IDS that was never written at all (no occurrence ever
  created) fails cleanly** — the server-side HDF5 backend has no file for
  that IDS at all, distinct from "leaf absent within an existing IDS file"
  above.
- **Slice reads round-trip correctly through UDA remote mode**: a real DD
  dynamic top-level leaf (`vacuum_toroidal_field/b0`, coordinate1 `time`) with
  `CLOSEST_INTERP` at a UDA `al_begin_slice_action` correctly resolves to the
  nearer sample — note this leaf is *not* nested inside a struct_array, so it
  does not hit the AOS defect above; `supportsTimeDataInterpolation()`'s
  server-version gate (`IMAS::version()` 1.8.0 > 1.4.0) already permits it.
- **NEW DEFECT discovered (xfail), distinct from the AOS one above and not
  UDA-specific in origin: `al_begin_timerange_action`'s `OperationContext`
  constructor (`src/al_context.cpp`, the `(ctx, dataobject, access, range,
  tmin, tmax, dtime, interp)` overload) never assigns the base `interpmode`
  member — only `time_range.interpolation_method`** — unlike the GLOBAL_OP
  and SLICE_OP constructors, which both set it. Every always-on backend's
  `readData()` ignores `getInterpmode()` for a TIMERANGE_OP context (they
  consult `time_range` instead), so the uninitialized member is silently
  never observed on any other backend — until UDA's remote `readData()`
  directive-builder (`src/uda/uda_backend.cpp`) calls `op_ctx->
  getInterpmode()` *unconditionally*, regardless of rangemode, to fill the
  outgoing directive's `interp=` field. Confirmed empirically: the garbage
  value trips `uda_utilities.hpp`'s `InterpMode` convertor's default case,
  throwing `std::runtime_error("unknown interp mode: <garbage int>")`,
  surfaced through the C ABI as `al_plugin_read_data: unknown interp mode:
  ...`. Pinned: `UdaSliceAndTimeRange.
  DISABLED_TimeRangeReadWithoutResamplingThroughReopenSucceeds` + tripwire
  `UdaSliceAndTimeRange.
  TimeRangeReadWithoutResamplingCurrentlyFailsWithUnknownInterpMode`.

| Cluster / Capability | Test(s) | Status |
|---|---|---|
| AOS traversal, top-level (reported size + exact three-element forward sequence; invalid ended context returns an error; nested traversal proven separately by the real-path matrix's static AOS cells) | `UdaBreadthTest.AosTraversalAdvancesAcrossDistinctStaticElements` | covered |
| ↳ a *dynamic* leaf nested inside a struct_array returns the HDF5 absent-scalar sentinel instead of what was written | `UdaAosKnownDefects.DISABLED_DynamicLeafInsideAosRoundTrips` + tripwire `UdaAosKnownDefects.DynamicLeafInsideAosCurrentlyReturnsSentinel` | **xfail** |
| `al_get_occurrences` real through UDA remote mode, HDF5's `<ids>_<N>` storage naming and UDA's `<ids>/<N>` public naming pinned; every reported occurrence is reopened and read | `UdaBreadthTest.OccurrencesListsWrittenOccurrencesThroughReopen` | covered |
| `al_list_filled_paths` real through UDA remote mode (`backend=hdf5` only), ownership pinned | `UdaBreadthTest.ListFilledPathsThroughReopen` | covered |
| `al_begin_dataentry_action` mode matrix: `OPEN_PULSE` refuses an absent remote pulse; `FORCE_OPEN_PULSE` creates one; `CREATE_PULSE` refuses an existing one; `FORCE_CREATE_PULSE` accepts an existing one — genuine parity with the on-disk lifecycle contract | `UdaBreadthTest.{OpenPulseFailsWhenRemotePathAbsent,ForceOpenPulseCreatesRemotePulseWhenAbsent,CreatePulseRefusesExistingRemotePulse,ForceCreatePulseAcceptsExistingRemotePulse}` | covered |
| `al_close_pulse(..., CLOSE_PULSE)` retains the remote pulse for a later `OPEN_PULSE` | asserted by the reopen checks in `UdaBreadthTest.{ForceOpenPulseCreatesRemotePulseWhenAbsent,CreatePulseRefusesExistingRemotePulse,ForceCreatePulseAcceptsExistingRemotePulse}` | covered (indirect) |
| ↳ `al_close_pulse(..., ERASE_PULSE)` reports success but leaves the remote pulse openable, carrying the server-side HDF5 close-mode defect across the UDA boundary | `UdaBreadthTest.DISABLED_ErasePulseMakesRemotePulseUnopenable` + tripwire `UdaBreadthTest.ErasePulseCurrentlyLeavesRemotePulseOpenable` | **xfail** |
| Reading an unwritten leaf on a seeded IDS returns the HDF5 absent-scalar sentinel — transparent parity with the local HDF5 backend's own contract, not UDA-specific | `UdaBreadthTest.ReadOfUnwrittenLeafOnSeededIdsReturnsHdf5AbsentSentinel` | covered |
| Reading a never-written IDS fails cleanly | `UdaBreadthTest.ReadFailsForNeverWrittenIds` | covered |
| Slice read (`CLOSEST_INTERP`) on a real DD dynamic top-level leaf round-trips correctly through UDA remote mode | `UdaSliceAndTimeRange.SliceReadThroughReopen` | covered |
| ↳ time-range read (no resampling) fails with an "unknown interp mode" exception — `OperationContext`'s TIMERANGE_OP constructor never initializes the base `interpmode` member, only observable because UDA's remote directive-builder reads it unconditionally | `UdaSliceAndTimeRange.DISABLED_TimeRangeReadWithoutResamplingThroughReopenSucceeds` + tripwire `UdaSliceAndTimeRange.TimeRangeReadWithoutResamplingCurrentlyFailsWithUnknownInterpMode` | **xfail** |

## UDA-unique surface (issues #26 + #27): cache modes, runtime DD loading, datapath, capability negotiation, fetch mode, write/delete pins

`test_uda_unique_surface.cpp` — one dedicated unique-surface file covering all
ten areas, against the reference stack (UDA server 2.9.3 / `IMAS` plugin 1.8.0
/ DD 4.1.1). A second, older DD version (**3.42.0**, `IDSDEF_PATH_OLDER`) was
added to `docker/uda/Dockerfile` for the wrong-version-DD row below; it is a
real, independently-sourced pin (`imas-data-dictionary==3.42.0` from PyPI),
recorded here alongside the header's existing stack identifiers.

**Characterization-discovered facts:**

- **URI options accepted/rejected:** `cache_mode` rejects anything outside
  `{none, ids, struct}` at open time, purely client-side, before any network
  round trip (`UDABackend::process_options`). `verbose` toggles debug tracing
  on stdout (captured directly via `CaptureStdout`, not inferred). `plugin`
  naming a name the reference stack never registered fails at open — the
  client's `init` directive reaches a real server with no such plugin.
  `init_args` is accepted but has **no observable effect** against the
  reference `IMAS` plugin: its `init()` handler (`imas_plugin.cpp`,
  `handle_request`) takes no arguments at all, only tracking a per-connection
  `_init` bool — confirmed by reading the plugin's own dispatch, then
  empirically (a read with arbitrary `init_args` behaves identically to one
  without). `dd_version` override is forwarded verbatim on every directive but
  is never consulted by any backend on either side of the wire (client schema
  resolution is driven by the file loaded at `$IDSDEF_PATH`; the server's HDF5
  backend is DD-agnostic) — accepted, no observable effect. A host that
  resolves but a port nothing listens on fails to connect, distinct from every
  post-connection request failure characterized above.
- **NEW DEFECT discovered (xfail): cache mode changes a physics value for a
  top-level field.** `none` and `ids` agree on any field; `ids` populates the
  whole (or datapath-scoped) IDS at `beginAction`, while `struct` populates
  lazily, per struct_array, only inside `beginArraystructAction`. A static leaf
  nested inside a struct_array therefore returns the identical value under all
  three modes. A plain top-level field, however, is never populated under
  `cache_mode=struct`, so it silently reads back as the absent-leaf sentinel
  even though it was genuinely written. That violates issue #26's requirement
  that cache modes never alter data. Pinned:
  `UdaUniqueSurfaceTest.DISABLED_CacheModeStructPreservesTopLevelField` +
  tripwire
  `UdaUniqueSurfaceTest.CacheModeStructCurrentlyReturnsAbsentSentinelForTopLevelField`.
- **NEW DEFECT discovered (xfail), found while characterizing "cache cleared
  on close": closing a UDA remote-mode session leaks a server-side pulse
  handle.** Root cause traced by comparing the exact directives exchanged
  (`verbose=1`): `UDABackend::openPulse`/`closePulse` both strip `cache_mode`
  and `verbose` before building the `uri=` argument sent to the server, so
  `IMAS::open(...)` and `IMAS::close(...)` agree on the same key — but two of
  the *read*-triggering directive builders don't follow that convention:
  `readData`'s `cache_mode=None` branch and `populate_cache` (used by `ids`
  and `struct` alike, both in `src/uda/uda_backend.cpp`) each build their own
  `IMAS::get(uri=...)` from the same query object but only ever remove
  `"backend"` — never `cache_mode`/`verbose` — so their `uri=` differs from
  the one `open`/`close` agreed on. (`beginArraystructAction`'s own `None`-mode
  branch does strip both, matching `open`/`close` — this is specifically a
  `readData`/`populate_cache` inconsistency, not a blanket "every `get()` path"
  one.) The reference `IMAS` plugin's `get()` handler treats an unrecognized
  `uri=` as a new pulse and implicit-opens it (`imas_plugin.cpp`), creating a
  second, real server-side HDF5 file handle that the client's `close()`
  directive can never reach (it only ever sends the stripped key) — leaked for
  the lifetime of the server's per-connection process. HDF5's default file
  locking then blocks a later *local* open of the identical on-disk file until
  that server process exits. Reproduces identically whether the read went
  through `readData`'s `None`-mode path or `populate_cache`'s `ids`/`struct`-
  mode path (both hit the same unstripped-uri bug), so this is not an
  `ids`-specific finding despite living in the cache-mode area: the
  client-side RAM cache genuinely *is* cleared on close
  (`UDABackend::closePulse`), but a server-side resource opened on the read
  path outlives it regardless. Pinned:
  `UdaUniqueSurfaceTest.DISABLED_ClosingUdaSessionReleasesEveryServerSideHandle`
  + tripwire `UdaUniqueSurfaceTest.ClosingUdaSessionCurrentlyLeaksServerSideHandle`.
- **Runtime DD loading — present / absent / wrong-version, all through
  `al_begin_dataentry_action`** (the constructor loads `IDSDef.xml`
  unconditionally, before any network access): present is the reference
  stack's default (DD 4.1.1, restated here as the area-3 baseline). Absent has
  two distinct failure messages, matching issue #23's empirical findings:
  neither `$IDSDEF_PATH` nor `$IMAS_PREFIX` set → "neither IMAS_PREFIX or
  IDSDEF_PATH environmental variable is set"; `$IDSDEF_PATH` set but the file
  missing → "IDSDef.xml not found at either $IDSDEF_PATH or
  $IMAS_PREFIX/include/IDSDef.xml". **Wrong-version acceptance is a defect**:
  pointing `$IDSDEF_PATH` at a real, structurally valid, but older DD (3.42.0)
  than the reference stack's own 4.1.1 loads without
  complaint — `load_xml()`/`get_dd_version()` never compare the loaded file's
  version against anything (not the data's actual DD version, not the
  server's). A real path stable across both schemas
  (`vacuum_toroidal_field/r0`) round-trips exactly as if the "correct" DD had
  been loaded — nothing signals that a version-drifted schema is in use. Same
  practical risk class NORTH_STAR.md flags for COCOS-sign-flip-bearing paths:
  this mechanism checks path existence only, never semantic version
  agreement. Pinned:
  `UdaUniqueSurfaceTest.DISABLED_DdWrongVersionIsRejected` + tripwire
  `UdaUniqueSurfaceTest.DdWrongVersionCurrentlyLoadsSilentlyWithNoCrossVersionCheck`.
- **`datapath` genuinely hides data, not just reorders fetch priority.**
  With `cache_mode=ids` and a non-empty `datapath`, `populate_cache` only
  requests paths under `<ids>/<datapath>`'s subtree
  (`UDABackend::beginAction`, `uda_backend.cpp:1016-1032`). A real, written
  field *outside* that subtree is not in the cache and is not one of the two
  homogeneous-time/version preconditions, so `readData` falls through to
  `return 0` — the ordinary absent-leaf contract — even though the plain
  HDF5 backend underneath genuinely has it. This is the evidence backing the
  `FUNCTIONALITY_INVENTORY.md`/`CLAUDE.md` claim (corrected by this issue)
  that UDA is the one living exception to "no backend uses `datapath`".
- **NEW DEFECT discovered (xfail): version-drift check inertness, confirmed
  from source**:
  `al_lowlevel.cpp`'s open-time comparison
  (`getVersion(NULL)` vs. `getVersion(pctx)`) can never fire for UDA — both
  sides are hardcoded `{0, 0}` placeholders (`UDABackend::getVersion`,
  `uda_backend.h`'s `UDA_BACKEND_VERSION_MAJOR/MINOR` and the "temporary
  placeholder" non-null-ctx branch), so `(0!=0)||(0<0)` is always false
  regardless of what is genuinely stored remotely. An unavailable stored
  version must not be treated as verified compatibility. Pinned with a
  genuinely MISMATCHED stored fixture (issue #39): the pulse's
  `HDF5_BACKEND_VERSION` is rewritten to `999.0` out-of-band by
  `hdf5_fixture_tool` (issue #36's isolated producer; fixture preparation
  only — every asserted behavior stays on the public C ABI). Observed against
  the reference stack: the mismatched pulse is REFUSED, but by the
  SERVER-side HDF5 open (the server plugin's own IMAS-Core hits
  `HDF5BackendFactory`'s "No backend writer with version: 999.0"), forwarded
  to the client as exactly `UNKNOWN_ERR` (-1); the client-side check itself
  stays inert — its own `LOWLEVEL_ERR` "Compatibility …" refusal never
  appears, which is what the active tripwire pins. Pinned:
  `UdaUniqueSurfaceTest.DISABLED_OpenRefusesMismatchedStoredBackendVersion`
  (client-side refusal, correct contract) + tripwire
  `UdaUniqueSurfaceTest.VersionDriftCheckCurrentlyDefersToServerSideRefusal`,
  with baseline `UdaUniqueSurfaceTest.MatchingStoredVersionOpensThroughUda`.
- **`supportsTimeRangeOperation()` capability negotiation confirmed against
  the reference server**: the reference plugin reports `1.8.0` (issue #23),
  so `1.8.0 > 1.4.0` grants the capability — `al_begin_timerange_action` must
  pass `al_lowlevel.cpp`'s capability gate ("Selected backend does not
  support time range operations.") rather than being refused outright,
  distinct from the separate, already-pinned uninitialized-interpmode defect
  on the subsequent read (`UdaSliceAndTimeRange`, above).
- **Fetch mode: download, local-backend handoff, cache reuse, all confirmed
  end-to-end (issue #27)** — the `BYTES` server plugin fetch mode needs ships
  and is registered **by default** in `ukaea/uda`'s own build (corrects this
  file's `docker/uda/README.md` spike note that it wasn't registered; that was
  an unverified assumption, not an empirical finding). `UDABackend`'s
  constructor lists the remote pulse's files and downloads `master.h5` via
  `BYTES::read`, then delegates every subsequent `Backend` virtual straight to
  a freshly constructed local backend over the download — confirmed by a
  correct scalar read-back. A reopen against the same cache reuses it without
  re-downloading, confirmed directly via `download_file`'s own verbose trace
  lines (`"...cache directory already exists"`, `"...cached local file
  already exists"`), not inferred.
- **`local_cache` overrides only the cache *root*, not the whole path**: with
  an explicit `local_cache`, `fetch_files` still joins it with the remote
  path's `relative_path()` (`uda_backend.cpp:428-429`) — the same nesting the
  default `$TMPDIR/uda-cache-of-$USER/<remote_path>` formula applies, just
  under a different root.
- **The stale-cache / write-divergence pin, end-to-end (issue #27)**: a write
  through fetch mode "succeeds" (`UDABackend::writeData` delegates straight to
  the local backend once `access_local_` is set, `uda_backend.cpp:1107-1109`
  — there is no upload path back to the server at all), but lands only on the
  local cache copy. Confirmed three ways in one test: (1) the write reports
  success; (2) reopening the identical pulse via ordinary *remote* mode shows
  the server-side data is genuinely unchanged; (3) reopening *fetch* mode
  again still serves the divergent local value, because `download_file`'s
  early-exists-return (area 7) never re-fetches the true server-side value —
  the divergence is permanent until the cache dir is manually cleared. (Pinned
  against a brand-new field, not the already-seeded scalar: overwriting
  pre-existing data through a fresh `WRITE_OP` session hits a separate, HDF5-
  backend-wide Limit — `HDF5Writer::write_ND_Data` unconditionally calls
  `H5Dcreate2` for any dataset not already tracked in *this session's*
  in-memory map, with no `H5Lexists` check against the file
  (`hdf5_writer.cpp:417-423`) — orthogonal to UDA and out of this issue's
  scope; exercising it here would have conflated two different defects.)
- **NEW DEFECT discovered (xfail): remote write and delete diverge in how
  they surface the same underlying dispatch failure (issue #27)** — a finding
  only visible by driving the actual client library calls, not by probing the
  server with `uda_cli` directly (as issue #23's spike did): the reference
  `IMAS` server plugin has no `writeData`/`deleteData` handler at all; both
  raw directives return `[handle_request]: Unknown function requested!`.
  `UDABackend::deleteData` issues its directive via `uda::Client::get()`,
  whose error path *does* propagate that failure — caught as a
  `uda::UDAException`, re-thrown as an `ALException`, surfacing at
  `al_delete_data` as a non-zero `al_status_t` carrying that exact text: the
  intended, correctly-refused contract, confirmed and covered.
  `UDABackend::writeData` issues its directive via `uda::Client::put()`
  instead, which does **not** throw on an in-band server dispatch failure
  (only on transport-level faults) — so `al_write_data` reports
  `al_status_t.code == 0` (false success) while nothing is persisted. This is
  a more dangerous pin than a clean refusal: the caller is told the write
  succeeded with no signal that the server never received it, confirmed by
  reopening via remote mode afterward and finding the value unchanged.
  Pinned: `UdaUniqueSurfaceTest.DISABLED_RemoteWriteFailsWithDispatchError` +
  tripwire
  `UdaUniqueSurfaceTest.RemoteWriteCurrentlyReportsSuccessButNeverPersists`.

| Cluster / Capability | Test(s) | Status |
|---|---|---|
| URI option surface: `cache_mode` invalid value throws; `verbose` toggles debug tracing (captured directly); `plugin` naming an unregistered plugin fails at open; `init_args` accepted with no observable effect against the reference plugin; `dd_version` override accepted with no observable effect; wrong port fails to connect | `UdaUniqueSurfaceTest.{InvalidCacheModeThrows,VerboseTrueEmitsDebugTracingOnStdout,VerboseAbsentEmitsNoDebugTracing,PluginOptionNamingUnregisteredPluginFailsAtOpen,InitArgsAcceptedAndIgnoredByReferencePlugin,DdVersionOverrideAcceptedWithNoObservableEffect,WrongPortFailsToConnectDistinctlyFromRequestFailures}` | covered |
| Cache-mode invisibility: `none`/`ids`/`struct` agree on a field all three can reach (a static AOS-nested leaf) | `UdaUniqueSurfaceTest.CacheModeNoneIdsStructAgreeForAosCoveredField` | covered |
| ↳ `struct` mode reports success but returns the absent sentinel for a genuinely written top-level field, so cache-mode selection changes the physics value | `UdaUniqueSurfaceTest.DISABLED_CacheModeStructPreservesTopLevelField` + tripwire `UdaUniqueSurfaceTest.CacheModeStructCurrentlyReturnsAbsentSentinelForTopLevelField` | **xfail** |
| ↳ closing a UDA session leaks a server-side pulse handle (uri-stripping mismatch between `open`/`close` and every `get()`-directive builder), blocking a later local reopen of the same file | `UdaUniqueSurfaceTest.DISABLED_ClosingUdaSessionReleasesEveryServerSideHandle` + tripwire `UdaUniqueSurfaceTest.ClosingUdaSessionCurrentlyLeaksServerSideHandle` | **xfail** |
| Runtime DD loading: present baseline and absent DD (both failure messages) | `UdaUniqueSurfaceTest.{DdPresentLoadsAndOpenSucceeds,DdAbsentNeitherEnvVarSetFailsWithClearMessage,DdAbsentFileMissingAtIdsDefPathFailsWithClearMessage}` | covered |
| ↳ wrong-version DD loads silently with no semantic cross-version check | `UdaUniqueSurfaceTest.DISABLED_DdWrongVersionIsRejected` + tripwire `UdaUniqueSurfaceTest.DdWrongVersionCurrentlyLoadsSilentlyWithNoCrossVersionCheck` | **xfail** |
| `datapath` partial-get via `cache_mode=ids`: in-scope field round-trips, out-of-scope field silently reads as absent | `UdaUniqueSurfaceTest.DatapathScopesCachePopulationFieldOutsideScopeReadsAsAbsent` | covered |
| Version-drift check inertness: the client's own drift check never fires (both sides hardcoded placeholders); a pulse whose stored backend version (`999.0`, rewritten out-of-band by `hdf5_fixture_tool`) can never match is refused only by the forwarded SERVER-side HDF5 error (`UNKNOWN_ERR`, "No backend writer with version: 999.0") | `UdaUniqueSurfaceTest.DISABLED_OpenRefusesMismatchedStoredBackendVersion` + tripwire `UdaUniqueSurfaceTest.VersionDriftCheckCurrentlyDefersToServerSideRefusal` (baseline: `MatchingStoredVersionOpensThroughUda`) | **xfail** |
| Server-version-gated `supportsTimeRangeOperation()`: reference plugin 1.8.0 > 1.4.0 grants the capability | `UdaUniqueSurfaceTest.TimeRangeCapabilityGrantedByReferenceServerVersion180` | covered |
| Fetch mode: download, local-backend handoff, correct read-back, cache reuse on reopen (confirmed via `download_file`'s own verbose trace) | `UdaUniqueSurfaceTest.FetchModeDownloadsHandsOffToLocalBackendAndReusesCacheOnReopen` | covered |
| `local_cache` overrides the cache root only — the remote path is still nested underneath it, same as the default formula | `UdaUniqueSurfaceTest.FetchModeLocalCacheOptionOverridesDefaultCacheDir` | covered |
| ↳ stale-cache / write-divergence pin: a fetch-mode write succeeds locally only, server-side pulse (reopened via remote mode) is unchanged, divergent local copy persists across close/reopen | `UdaUniqueSurfaceTest.FetchModeWriteDivergesFromServerAndStalePersistsAcrossReopen` | covered |
| Remote delete pinned unsupported (correctly refused): no `deleteData` handler on the reference plugin, `al_delete_data` surfaces the server's exact dispatch-failure text | `UdaUniqueSurfaceTest.RemoteDeleteFailsWithUnknownFunctionRequested` | covered |
| ↳ remote write reports **false success** instead of the same refusal: `uda::Client::put()`'s error path does not propagate the dispatch failure deleteData's `get()` surfaces, so `al_write_data` returns `code == 0` while nothing is persisted (confirmed by reopening and finding the value unchanged) | `UdaUniqueSurfaceTest.DISABLED_RemoteWriteFailsWithDispatchError` + tripwire `UdaUniqueSurfaceTest.RemoteWriteCurrentlyReportsSuccessButNeverPersists` | **xfail** |

**Progress (issue #27): UDA's unique surface is now fully characterized —
areas 1-6 (issue #26) plus fetch mode and the write/delete pins (areas 7-10)
close out `test_uda_unique_surface.cpp`. One new genuine defect discovered and
pinned (xfail, D2 correct-contract + tripwire): remote write silently reports
success without persisting, unlike remote delete's clean refusal. The
`docker/uda/` spike's assumption that the `BYTES` plugin fetch mode needs
wasn't registered on this reference stack did not hold up empirically — it
ships and registers by default in `ukaea/uda`'s own build — and is corrected
in `docker/uda/README.md` alongside this issue's findings.

**Progress summary (issue #28): zero blank rows remain in Part 5 — every one
of its 31 rows is terminal (19 `covered`/`covered (indirect)`, 1
`divergence`, 9 `xfail`, 2 `terminal-gap`), each xfail row's `DISABLED_`
correct-contract test and current-behavior tripwire both verified present in
`test_uda_real_paths.cpp`/`test_uda_breadth.cpp`/
`test_uda_unique_surface.cpp`, and every divergence
row's reasoning is backed by prose above its table, not just the one-line
cell. The pinned stack identifiers above (UDA server 2.9.3, UDA-Plugins
1.8.0 at commit `ede25b921081d8fc2d66c5b5ca152c664b50ee78`, DD 4.1.1,
plus the older DD 3.42.0 pin) match `docker/uda/`
verbatim. The corresponding shared always-on rows in Part 1/Part 2 above now
each carry a one-line "UDA: … see Part 5" note, mirroring Part 4's own
issue-#18 closure (Cluster 3 plugin management and Cluster 4
introspection/diagnostics carry no such note for either backend — both
clusters are backend-agnostic registry/lookup operations with no `Backend`
virtual dispatch, exercised once in Part 1's own tier, not a gap).
