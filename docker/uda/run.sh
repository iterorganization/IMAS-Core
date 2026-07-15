#!/usr/bin/env bash
# docker/uda/run.sh — assemble the UDA reference stack against the mounted
# workspace and drive the UDA contract tier (issue #23, PRD #21).
#
# The image (docker/uda/Dockerfile) ships the pinned, source-independent layers:
# UDA server 2.9.3, the UDA-Plugins 1.8.0 source, IDSDef.xml (DD 4.1.1), xinetd.
# This script does the source-dependent assembly at run time so the tests always
# exercise the IMAS-Core under review (never a stale baked-in core), mirroring
# the MDSplus leg's build-workspace-at-runtime philosophy:
#   1. build + install IMAS-Core (the client under test) with the HDF5 and UDA
#      backends, from /workspace;
#   2. build + install + register the `IMAS` server plugin against *that*
#      IMAS-Core (al-core.pc) — asserting it is NOT the mapping-only NO_IMAS
#      degradation the PRD forbids;
#   3. start the UDA server (xinetd fronts uda_server on TCP 56565);
#   4. run the UDA contract suite (`ctest -L uda`).
#
# Any extra CLI args are treated as the ctest invocation to run instead of the
# default, e.g. `uda-run.sh ctest -L uda -R Smoke --output-on-failure` or
# `uda-run.sh bash` for an interactive shell after the stack is up.
set -euo pipefail

WORKSPACE=${WORKSPACE:-/workspace}
PREFIX=/usr/local
BUILD_DIR=${BUILD_DIR:-${WORKSPACE}/build-uda}
PLUGINS_SRC=/opt/uda-plugins-src

export PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
export CMAKE_PREFIX_PATH="${PREFIX}:${CMAKE_PREFIX_PATH:-}"
export LD_LIBRARY_PATH="${PREFIX}/lib:${PREFIX}/lib/plugins:${LD_LIBRARY_PATH:-}"

echo "==> [1/4] Building IMAS-Core (client under test) with HDF5 + UDA backends"
cmake -S "${WORKSPACE}" -B "${BUILD_DIR}" -G Ninja \
  -DAL_BACKEND_HDF5=ON -DAL_BACKEND_UDA=ON -DAL_BUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${PREFIX}"
cmake --build "${BUILD_DIR}" -j"$(nproc)"
cmake --install "${BUILD_DIR}"
# al-core.pc is the seam the plugin's FindIMAS.cmake consumes; fail early if the
# install didn't produce it (otherwise the plugin would silently go NO_IMAS).
pkg-config --exists al-core \
  || { echo "FATAL: al-core.pc not installed — plugin would degrade to NO_IMAS"; exit 1; }

echo "==> [2/4] Building + registering the IMAS server plugin against IMAS-Core"
# Configure output is captured so we can prove IMAS was found (not NO_IMAS): the
# plugin's source/imas/CMakeLists.txt prints 'IMAS not found - building for
# MAPPING only' and appends -DNO_IMAS when al-core is absent. The PRD forbids
# that degradation, so we treat it as a hard failure.
cfg_log=$(mktemp)
cmake -S "${PLUGINS_SRC}" -B "${PLUGINS_SRC}/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
  -DBUILD_PLUGINS="imas" 2>&1 | tee "${cfg_log}"
if grep -qi "building for MAPPING only" "${cfg_log}"; then
  echo "FATAL: IMAS plugin configured as NO_IMAS (mapping-only) — not acceptable (PRD #21)."
  exit 1
fi
cmake --build "${PLUGINS_SRC}/build" -j"$(nproc)"
cmake --install "${PLUGINS_SRC}/build"
test -e "${PREFIX}/lib/plugins/libimas_plugin.so" \
  || { echo "FATAL: IMAS plugin .so not installed"; exit 1; }
"${PREFIX}/bin/install_plugin" -u "${PREFIX}" install IMAS
echo "--- registered plugins (udaPlugins.conf) ---"
cat "${PREFIX}/etc/plugins/udaPlugins.conf"

echo "==> [3/4] Starting the UDA server (xinetd on TCP 56565)"
# udaserver.cfg ships without UDA_ALLOWED_PATHS (which defaults to blocking all
# file access) and without IDSDEF_PATH; append both. UDA_ALLOWED_PATHS=/ is safe
# in this isolated single-purpose reference container (documented in README).
# UDA_IMAS_MAPPINGS_FILE: the IMAS plugin's init unconditionally loads a
# machine-mapping table (source/imas/machine_mapping.h) and throws if the env
# var is unset -- even for remote (non-mapped) access like ours. The plugin
# ships a valid table; point at it so init succeeds. Our remote hdf5 path never
# consults the mapping, but the file must exist and parse (5 columns/line).
#
# The shipped udaserver.cfg already assigns UDA_ALLOWED_PATHS= (empty, which
# blocks all file access), so we cannot guard on that name -- we would match the
# shipped line and skip our whole block. Guard instead on UDA_IMAS_MAPPINGS_FILE
# (absent from the shipped cfg) and append at the end: udaserver.sh sources the
# file top-to-bottom, so our later UDA_ALLOWED_PATHS=/ overrides the empty one.
PLUGIN_MAPPINGS=${PLUGINS_SRC}/source/imas/mappings.txt
if ! grep -q UDA_IMAS_MAPPINGS_FILE "${PREFIX}/etc/udaserver.cfg"; then
  {
    echo "export UDA_ALLOWED_PATHS=/"
    echo "export IDSDEF_PATH=${IDSDEF_PATH:-/opt/IDSDef.xml}"
    echo "export UDA_IMAS_MAPPINGS_FILE=${PLUGIN_MAPPINGS}"
  } >> "${PREFIX}/etc/udaserver.cfg"
fi
# xinetd's shipped config leaves user= empty; make it explicit for a root
# container so xinetd doesn't refuse the service.
sed -i 's/^\( *user *= *\)$/\1root/' "${PREFIX}/etc/xinetd.conf"
xinetd -dontfork -f "${PREFIX}/etc/xinetd.conf" &
XINETD_PID=$!
# Wait for the listener before running the client.
for _ in $(seq 1 30); do
  if bash -c "exec 3<>/dev/tcp/localhost/56565" 2>/dev/null; then
    exec 3>&- 2>/dev/null || true
    break
  fi
  sleep 0.5
done
echo "UDA server listening on localhost:56565 (xinetd pid ${XINETD_PID})"
# Sanity ping through UDA's own CLI (independent of IMAS-Core) so a server-side
# failure is distinguishable from a client-side one.
UDA_HOST=localhost UDA_PORT=56565 "${PREFIX}/bin/uda_cli" \
  --host localhost --port 56565 --request "help::help()" >/dev/null 2>&1 \
  && echo "uda_cli help::help() OK" || echo "WARN: uda_cli ping failed (continuing)"

echo "==> [4/4] Running the UDA contract tier"
export UDA_HOST=localhost UDA_PORT=56565
if [ "$#" -gt 0 ]; then
  exec "$@"
fi
cd "${BUILD_DIR}/tests/contract"
ctest -L uda --output-on-failure
