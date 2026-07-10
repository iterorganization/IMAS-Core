# MDSplus characterization environment (issue #13)

aarch64 image standing up a real, runnable MDSplus for the contract suite's
MDSplus tier (`tests/contract/test_mdsplus.cpp`, `TRACEABILITY.md` Part 4).

## Deviation from the issue's starting point

Issue #12/#13 specified `FROM ubuntu:22.04`. Empirically, MDSplus does not
publish arm64 `.deb` packages for Ubuntu 22 (or 20): its
`https://www.mdsplus.org/dist/Ubuntu22/repo/dists/MDSplus/stable/binary-arm64/Packages`
is a 0-byte file. Ubuntu 24.04 is the first LTS with a populated arm64
repo, so this image uses `ubuntu:24.04` instead — still native aarch64 (no
emulation), which is what the issue's Apple-Silicon developer story (#12
user story 8) actually needs. The issue's stated amd64-emulation fallback is
unnecessary as a result.

## Build

```sh
docker build --platform linux/arm64 -f docker/mdsplus/Dockerfile \
  -t imas-core-mdsplus:dev docker/mdsplus
```

## Bake the DD-4.1.1 model tree and build the contract suite

Run from the repository root, mounting it into the container:

```sh
docker run --rm --platform linux/arm64 \
  -v "$PWD":/workspace -w /workspace \
  imas-core-mdsplus:dev bash -c '
    cmake -B build-mdsplus -G Ninja \
      -DAL_BACKEND_MDSPLUS=ON \
      -DAL_BUILD_MDSPLUS_MODELS=ON \
      -DAL_BUILD_TESTS=ON \
      -DDD_VERSION=4.1.1 \
      -DCMAKE_BUILD_TYPE=Release &&
    cmake --build build-mdsplus -j$(nproc)
  '
```

`-DDD_VERSION=4.1.1` with the default `AL_DOWNLOAD_DEPENDENCIES=ON` makes
`common/cmake/ALBuildDataDictionary.cmake` `pip install
imas_data_dictionary==4.1.1` from PyPI — no data-dictionary git clone needed.
The model tree (`ids_model.tree`/`.datafile`/`.characteristics`) lands under
`build-mdsplus/models/mdsplus/`.

## Run the MDSplus tests

```sh
docker run --rm --platform linux/arm64 \
  -v "$PWD":/workspace -w /workspace \
  -e MDSPLUS_MODELS_PATH=/workspace/build-mdsplus/models/mdsplus \
  imas-core-mdsplus:dev bash -c \
    'cd build-mdsplus/tests/contract && ctest -L mdsplus --output-on-failure'
```

Omitting `MDSPLUS_MODELS_PATH` makes the same tests report `Skipped` rather
than failing (`GTEST_SKIP()`, TEST_STRATEGY.md D4).
