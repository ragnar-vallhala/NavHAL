#!/usr/bin/env sh
# Copyright (C) 2025 NAVRobotec Pvt Ltd
# Author: Ragnar Vallhala
# SPDX-License-Identifier: Apache-2.0
#
# Run vtest over NavHAL's suites (vtest.conf), from anywhere in the tree.
#
#   tools/vtest.sh            interactive TUI
#   tools/vtest.sh --run      everything, batch
#   tools/vtest.sh --list     the catalog
#
# vtest is a system tool shared by every repo on the machine, not something this
# tree builds: install it once (docs/testing/vtest.md) and every repo uses the
# same binary. VTEST=path forces a specific one.
set -eu
cd "$(dirname "$0")/.."

VTEST="${VTEST:-vtest}"
if ! command -v "$VTEST" >/dev/null 2>&1; then
  echo "vtest not installed. Install it once:" >&2
  echo "  git clone https://github.com/ragnar-vallhala/vtest.git ~/src/vtest" >&2
  echo "  cmake -S ~/src/vtest -B ~/src/vtest/build" >&2
  echo "  cmake --build ~/src/vtest/build" >&2
  echo "  cmake --install ~/src/vtest/build --prefix ~/.local" >&2
  echo "See docs/testing/vtest.md." >&2
  exit 2
fi
exec "$VTEST" "$@"
