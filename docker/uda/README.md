# UDA characterization environment (issue #23, spike for #21)

aarch64 image standing up the pinned **UDA reference stack** for the contract
suite's UDA tier (`tests/contract/test_uda.cpp`, and — as later subtasks of
[#21] land — `TRACEABILITY.md` Part 5). This subtask (#23) is the time-boxed
**feasibility spike**: assemble the stack and prove one end-to-end smoke round
trip through the public C ABI, then record the gate decision and the
empirically-resolved open items the PRD listed.

## Pinned reference stack

Every UDA verdict is "against reference stack X"; these are that X, and they are
pinned in `Dockerfile` (as `ARG`s + image labels) for the future TRACEABILITY
Part 5 header:

| Component | Source | Pin |
|-----------|--------|-----|
| UDA server | [`ukaea/uda`](https://github.com/ukaea/uda) | tag **2.9.3** |
| `IMAS` UDA server plugin | [`iterorganization/UDA-Plugins`](https://github.com/iterorganization/UDA-Plugins) | tag **1.8.0** |
| Data Dictionary | `imas-data-dictionary` (PyPI wheel) | **4.1.1** |
| Base image | `ubuntu:24.04` | arm64 |

`IMAS` plugin reported version (from `IMAS::version()`, confirmed against the
running stack): **1.8.0**. Since 1.8.0 > 1.4.0, the UDA backend's
`supportsTimeRangeOperation()` returns *true* against this reference stack
(`src/uda/uda_backend.cpp` parses `IMAS::version()` as a semver and compares
`> 1.4.0`) — the row-8 verdict a later subtask pins.

## Design: toolchain image + build-workspace-at-runtime

Mirrors the MDSplus leg (`docker/mdsplus/`): the **image** carries the pinned,
source-independent heavy layers — the built UDA server, the DD `IDSDef.xml`, the
UDA-Plugins source, xinetd — and a **run script** (`run.sh`) assembles the
source-dependent parts against the mounted workspace so the tests always
exercise the IMAS-Core under review, never a stale baked-in core. Concretely
`run.sh`:

1. builds + installs **IMAS-Core** (the client under test) with the HDF5 and UDA
   backends, from `/workspace`;
2. builds + installs + registers the **`IMAS` server plugin** against *that*
   IMAS-Core (via its `al-core.pc`) — and fails loudly if the plugin degrades to
   the mapping-only `NO_IMAS` build, which the PRD forbids;
3. starts the **UDA server** (xinetd fronts the per-connection `uda_server` on
   TCP 56565);
4. runs the **UDA contract tier** (`ctest -L uda`).

Because the plugin is compiled against the workspace's IMAS-Core, "linked
against server-side IMAS-Core, not `NO_IMAS`" is guaranteed structurally: the
plugin's `find_package(IMAS)` resolves the `al-core` pkg-config module the repo
installs (`pkgconfig/al-core.pc.in`), so `IMAS_FOUND` is true and `-DNO_IMAS` is
never added (`UDA-Plugins/source/imas/CMakeLists.txt`).

## Build

```sh
docker build --platform linux/arm64 -f docker/uda/Dockerfile \
  -t imas-core-uda:dev docker/uda
```

The build is source-independent: it does not COPY the repo, so it is cached
across code changes (only `Dockerfile`/`run.sh` edits invalidate it).

## Run the smoke round trip

From the repository root, mounting it into the container:

```sh
docker run --rm --platform linux/arm64 \
  -v "$PWD":/workspace -w /workspace \
  imas-core-uda:dev uda-run.sh
```

`uda-run.sh` builds the stack, starts the server, and runs `ctest -L uda`. Pass
an alternative command to stop after the stack is up, e.g. `... imas-core-uda:dev
uda-run.sh bash` for an interactive shell, or `... uda-run.sh ctest -L uda -R
Smoke --output-on-failure`.

Outside this container the same test is inert: `tests/contract/test_uda.cpp`
compiles out unless `AL_BACKEND_UDA=ON` (build gate `AL_CONTRACT_HAVE_UDA`), and
runtime-skips (`GTEST_SKIP()`) unless `UDA_HOST` is set — so ordinary local
builds and always-on CI legs never see it fail (TEST_STRATEGY.md D4).

## Cluster portability

The recipe makes no host-specific assumptions: a single base image, all
dependencies from `apt` + the DD PyPI wheel, no bind mounts other than the
workspace, and TCP 56565 kept inside the container. `UDA_ALLOWED_PATHS=/` (set
by `run.sh`) is acceptable only because this is a single-purpose, throwaway,
network-isolated reference container — it must not be copied to a shared server.

## Gate decision — FULL-STACK TIER CONFIRMED

The stack assembled within the time box and the smoke round trip passes:
`ctest -L uda` runs `UdaSmokeRoundTrip.ScalarSeededViaHdf5ReadsBackThroughUda`
green — a scalar seeded through the plain HDF5 backend reads back
byte-identical through the UDA backend in remote mode, across a real
`uda_server` running the `IMAS` plugin linked against a server-side IMAS-Core
(not `NO_IMAS`; `run.sh` fails loudly on that degradation). The fallback tier
(client-side-only, remote rows blocked-by-environment) is **not** invoked.

The subsequent subtasks of [#21] (read-only parity fixture, unique-surface file,
TRACEABILITY Part 5, the non-blocking `uda-contract` CI leg) can proceed on this
foundation.

## Empirical findings (open items from PRD #21)

Resolved from the running stack (2.9.3 / plugin 1.8.0 / DD 4.1.1), not from
static reading — recorded here for the Part 5 header and the unique-surface
subtask (#21 area 3).

- **`IDSDef.xml` runtime lookup + absent-file behavior** (client-side, the UDA
  backend's runtime DD coupling): lookup order is `$IDSDEF_PATH` first, else
  `$IMAS_PREFIX/include/IDSDef.xml` (`src/uda/uda_xml.cpp` `load_xml()`). Both
  failure modes surface through the C ABI at `al_begin_dataentry_action` (the
  `UDABackend` constructor loads the DD), `al_status_t.code = -1`:
  - neither var set → `neither IMAS_PREFIX or IDSDEF_PATH environmental variable
    is set`;
  - `$IDSDEF_PATH` set but the file missing → `IDSDef.xml not found at either
    $IDSDEF_PATH or $IMAS_PREFIX/include/IDSDef.xml`.
- **Remote-write / delete error surface:** the reference `IMAS` server plugin
  has no `writeData`/`deleteData` handler. Issuing the exact directives the
  client emits to the running server (`uda_cli --request "IMAS::writeData(...)"`
  / `"IMAS::deleteData(...)"`) both return `[handle_request]: Unknown function
  requested!`. The UDA backend catches the resulting `uda::UDAException` and
  re-throws it as an `ALException` carrying that text, which surfaces at the C
  ABI as a non-zero `al_status_t`. This empirically confirms the static finding
  that remote mode is read-only by construction; the exact C-ABI-level pin
  (correct-contract + tripwire, driven through `al_write_data`) is the
  write-path subtask's job, not the spike's.
- **Fetch-mode cache-directory naming/reuse semantics** (from source; full
  empirical characterization deferred to the fetch-mode subtask): the local
  cache dir is the `local_cache` URI option when given, else
  `${TMPDIR}/uda-cache-of-${USER}/<remote_path>` (falling back to
  `${TMPDIR}/uda-cache` when `$USER` is unset) (`src/uda/uda_backend.cpp`
  `fetch_files`). A file already present in the cache is **not** re-downloaded
  (`download_file` early-returns on existence), so a mutated or stale local copy
  persists across sessions — the divergence the fetch-mode subtask pins.
  Empirical exercise is deferred deliberately, not overlooked: fetch mode
  downloads via a `BYTES::read` directive to a **`BYTES` server plugin** that
  this reference stack does not register (only `IMAS` is), so a complete
  download+reuse round trip needs that extra server plugin — which the fetch
  subtask stands up. The spike's gate does not depend on it.
- **Reference plugin reported version:** `IMAS::version()` → **1.8.0** (see the
  pinned-stack note above for the `supportsTimeRangeOperation` consequence).
- **Pinned stack identifiers** (for the Part 5 header): UDA server **2.9.3**,
  `IMAS` plugin **1.8.0**, DD **4.1.1**, base `ubuntu:24.04` (arm64). Also
  recorded as `org.iter.imas-core.*` image labels.

[#21]: https://github.com/yohannmarguier/IMAS-Core/issues/21
