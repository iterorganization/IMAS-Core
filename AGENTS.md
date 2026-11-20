# AGENTS.md - IMAS-Core

Guidance for AI coding assistants working on this repository, and a useful
summary for human contributors too. Most changes here are still written by
developers directly; assistants support that work and follow the same rules.

IMAS-Core is the low-level IMAS Access Layer (AL core): a stable C ABI over a
C++17 core, with pluggable storage backends and Cython/Python bindings. It
reads and writes IMAS IDS data addressed by Data Dictionary (DD) paths, but it
is DD-version-agnostic: DD semantics belong to the high-level interfaces
(HLIs) and the DD tooling, not to this repository.

Start with `README.md`, `CONTRIBUTING.md` and the developer documentation in
`docs/source/developers/`.

## Repository layout

- `include/`: public C ABI (`al_lowlevel.h`, `al_defs.h.in`, ...).
- `src/`: C++ core (`al_lowlevel.cpp`, `al_context.cpp`, `al_backend.cpp`,
  plugin manager) and the backends: `hdf5/`, `mdsplus/`, `uda/`,
  `memory_backend.*`, `ascii_backend.*`, `flexbuffers_backend.*`.
- `python/`: Cython bindings (`imas_core/`) and pytest suite (`tests/`).
- `tests/`: C/C++ low-level ABI smoke tests.
- `models/mdsplus/`, `common/cmake/`: MDSplus model generation and shared
  CMake logic.
- `docs/`: Sphinx documentation.
- Vendored code: `src/flatbuffers/` and most of `src/uda/` (pugixml). Don't
  edit or reformat it, and don't count it when sizing the code base.

## Architecture constraints

- **DD paths are opaque strings through the whole C ABI** (`al_read_data`,
  `al_write_data` in `include/al_lowlevel.h`). The caller drives the tree
  traversal node by node and owns all DD semantics. The core has no
  per-DD-version logic.
- **The compile-time DD version is deprecated on purpose**: `DD_VERSION` is
  `"!!DEPRECATED!!"` in `include/al_defs.h.in`, and
  `python/tests/test_imasdef.py` asserts it. Don't reintroduce compile-time DD
  coupling.
- **Backend DD coupling:** HDF5, memory, ASCII and flexbuffers are fully
  DD-agnostic. Only two backends depend on a DD version:
  - MDSplus uses a model tree built from one `IDSDef.xml` at build time
    (`models/mdsplus/`), selected at runtime with `MDSPLUS_MODELS_PATH`.
  - UDA loads `IDSDef.xml` at runtime from `$IDSDEF_PATH` (`src/uda/uda_xml.cpp`).
- The DD version of stored data is only the string field
  `ids_properties/version_put/data_dictionary`, written by the HLI.
- **The C ABI is a public contract** used by several HLIs. Keep it stable:
  changes must be backward-compatible and additive.
- **On-disk formats are contracts too.** HDF5 uses a master file plus one file
  per IDS, with one "tensorized" dataset per leaf path (`/` becomes `&` in
  names). Arrays of structures are flattened with a leading index dimension
  and `_SHAPE` companion datasets. HDF5 stores a backend format version, not a
  DD version. A layout change can make existing data unreadable.
- **Performance shape:** the per-node `Backend` interface is the main cost,
  and there is no general subtree/bulk read. Every read and write checks
  `IMAS_AL_ENABLE_PLUGINS`. Python reads are zero-copy; Python writes copy the
  data.

## Build and test

- CMake ≥ 3.21: `cmake -B build && cmake --build build -j2`. Dependencies come
  from vcpkg or the system (HDF5, Boost). Main options: `AL_BACKEND_HDF5` (ON),
  `AL_BACKEND_MDSPLUS`, `AL_BACKEND_UDA`, `AL_PYTHON_BINDINGS`,
  `AL_BUILD_TESTS` (all OFF by default). `AL_BUILD_MDSPLUS_MODELS=ON` is the
  only build path that downloads the DD.
- Python wheel: `pip install .` (scikit-build-core).
- Tests: `ctest` in the build directory for C/C++ (configure with
  `-DAL_BUILD_TESTS=ON`); `pytest` for Python (`testpaths` = `python/tests`).
- Builds are often run on shared machines: keep parallel jobs low (`-j2`)
  unless the developer asks for more.

## Contribution rules (humans and assistants)

- Follow `CONTRIBUTING.md`: discuss the change in an issue on
  `iterorganization/IMAS-Core`, branch off `develop`, and open the pull request
  against `develop`.
- Keep pull requests small and single-purpose, so a human can review them. Don't
  mix a bug fix with refactoring, reformatting or new test infrastructure.
  Explain *why* in commit and PR messages, not only what.
- Every bug fix or behaviour change comes with a test that fails before the
  change and passes after it. Run `ctest` and `pytest` locally before pushing,
  and keep `tests/` building.
- Don't commit build outputs, local benchmark results, IDE settings, personal
  files or tool caches.
- The developer who submits a change is responsible for it and must understand
  it, whatever produced the code. Mark AI-assisted commits with an
  `Assisted-by: <tool>/<model>` trailer, not with `Co-Authored-By`.
- Never guess DD paths or DD version behaviour. Check the DD documentation, or
  use the `imas-dd` MCP server if it is available.

## Maintaining this file

- Name files, functions and symbols, not `file:line` locations, which go stale.
- Only reference files that exist in this repository, and keep the content
  generic: no personal workflows, forks or tool setups.
