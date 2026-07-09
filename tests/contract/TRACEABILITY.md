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
- **gap** — not yet tested; the owning later build-order step (TEST_STRATEGY §4)
  is named so the future issue knows which row to fill.

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

## Coverage status — as of the five implemented issues (#2 scaffold, #5
## introspection, #3 round-trip matrix, #6 capability-gated, #7 plugins)

The suite covers the **data-path, introspection, and plugin-management surface**
end to end and pins the defects those issues surfaced (eleven xfails, each with a
paired current-behavior tripwire). It does **not yet** cover the pulse-lifecycle
detail, arrays-of-structures, deletion, occurrences, or the Plugin-author
audience — those are the remaining build-order steps and appear below as explicit
**gap** rows, each tagged with the step that owns it:

- **Structured data** (TEST_STRATEGY §4 step 2, task doc `2b-structured.md`):
  AOS, `al_delete_data`, `al_get_occurrences`, full pulse lifecycle.
- **Plugins** (§4 step 5, task doc `5-plugins.md`): User Cluster 3 is now
  covered (register/bind/unbind/unregister state machine + quirks,
  `al_is_plugin_registered`, all three `al_setvalue_*` variants, four pinned
  defects). Still open: readback binding, `al_write_plugins_metadata`, and all
  of Part 3 (the Plugin-author audience).
- **Ownership sweep** (§4 step 6, task doc `6-ownership-sweep.md`): the
  caller-frees questions and the remaining audience sweep.

GitHub issue numbers for those three are to be filled in when the issues are
opened; until then they are referenced by build-order step + task-doc slug.

---

# Part 1 — User (HLI implementer) audience

## Cluster 1 — Pulse lifecycle  (`FUNCTIONALITY_INVENTORY.md:86-149`)

| Capability | Inventory ref | Unit | Integ. | Test(s) — and residual | Status |
|---|---|:--:|:--:|---|---|
| `al_begin_dataentry_action` opens a data entry from a URI | :90-107 | ✓ (Memory) | ✓ (HDF5, ASCII) | `FORCE_CREATE_PULSE` asserted OK in every suite (`Backends/RoundTrip.*`, `RoundTripMatrix.*`, `VersionSentinel.*`, `CapabilityMatrix.*`). Residual: `OPEN_PULSE`/`FORCE_OPEN_PULSE`/`CREATE_PULSE` modes + error paths untested → step 2 | covered (indirect) |
| `al_close_pulse` closes (`CLOSE_PULSE`) / erases (`ERASE_PULSE`) | :109-116 | ✓ (Memory) | ✓ (HDF5, ASCII) | `CLOSE_PULSE` asserted `code==0` in every suite. Residual: `ERASE_PULSE` untested → step 2 | covered (indirect) |
| `al_context_info` describes a context; **caller frees `*info`** | :118-126 | — | — | — → ownership sweep (step 6) | gap |
| `al_get_backendID` returns the active `BACKEND` for a context | :128-135 | — | — | — → step 2 | gap |
| `al_build_uri_from_legacy_parameters` builds a URI from legacy params | :137-149 | ✓ | ✓ | `al_contract::build_uri` (asserts OK) drives HDF5·Memory·ASCII in all on-disk suites. Caller-frees of `*uri` → ownership sweep (step 6) | covered |
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
| `al_begin_global_action` starts a GLOBAL read/write op | :159-175 | ✓ (Memory) | ✓ (HDF5, ASCII) | Vehicle for every round trip: `RoundTrip.*`, `RoundTripMatrix.*`, `VersionSentinel.*`, `CapabilityMatrix.*` (WRITE_OP + READ_OP). `datapath` partial-get is UDA-only (out of always-on scope) | covered |
| `al_begin_slice_action` starts a time-slice op (READ interp; WRITE append via `UNDEFINED_TIME`) | :177-194 | ✓ (Memory) | ✓ (HDF5) | HDF5·Memory positive / ASCII·Flexbuffers refused `BACKEND_ERR`: `CapabilityMatrix.SliceReadPositiveOrRefused`, `CapabilityMatrix.SliceWriteBeginPositiveOrRefused`. Interp modes + missing-interp refusal (`CONTEXT_ERR`): `Hdf5TimeDependent.SliceInterpolationModes`, `Hdf5TimeDependent.SliceReadWithoutInterpModeIsRejected` | covered |
| ↳ WRITE append via `UNDEFINED_TIME` must accumulate — **only the last slice persists in this build** | :177-194; al_lowlevel.h:300-316 | — | ✓ | `Hdf5SliceAppend.DISABLED_AppendedSlicesAllPersist` + tripwire `Hdf5SliceAppend.OnlyLastAppendedSlicePersists_CurrentBehavior` | **xfail** |
| `al_begin_timerange_action` starts a time-range op (with/without resampling `dtime`) | :196-221 | ✓ (Memory) | ✓ (HDF5) | HDF5 positive (no-resample + LINEAR resample): `Hdf5TimeDependent.TimeRangeReadWithoutResampling`, `…WithResampling`; Memory·ASCII·Flexbuffers refused `LOWLEVEL_ERR`: `CapabilityMatrix.TimeRangeReadPositiveOrRefused` | covered |
| `al_begin_arraystruct_action` starts an AOS op (top-level + nested) | :223-233 | — | — | — → structured data (step 2) | gap |
| `al_end_action` ends any context | :235-242 | ✓ (Memory) | ✓ (HDF5, ASCII) | asserted OK in every suite | covered (indirect) |
| `al_write_data` → `al_read_data` round trip preserves value + shape (INTEGER/DOUBLE/COMPLEX scalar→7-D on HDF5·Memory·ASCII; CHAR scalar/1-D/2-D on HDF5·ASCII, all ranks on Memory) | :244-263 | ✓ (Memory) | ✓ (HDF5, ASCII) | `Backends/RoundTripMatrix.ReadEqualsWrite/*`; generator `al_contract.h` `synth_value`/`synth_buffer`/`shape_for_rank` | covered |
| ↳ Flexbuffers must-refuse column (serializer, not a pulse store): write accepted, read refused, every datatype × shape | :244-263 / D4 | — | ✓ | `…/Flexbuffers_*_r{0..7}` (paired-negative) | covered |
| ↳ CHAR > 2-D refused (documented "not implemented") on HDF5·ASCII | :244-263 / D4 | — | ✓ | `…/{HDF5,ASCII}_CHAR_r{3..7}` (paired-negative) | covered |
| ↳ CHAR scalar (dim 0) must round-trip — **HDF5 crashes** | :244-263 | — | ✓ | `RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips` + tripwire `RoundTripKnownDefectsDeath.Hdf5CharScalarCurrentlyCrashes` | **xfail** |
| ↳ numeric at MAXDIM (rank 7) must round-trip — **ASCII corrupts (DOUBLE/COMPLEX) / aborts (INTEGER)** | :244-263 | — | ✓ | `RoundTripKnownDefects.DISABLED_Ascii{Integer,Double,Complex}MaxdimRoundTrips` + tripwires `RoundTripKnownDefectsDeath.AsciiIntegerMaxdimCurrentlyAborts`, `RoundTripKnownDefects.Ascii{Double,Complex}MaxdimCurrentlyCorrupts` | **xfail** |
| `al_delete_data` at signal / structure / DATAOBJECT-root granularity | :265-272 | — | — | — → structured data (step 2) | gap |
| `al_iterate_over_arraystruct` advances the AOS cursor | :274-282 | — | — | — → structured data (step 2) | gap |
| `al_get_occurrences` lists non-empty occurrences; caller-frees `*occurrences_list` | :284-292 | — | — | — → structured data (step 2); ownership → step 6 | gap |
| `al_list_filled_paths` lists filled leaf paths; **caller frees list + strings** | :294-314 | ✓ (Memory) | ✓ (HDF5) | HDF5 positive (leaves discoverable, list freed) / Memory·ASCII·Flexbuffers refused `BACKEND_ERR`: `CapabilityMatrix.ListFilledPathsPositiveOrRefused` | covered |

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
suite is `unit` (in-process registry, no on-disk backend). Still deferred:
readback binding and `al_write_plugins_metadata` (need a get/put through a
readback-implementing plugin — Part 3 / ownership sweep).

| Capability | Inventory ref | Unit | Integ. | Test(s) — and residual | Status |
|---|---|:--:|:--:|---|---|
| `al_register_plugin` / `al_unregister_plugin` lifecycle (register happy path; register-twice throws; unregister-unknown throws; **framework-gated**) | :325-360 | ✓ | — | `PluginTest.{RegisterMakesPluginRegistered, RegisterTwiceReturnsError, UnregisterNeverRegisteredNameReturnsError, UnregisterBoundPluginDestroysIt, RegisterWithFrameworkDisabledReturnsError}` | covered |
| ↳ unregister of a **registered-but-never-bound** plugin must destroy it — **it is left un-destroyed** (`unregisterPlugin` destroy+erase run only inside the `boundPlugins` loop) | :347-354; src/al_lowlevel.cpp:382-431 | ✓ | — | `PluginTest.DISABLED_UnregisterNeverBoundPluginDestroysIt` + tripwire `PluginTest.UnregisterNeverBoundPluginLeavesItRegistered_CurrentBehavior` | **xfail** |
| ↳ `register` on an existing-but-unloadable `.so` must report the **dlopen** failure — the failure is guarded only by an `assert`, stripped under `-DNDEBUG` (misleading downstream "Cannot load symbol create"; would `SIGABRT` in an assert-enabled build) | :355-360; src/al_lowlevel.cpp:350-357 | ✓ | — | `PluginTest.DISABLED_RegisterUnloadableSharedLibReportsDlopenFailure` + tripwire `PluginTest.RegisterUnloadableSharedLibSwallowsAssert_CurrentBehavior` | **xfail** |
| `al_bind_plugin` / `al_unbind_plugin` (bind registered OK; bind-unregistered throws; double-bind throws; unbind-unbound silent no-op) | :362-376 | ✓ | — | `PluginTest.{BindRegisteredPluginSucceeds, BindUnregisteredPluginReturnsError, DoubleBindSamePathReturnsError, UnbindNeverBoundPathIsSilentNoOp}` | covered |
| `al_bind_readback_plugins` / `al_unbind_readback_plugins` | :378-389 | — | — | needs a readback-implementing plugin + a get op → Part 3 / ownership sweep | gap |
| `al_is_plugin_registered` boolean query (true/false; framework-gated) | :391-397 | ✓ | — | `PluginTest.{RegisterMakesPluginRegistered, IsPluginRegisteredIsFalseForNeverRegisteredName, IsPluginRegisteredWithFrameworkDisabledReturnsError}` | covered |
| `al_setvalue_parameter_plugin` (generic typed variant) on a registered plugin | :399-420 | ✓ | — | `PluginTest.SetValueGenericOnRegisteredPluginSucceeds` | covered |
| ↳ generic variant on an **unregistered** name must error, not crash | :399-417 | ✓ | — | `KnownDefects.DISABLED_SetValueGenericUnregisteredPluginReturnsError` + tripwire `KnownDefectsDeath.SetValueGenericUnregisteredPluginCurrentlyCrashes` (SIGSEGV) | **xfail** |
| `al_setvalue_int_scalar_parameter_plugin` on a registered plugin reaches `setParameter` with the right value | :399-417 | ✓ | — | `PluginTest.SetValueIntScalarOnRegisteredPluginReachesPlugin` (asserts the value via the plugin's parameter log) | covered |
| ↳ int-scalar variant on an **unregistered** name must error, not crash | :399-417 | ✓ | — | `KnownDefects.DISABLED_SetValueIntScalarUnregisteredPluginReturnsError` + tripwire `KnownDefectsDeath.SetValueIntScalarUnregisteredPluginCurrentlyCrashes` (SIGSEGV) | **xfail** |
| `al_setvalue_double_scalar_parameter_plugin` on a registered plugin reaches `setParameter` with the right value | :399-417 | ✓ | — | `PluginTest.SetValueDoubleScalarOnRegisteredPluginReachesPlugin` | covered |
| ↳ double-scalar variant on an **unregistered** name must error, not crash | :399-417 | ✓ | — | `KnownDefects.DISABLED_SetValueDoubleScalarUnregisteredPluginReturnsError` + tripwire `KnownDefectsDeath.SetValueDoubleScalarUnregisteredPluginCurrentlyCrashes` (SIGSEGV) | **xfail** |
| `al_write_plugins_metadata` | :422-430 | — | — | needs bound provenance plugin + open ctx → Part 3 / ownership sweep | gap |

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
| **A** `initBackend` factory selects the backend from the URI | :496-514 | ✓ (Memory) | ✓ (HDF5, ASCII) | every always-on backend instantiated via `build_uri` in `RoundTripMatrix`. Unrecognized-ID throw untested → step 6 | covered (indirect) |
| **B** `getVersion` (installed vs stored, drift check) | :520-540 | — | — | version-drift guard untested → step 6 | gap |
| **B** `openPulse` / `closePulse` | :542-552 | ✓ (Memory) | ✓ (HDF5, ASCII) | via `al_begin_dataentry_action` / `al_close_pulse` in all suites | covered (indirect) |
| **C** `beginAction` / `endAction` | :558-574 | ✓ (Memory) | ✓ (HDF5, ASCII) | via `al_begin_global_action` / `al_end_action` | covered (indirect) |
| **C** `writeData` / `readData` (0=not-found vs 1=success inner convention) | :576-589 | ✓ (Memory) | ✓ (HDF5, ASCII) | via `al_write_data` / `al_read_data` — `RoundTripMatrix` (+ the CHAR-scalar / ASCII-maxdim xfails above) | covered |
| **C** `deleteData` | :591-599 | — | — | — → structured data (step 2) | gap |
| **C** `beginArraystructAction` | :601-606 | — | — | — → structured data (step 2) | gap |
| **D** `get_occurrences` | :612-616 | — | — | — → structured data (step 2) | gap |
| **D** `list_filled_paths` (per-backend: HDF5 real, others throw) | :618-640 | ✓ (Memory) | ✓ (HDF5) | via `al_list_filled_paths` — `CapabilityMatrix.ListFilledPathsPositiveOrRefused` pins both the HDF5 impl and the unconditional throw on Memory·ASCII·Flexbuffers | covered |
| **E** `supportsTimeDataInterpolation` / `supportsTimeRangeOperation` | :644-690 | ✓ (Memory) | ✓ (HDF5) | asserted through their sole ABI consequence (op accepted vs refused per backend): `CapabilityMatrix.{TimeRange,Slice,SliceWriteBegin}*`. The empirically-corrected fact that **Memory supports slice** (D2 over the issue's assumption) is encoded here | covered (indirect) |
| **E** `initDataInterpolationComponent` (framework-driven) | :644-690 | — | — | no-op where the factory calls it; not separately ABI-observable → step 6 | gap |

---

# Part 3 — Plugin author audience  (`FUNCTIONALITY_INVENTORY.md:708-895`)

Entirely deferred to the plugins issue (step 5). Listed as explicit gap rows so
the plugins issue has a concrete checklist rather than a blank slate.

| Cluster / Capability | Inventory ref | Unit | Integ. | Test(s) | Status |
|---|---|:--:|:--:|---|---|
| **1** Provenance metadata (`getName`/`getVersion`/`getCommit`/…) | :739-759 | — | — | — → plugins (step 5) | gap |
| **2** Parameter configuration (`setParameter` / `setParameters`) | :763-780 | ✓ | — | `setParameter` is reached (with the exact value) via `al_setvalue_*` on the registered fixture plugin: `PluginTest.SetValue{IntScalar,DoubleScalar}OnRegisteredPluginReachesPlugin` (asserted through the plugin's parameter log). The User-side unregistered-name crash is xfail'd in Part 1 Cluster 3. Residual: `setParameters` (bulk) untested → step 6 | covered (indirect) |
| **3** Action lifecycle & data interception (`begin_*_action`, `read_data`/`write_data`, `node_operation`) | :784-824 | — | — | — → plugins (step 5) | gap |
| **4** Readback metadata (`getReadback*`) | :827-850 | — | — | — → plugins (step 5) | gap |
| **5** Low-level reentry (`al_plugin_*`); `al_plugin_begin_timerange_action` is broken (declaration/definition mismatch) | :854-895 | — | — | — → plugins (step 5); the reentry bug is tracked separately as an upstream GitHub issue | gap |

---

## xfail bookkeeping — every xfail now has a paired tripwire

The D2 discipline is "correct-contract `DISABLED_` test **plus** a paired
current-behavior tripwire, so the xfail can't rot." All eleven xfail rows satisfy
it — a fix to any underlying defect turns its tripwire red, forcing whoever fixed
it to enable the paired `DISABLED_` correct-contract test:

| Defect | Correct-contract (`DISABLED_`) | Current-behavior tripwire |
|---|---|---|
| HDF5 CHAR scalar crashes | `RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips` | `RoundTripKnownDefectsDeath.Hdf5CharScalarCurrentlyCrashes` |
| ASCII rank-7 INTEGER aborts | `…DISABLED_AsciiIntegerMaxdimRoundTrips` | `RoundTripKnownDefectsDeath.AsciiIntegerMaxdimCurrentlyAborts` |
| ASCII rank-7 DOUBLE corrupts | `…DISABLED_AsciiDoubleMaxdimRoundTrips` | `RoundTripKnownDefects.AsciiDoubleMaxdimCurrentlyCorrupts` |
| ASCII rank-7 COMPLEX corrupts | `…DISABLED_AsciiComplexMaxdimRoundTrips` | `RoundTripKnownDefects.AsciiComplexMaxdimCurrentlyCorrupts` |
| `build_uri` can't address FLEXBUFFERS | `…DISABLED_BuildUriSupportsFlexbuffers` | `RoundTripKnownDefects.BuildUriFlexbuffersCurrentlyFails` |
| slice append doesn't accumulate | `Hdf5SliceAppend.DISABLED_AppendedSlicesAllPersist` | `Hdf5SliceAppend.OnlyLastAppendedSlicePersists_CurrentBehavior` |
| plugin setvalue (int) unregistered-name crash | `KnownDefects.DISABLED_SetValueIntScalarUnregisteredPluginReturnsError` | `KnownDefectsDeath.SetValueIntScalarUnregisteredPluginCurrentlyCrashes` |
| plugin setvalue (double) unregistered-name crash | `KnownDefects.DISABLED_SetValueDoubleScalarUnregisteredPluginReturnsError` | `KnownDefectsDeath.SetValueDoubleScalarUnregisteredPluginCurrentlyCrashes` |
| plugin setvalue (generic) unregistered-name crash | `KnownDefects.DISABLED_SetValueGenericUnregisteredPluginReturnsError` | `KnownDefectsDeath.SetValueGenericUnregisteredPluginCurrentlyCrashes` |
| unregister leaves a never-bound plugin registered | `PluginTest.DISABLED_UnregisterNeverBoundPluginDestroysIt` | `PluginTest.UnregisterNeverBoundPluginLeavesItRegistered_CurrentBehavior` |
| `register` swallows the dlopen failure (NDEBUG-stripped assert) | `PluginTest.DISABLED_RegisterUnloadableSharedLibReportsDlopenFailure` | `PluginTest.RegisterUnloadableSharedLibSwallowsAssert_CurrentBehavior` |
