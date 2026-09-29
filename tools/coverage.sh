#!/usr/bin/env bash
# Copyright (C) 2025 NAVRobotec Pvt Ltd
# Author: Ragnar Vallhala
# SPDX-License-Identifier: Apache-2.0
#
# Host-tier line coverage over src/: build the SIL suites with --coverage, run
# them, and summarise the .gcda with gcovr.
#
# Usage:
#   tools/coverage.sh                  # report only
#   tools/coverage.sh --min-lines 55   # and fail below 55% lines
#
# Report-only by default: the number is informational until someone decides to
# ratchet it, and a floor nobody chose would either be met trivially or block
# the release gate on the day it is introduced.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

BUILD_DIR=build-host-cov
MIN_LINES=""

while [ $# -gt 0 ]; do
  case "$1" in
    --min-lines) MIN_LINES="${2:-}"; shift 2 ;;
    -h|--help) sed -n '6,13p' "$0"; exit 0 ;;
    *) echo "error: unknown flag '$1'" >&2; exit 2 ;;
  esac
done

command -v gcovr >/dev/null 2>&1 || {
  echo "error: gcovr not installed (pip install gcovr)" >&2
  exit 2
}

rm -rf "$BUILD_DIR"
cmake -B "$BUILD_DIR" -S tests/host \
  -DCMAKE_C_FLAGS="--coverage -O0 -g" \
  -DCMAKE_EXE_LINKER_FLAGS="--coverage" >/dev/null
cmake --build "$BUILD_DIR" -j >/dev/null

# The suites are what produce the .gcda; a failing case still leaves coverage
# behind, and this script reports coverage rather than re-judging the suites.
for exe in tests_host tests_host_drivers; do
  [ -x "$BUILD_DIR/$exe" ] || { echo "error: $exe not built" >&2; exit 1; }
  "$BUILD_DIR/$exe" >/dev/null || true
done

rc=0
gcovr --root . --filter src/ --print-summary --decisions \
  ${MIN_LINES:+--fail-under-line "$MIN_LINES"} "$BUILD_DIR" || rc=$?
rm -rf "$BUILD_DIR"
exit "$rc"
