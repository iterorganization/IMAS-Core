# Running the tests

Everything here is driven through the presets in [`CMakePresets.json`](../CMakePresets.json)
— the same entry points CI uses, so a local run reproduces a CI leg exactly.

```sh
cmake --preset tests           # configure (AL_BUILD_TESTS=ON, no Python bindings)
cmake --build --preset tests   # build libal + test binaries
ctest --preset unit            # fast hermetic inner loop (< 1 s)
ctest --preset tests           # everything the test build registers
```

For the tiers that need a Docker backend (MDSplus, UDA), the [`./test`](../test)
dispatcher at the repo root wraps the image build + `docker run` recipes behind
one verb and delegates all cmake/ctest flags to these same presets:

```sh
./test common      # host-native: the three preset commands above
./test mdsplus     # Docker: bake the DD model tree, run the mdsplus tier
./test uda         # Docker: assemble the UDA reference stack, run the uda tier
./test all         # every tier + a PASS/FAIL summary table
```

## What lives where

| Suite | Location | Framework | Purpose |
|---|---|---|---|
| Smoke tests | `tests/testlowlevel.cpp`, `tests/testlowlevel_c.c` | none (plain executables) | End-to-end C ABI exercise; the `.c` build doubles as a compile canary that `al_lowlevel.h` stays C-compatible. |
| Contract suite | `tests/contract/` | GoogleTest + CTest | Pins the behavior of the public C ABI (`al_lowlevel.h` + `al_const.h`) capability by capability. The *why* is [`TEST_STRATEGY.md`](../TEST_STRATEGY.md); the coverage ledger is [`tests/contract/TRACEABILITY.md`](contract/TRACEABILITY.md). |
| Python binding tests | `python/tests/` | pytest | Bindings-level checks (import, exceptions, `imasdef` constants, HDF5 permissions). |

## Test tiers (CTest labels)

Every contract test carries exactly one tier, assigned post-discovery by
`tests/contract/assign_contract_labels.cmake.in` (the tier definitions live
there) and verified disjoint by `tests/contract/check_label_partition.sh`.

| `ctest --preset …` | Label | What runs | Needs |
|---|---|---|---|
| `unit` | `unit` | Pure lookups (`Introspection.*`), the in-RAM Memory backend matrix, plugin registry lifecycle + ABI death tests, URI ownership | Nothing — hermetic, no disk |
| `integration` | `integration` | HDF5 / ASCII / Flexbuffers round trips on disk | Disk only |
| `tests` / `tests-debug` | *(all)* | Contract suite + smoke tests, in the build mode CI's blocking legs use (both modes matter — issue #32) | — |
| `sanitize` | `contract` | The suite under ASan/UBSan (`AL_CONTRACT_SANITIZE=ON`), excluding `Death` and `CurrentlyCorrupts` cases — see the comments in `sanitizers.yml` for why | Linux for leak detection; on macOS prepend `ASAN_OPTIONS=detect_leaks=0` |
| `mdsplus` | `mdsplus` | MDSplus tier against the DD model tree the `mdsplus` configure preset bakes | MDSplus installed; configure downloads the DD (`-DDD_VERSION=<tag>` to pin) |
| *(no preset)* | `uda` | UDA tier against the pinned reference stack | `./test uda` (or `docker/uda/run.sh` inside the container — see `docker/uda/README.md`); there is deliberately no host preset |

Each test preset (except the run-everything ones' smoke tests) fails on an
empty selection (`noTestsAction: error`), so a broken label partition can't
pass silently.

For a one-off finer cut, plain CTest still works from the build directory:
`ctest -R Introspection`, `ctest -L integration -E Ascii`, etc.

## Reading the output

The suite encodes its own status vocabulary (full definitions in the
[TRACEABILITY.md preamble](contract/TRACEABILITY.md)):

- **`DISABLED_…` test + a paired `CurrentlyCorrupts`/tripwire test** — an
  *xfail*: the disabled test asserts the *correct* behavior of a known defect,
  the tripwire pins today's wrong behavior. The tripwire going red means
  someone fixed the defect — flip the pair, don't "fix the test".
- **`GTEST_SKIP()`** — either a legitimate backend *divergence* (the test
  shape doesn't transfer to that backend) or an unconfigured optional backend
  (e.g. `MDSPLUS_MODELS_PATH` / `UDA_HOST` unset). The skip message says which.

## CI legs

| Workflow | Preset(s) | Gate |
|---|---|---|
| `contract-tests.yml` | `tests`, `tests-debug` | **Blocking** on every PR |
| `sanitizers.yml` | `sanitize` | Informational (D7) |
| `mdsplus-contract.yml` | `mdsplus` (in Docker, DD pinned in the workflow) | Informational |
| `uda-contract.yml` | — (`docker/uda/run.sh`) | Informational |

## Python tests

```sh
pip install .[test]
pytest            # testpaths is rooted at python/tests in pyproject.toml
```

## Fixture tools

Two build-gated helper executables prepare states that must *not* be reachable
through the public C ABI (out-of-band file/tree surgery); the tests invoke
them as subprocesses: `hdf5_fixture_tool.cpp` (rewrites the stored
HDF5 backend version) and `mdsplus_fixture_tool.cpp` (rewrites the model
tree's version node). They are fixture *preparation*, never assertion — the
contract suite asserts only through the C ABI.
