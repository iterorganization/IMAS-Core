# AGENTS.md - IMAS-Core

Low-level IMAS Access Layer (AL core): a stable C ABI over a C++17 core with
pluggable storage backends and Cython/Python bindings. It reads/writes IMAS IDS
data addressed by Data Dictionary (DD) paths. **The active strategic effort is
the DD-version-agnostic migration — read `NORTH_STAR.md` before making
architectural changes.** A structural overview with diagrams lives in
`IMAS_CORE_STRUCTURE_MINDMAP.md`.

## Agent skills

### Issue tracker

GitHub Issues on the `origin` fork `yohannmarguier/IMAS-Core` (via the `gh` CLI); external PRs are not a triage surface. See `docs/agents/issue-tracker.md`.

### Triage labels

Canonical vocabulary, no overrides: `needs-triage` / `needs-info` / `ready-for-agent` / `ready-for-human` / `wontfix`, plus `bug` / `enhancement`. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: `CONTEXT.md` + `docs/adr/` at the repo root. See `docs/agents/domain.md`.

## Key architecture facts (audited 2026-07, develop @ 24bd5bc)

- **DD paths are opaque strings through the whole C ABI** (`al_read_data` /
  `al_write_data`, `include/al_lowlevel.h:487-489`). The caller (HLI) drives
  tree traversal node-by-node and owns all DD semantics. The core has no
  per-DD-version logic.
- **The compile-time DD version is deprecated on purpose**:
  `include/al_defs.h.in:57` sets `DD_VERSION "!!DEPRECATED!!"` and
  `python/tests/test_imasdef.py` asserts it stays that way. Do not reintroduce
  compile-time DD coupling anywhere.
- **Backend coupling status:** HDF5, memory, ASCII, and flexbuffers are fully
  DD-agnostic. Only two backends are DD-coupled:
  - **MDSplus** — consumes a binary model tree baked from one `IDSDef.xml` at
    build time (`models/mdsplus/`, `common/cmake/ALBuildDataDictionary.cmake`),
    selected at runtime via `MDSPLUS_MODELS_PATH`; DD version stored in tree
    node `VERSION:DATA_DICT` (`src/mdsplus/mdsplus_backend.cpp:5377-5389`).
  - **UDA** — loads `IDSDef.xml` at runtime from `$IDSDEF_PATH` and walks it
    for types/ranks/timebases (`src/uda/uda_xml.cpp:73-280`).
- The DD version of stored data lives only in the ordinary string field
  `ids_properties/version_put/data_dictionary`, written by the HLI; no backend
  except UDA interprets any DD version.
- **HDF5 layout:** master file + per-IDS files; one "tensorized" dataset per
  leaf DD path (`/` → `&` in names), AOS flattened with a leading index
  dimension plus `_SHAPE` companion datasets. HDF5 stores a backend-format
  version (1.0), not the DD version.
- **Performance shape:** the per-node `Backend` contract is the bottleneck; no
  general subtree/bulk read exists (`al_begin_global_action`'s `datapath` arg
  is ignored by HDF5, MDSplus, Memory, ASCII, and Flexbuffers). **UDA is the
  one living exception**: in remote mode with `cache_mode=ids`, `UDABackend`
  reads `datapath` to scope which paths `populate_cache()` requests from the
  server (`src/uda/uda_backend.cpp:1016-1031`, characterized in
  `tests/contract/test_uda_unique_surface.cpp`, issue #26). Slice mode
  disables HDF5 buffering
  (`src/hdf5/hdf5_dataset_handler.cpp:733`). Every read/write calls
  `getenv("IMAS_AL_ENABLE_PLUGINS")` (`src/al_lowlevel.cpp:108`). Python reads
  are zero-copy; Python writes copy twice (`_al_lowlevel.pyx:615-631`).
- **No benchmarks exist in-repo**; C++ tests are frameworkless executables in
  `tests/`, Python tests are pytest in `python/tests` (configured in
  `pyproject.toml`).

## DD version landscape (from the imas-dd tool suite)

- 35 DD versions in circulation, 3.22.0 → 4.1.1. Within-major changes are
  almost purely additive (4.0.0→4.1.1: zero breaking); the 3→4 major boundary
  is the cliff (894 breaking changes, 81 IDSs, COCOS 11→17).
- Equilibrium example: 585 paths (DD3) → 641 (DD4); the 3→4 transition has 143
  rename chains (`j_tor→j_phi`, `bpol_probe→b_field_pol_probe`,
  `psi_axis→psi_magnetic_axis`, …) **and 32 COCOS sign flips** — so
  cross-version mapping requires value transforms, not just path renames.
  Getting a sign flip wrong silently corrupts physics; strict mapping is the
  default in the north star design.

## Operating patterns for agents

- **Use the `imas-dd` MCP tools** for anything DD-related; never guess paths
  or version behaviour. Most useful: `get_dd_versions`,
  `get_ids_summary(ids, dd_version=3|4)`,
  `get_dd_migration_guide(from, to, ids_filter=..., summary_only=True)` (full
  output can be ~300 k chars — start with `summary_only` and grep the saved
  overflow file), `get_dd_version_context(change_type_filter='path_renamed',
  follow_rename_chains=True)` for rename lineages, `check_dd_paths` to
  validate paths.
- `src/uda/` line counts are misleading: it is mostly vendored pugixml.
  Vendored code also in `src/flatbuffers/`. Effective own-code base is ~28 k
  LOC.
- Build: CMake ≥3.21 (`cmake -B build && cmake --build build`); deps via vcpkg
  or system (HDF5, Boost::filesystem; MDSplus/UDA backends optional, OFF by
  default). Python wheel via scikit-build-core (`pip install .`); MDSplus
  models build (`AL_BUILD_MDSPLUS_MODELS=ON`) is the only build path that
  downloads the DD.
- Tests: `ctest` in the build dir for C++; `pytest` (rooted at `python/tests`)
  for Python.
- Default branch and PR target is `develop`.

## North star migration — where to contribute

Phased plan, effort, and decided design points are in `NORTH_STAR.md` §9/§11.
Standing decisions to respect: keep the C ABI stable and additive; store data
as-written and convert on read; version knowledge ships as data artifacts
(schema pack + map artifacts), never compiled in; equilibrium is the pilot
IDS; TDD — every phase starts with its tests (characterization tests and the
benchmark harness are Phase 0 and gate everything else).

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
