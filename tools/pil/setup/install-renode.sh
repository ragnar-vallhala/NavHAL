#!/usr/bin/env bash
# Install Renode from the upstream portable tarball, which bundles its own
# .NET runtime: the .deb depends on dotnet-runtime-8.0, which the CI runners
# can't install. Used by the Renode PIL boards (tools/pil/boards/nucleo_*.conf
# point at this script via SETUP_SCRIPT=).

set -euo pipefail

RENODE_VERSION=${RENODE_VERSION:-1.16.1}
RENODE_TGZ="renode-${RENODE_VERSION}.linux-portable-dotnet.tar.gz"
PREFIX=${RENODE_PREFIX:-/opt/renode}

cd "$(mktemp -d)"
echo ">> downloading Renode ${RENODE_VERSION}"
curl -sSL -o "${RENODE_TGZ}" \
  "https://github.com/renode/renode/releases/download/v${RENODE_VERSION}/${RENODE_TGZ}"
sudo mkdir -p "${PREFIX}"
sudo tar -xzf "${RENODE_TGZ}" -C "${PREFIX}" --strip-components=1
sudo ln -sf "${PREFIX}/renode" /usr/local/bin/renode
renode --version
