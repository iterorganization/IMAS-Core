# MDSplus characterization environment (issue #13, tracer bullet for #12).
#
# aarch64 (native on Apple Silicon) image that bakes a DD-4.1.1 MDSplus model
# tree and builds the contract suite with AL_BACKEND_MDSPLUS=ON. See
# docker/mdsplus/README.md for the characterization-discovered deviation from
# the issue's ubuntu:22.04 starting point (no arm64 MDSplus .debs exist for
# Ubuntu 22; Ubuntu 24 is the first LTS with an arm64 repo) and for the run
# recipe.
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# --- MDSplus apt repo -------------------------------------------------------
# The repo key is an ASCII-armored PGP block; apt's signed-by wants a binary
# keyring, hence the dearmor step. Plain http (not https): www.mdsplus.org's
# TLS chain is missing the InCommon intermediate cert, so a fresh container's
# default trust store cannot verify it (characterization-discovered; a
# browser/host with a cached intermediate does not hit this). The repo
# content is still authenticated via the dearmored signed-by key below.
RUN apt-get update && apt-get install -y --no-install-recommends \
      ca-certificates gnupg wget \
    && mkdir -p /etc/apt/keyrings \
    && wget -qO- http://www.mdsplus.org/dist/mdsplus.gpg.key \
       | gpg --dearmor -o /etc/apt/keyrings/mdsplus.gpg \
    && echo "deb [signed-by=/etc/apt/keyrings/mdsplus.gpg] http://www.mdsplus.org/dist/Ubuntu24/repo MDSplus stable" \
       > /etc/apt/sources.list.d/mdsplus.list \
    && apt-get update

# --- Toolchain + MDSplus + DD-bake dependencies -----------------------------
# mdsplus-devel: mdslib.h + link libs (common/cmake/FindMDSplus.cmake).
# mdsplus-java: jTraverser.jar (models/mdsplus/CMakeLists.txt CompileTree) +
#   pulls a java-runtime.
# python3/venv: ALBuildDataDictionary.cmake's saxonche venv + pip-installs
#   imas_data_dictionary==4.1.1 from PyPI (no DD git clone needed).
RUN apt-get install -y --no-install-recommends \
      mdsplus mdsplus-devel mdsplus-java \
      build-essential cmake git ninja-build pkg-config \
      libboost-filesystem-dev libhdf5-dev \
      python3 python3-venv python3-pip python3-dev \
      openjdk-21-jdk-headless \
    && rm -rf /var/lib/apt/lists/*

ENV MDSPLUS_DIR=/usr/local/mdsplus
ENV PATH="${MDSPLUS_DIR}/bin:${PATH}"
ENV LD_LIBRARY_PATH="${MDSPLUS_DIR}/lib:${LD_LIBRARY_PATH}"
ENV CLASSPATH="${MDSPLUS_DIR}/java/classes/jTraverser.jar"

WORKDIR /workspace
