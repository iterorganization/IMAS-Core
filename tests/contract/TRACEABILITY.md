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

## In progress by a sibling issue (#2a — TEST_STRATEGY §4 step 2)

| Capability | Test(s) | Status |
|---|---|---|
| Synthetic round-trip matrix: {HDF5, Memory, ASCII, Flexbuffers} × {CHAR, INTEGER, DOUBLE, COMPLEX} × scalar→7-D | `Backends/RoundTripMatrix.ReadEqualsWrite/*` | owned by #2a (Flexbuffers cases currently red) |

---

## Remaining clusters — gaps to be filled by later build-order steps

Per TEST_STRATEGY.md §4, still open (not covered by this issue):

- **Cluster 1 — Pulse lifecycle**: open/info/close/erase, occurrences (step 2).
- **Cluster 2 — Data access**: `al_delete_data`, AOS write/iterate/read (step 2).
- **Cluster E — capability-gated ops**: slice / timerange / `list_filled_paths`
  — positive on HDF5, paired-negative "must-refuse" on the others (step 4).
- **Cluster 3 — Plugins**: register/bind/unbind/unregister, parameter-setting,
  state-machine quirks and the remaining defect death-tests (step 5).
- Backend-implementer / plugin-author capabilities reachable through the C ABI,
  swept until no row is blank (step 6).
