#!/usr/bin/env bash
# Install Renode from the upstream .deb. Renode isn't packaged in
# stable Ubuntu, so we fetch the release tarball directly. Used by
# the Nucleo-F401RE PIL setup (tools/pil/boards/nucleo_f401re.conf
# points at this script via SETUP_SCRIPT=).

set -euo pipefail

RENODE_VERSION=${RENODE_VERSION:-1.16.1}
RENODE_DEB="renode_${RENODE_VERSION}_amd64.deb"

cd "$(mktemp -d)"
echo ">> downloading Renode ${RENODE_VERSION}"
curl -sSL -o "${RENODE_DEB}" \
  "https://github.com/renode/renode/releases/download/v${RENODE_VERSION}/${RENODE_DEB}"
sudo apt-get install -y "./${RENODE_DEB}"
renode --version
