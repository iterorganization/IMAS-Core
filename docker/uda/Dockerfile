# UDA characterization environment (issue #23, feasibility spike for #21).
#
# aarch64 (native on Apple Silicon / GitHub arm64 runners) image standing up the
# pinned UDA reference stack for the contract suite's UDA tier. Mirrors the
# MDSplus leg's philosophy (docker/mdsplus/): the image is the *toolchain +
# pinned reference stack*, and the IMAS-Core client + the IMAS server plugin are
# built at run time against the mounted workspace, so the tests always exercise
# the source under review (never a stale baked-in core). See docker/uda/README.md
# for the run recipe, the gate decision, and the empirically-answered open items.
#
# Pinned stack identifiers (recorded for TRACEABILITY Part 5):
#   - UDA server ....... ukaea/uda            tag 2.9.3
#   - IMAS UDA plugin .. iterorganization/UDA-Plugins  tag 1.8.0
#   - Data Dictionary .. IDSDef.xml           DD 4.1.1
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Pinned stack versions (single source of truth; also surfaced as image labels).
ARG UDA_VERSION=2.9.3
ARG UDA_PLUGINS_VERSION=1.8.0
ARG DD_VERSION=4.1.1

LABEL org.iter.imas-core.uda-server-version="2.9.3"
LABEL org.iter.imas-core.uda-plugins-version="1.8.0"
LABEL org.iter.imas-core.dd-version="4.1.1"

# --- Toolchain + UDA server + IMAS-Core + plugin build/runtime dependencies ---
# UDA server 2.9.3 (docker/client.*.dockerfile + .github/workflows/cmake.yml):
#   boost(+program_options), openssl, libxml2, fmt, spdlog, capnproto, tirpc.
# IMAS-Core (this repo, same set the mdsplus image uses): HDF5, Boost::filesystem.
# IMAS plugin (UDA-Plugins/source/imas): CURL, Boost::filesystem/system.
# xinetd fronts the per-connection uda_server on a TCP port (simplest in a
# container: no systemd/PID-1 dance).
RUN apt-get update && apt-get install -y --no-install-recommends \
      git build-essential cmake ninja-build pkg-config ca-certificates wget \
      libboost-dev libboost-program-options-dev libboost-filesystem-dev \
      libboost-system-dev \
      libssl-dev libxml2-dev libfmt-dev libspdlog-dev \
      capnproto libcapnp-dev libtirpc-dev \
      libhdf5-dev libcurl4-openssl-dev \
      libgtest-dev \
      python3 python3-venv python3-pip \
      xinetd \
    && rm -rf /var/lib/apt/lists/*

# Cap'n Proto's Ubuntu 24.04 CMake config has a broken WITH_LIBATOMIC guard
# (ukaea/uda .github/workflows/cmake.yml patches the same file; the arm64 path
# is aarch64-linux-gnu). Harmless no-op if the line is ever fixed upstream.
RUN sed -i 's/if (yes)  # WITH_LIBATOMIC/if (no)  # WITH_LIBATOMIC/' \
      /usr/lib/aarch64-linux-gnu/cmake/CapnProto/CapnProtoConfig.cmake || true

# --- UDA server (ukaea/uda, pinned) ----------------------------------------
# Full build (CLIENT_ONLY=OFF) so we get both the uda-cpp client the IMAS-Core
# UDA backend links AND the uda_server binary + install_plugin. SSL auth OFF to
# keep the reference stack self-contained (no cert provisioning); the contract
# suite characterizes data access, not transport security. Installs to
# /usr/local: uda_server, uda_cli, install_plugin, libuda_*.so, uda-cpp.pc.
RUN git clone --branch "${UDA_VERSION}" --depth 1 \
      https://github.com/ukaea/uda.git /opt/uda-src \
    && cmake -G Ninja -S /opt/uda-src -B /opt/uda-src/build \
         -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release \
         -DCLIENT_ONLY=OFF -DSSLAUTHENTICATION=OFF -DENABLE_CAPNP=ON \
         -DCMAKE_INSTALL_PREFIX=/usr/local \
    && cmake --build /opt/uda-src/build -j"$(nproc)" \
    && cmake --install /opt/uda-src/build \
    && rm -rf /opt/uda-src/build

ENV LD_LIBRARY_PATH="/usr/local/lib:/usr/local/lib/plugins"
ENV PATH="/usr/local/bin:${PATH}"

# --- Data Dictionary IDSDef.xml (DD 4.1.1) ---------------------------------
# Sourced from the official imas-data-dictionary PyPI wheel (the same source the
# repo's own ALBuildDataDictionary.cmake uses), NOT by baking a MDSplus model
# tree — AL_BUILD_MDSPLUS_MODELS is deliberately not enabled here. The DD's
# GitHub releases carry no IDSDef.xml asset (it is a generated artifact), so a
# direct release download is not an option; the wheel ships a prebuilt IDSDef.xml
# whose path the packaged `idsinfo idspath` reports. The IMAS-Core UDA backend
# walks this file at run time; its lookup order is $IDSDEF_PATH first, else
# $IMAS_PREFIX/include/IDSDef.xml (src/uda/uda_xml.cpp load_xml()).
RUN python3 -m venv /opt/ddvenv \
    && /opt/ddvenv/bin/pip install --no-cache-dir "imas-data-dictionary==${DD_VERSION}" \
    && cp "$(/opt/ddvenv/bin/idsinfo idspath)" /opt/IDSDef.xml \
    && test -s /opt/IDSDef.xml \
    && grep -q "<version>${DD_VERSION}</version>" /opt/IDSDef.xml
ENV IDSDEF_PATH="/opt/IDSDef.xml"

# --- IMAS UDA server plugin source (iterorganization/UDA-Plugins, pinned) ---
# Cloned (not tarball) so the plugin's git_describe-derived version resolves to
# the tag and the plugin reports 1.8.0 at run time. The plugin is *built* at run
# time (docker/uda/run.sh) because it must link the workspace's IMAS-Core
# (al-core.pc) — the subject under test — and must NOT fall back to the
# mapping-only NO_IMAS degradation.
RUN git clone --branch "${UDA_PLUGINS_VERSION}" --depth 1 \
      https://github.com/iterorganization/UDA-Plugins.git /opt/uda-plugins-src

COPY run.sh /usr/local/bin/uda-run.sh
RUN chmod +x /usr/local/bin/uda-run.sh

WORKDIR /workspace
