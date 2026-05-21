#!/usr/bin/env bash
# Build and run the host-runnable NavHAL test subset.
#
# Pure-logic tests (hal_status, conversion, software CRC, GPIO pin
# encoding) compiled with the system gcc — no embedded toolchain
# needed. Suitable for CI and for a quick local sanity check.
#
# Usage:  tools/run_host_tests.sh [build-dir]
#
# Default build-dir is `build-host` at the repo root.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${1:-${REPO_ROOT}/build-host}"

echo ">> configuring  $BUILD_DIR"
cmake -B "$BUILD_DIR" -S "$REPO_ROOT/tests/host" >/dev/null

echo ">> building"
cmake --build "$BUILD_DIR" -j >/dev/null

echo ">> running"
"$BUILD_DIR/tests_host"
