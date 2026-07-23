#!/usr/bin/env bash
# One-shot: configure + build the x86-64 sample, then boot it in QEMU.
#
#   tools/qemu/build_run.sh [sample]      # default sample: hal_x86_hello
#
# Env overrides: BUILD_DIR (default build-x86).
#
# NavHAL's Kconfig uses a single root .config as source of truth, and the
# toolchain only seeds the x86 defconfig when .config is absent — so we remove
# .config first to guarantee a clean x86 configure (this reconfigures the tree
# for x86, same as switching to any other target).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SAMPLE="${1:-hal_x86_hello}"
BUILD="${BUILD_DIR:-$ROOT/build-x86}"

cd "$ROOT"
rm -f .config
cmake -S "$ROOT" -B "$BUILD" \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-qemu-toolchain.cmake \
  -DSAMPLE="$SAMPLE" -G Ninja
cmake --build "$BUILD"

ELF="$(find "$BUILD/samples" -name "$SAMPLE.elf" -print -quit)"
[ -n "$ELF" ] || { echo "error: $SAMPLE.elf not found under $BUILD/samples" >&2; exit 1; }

exec "$ROOT/tools/qemu/run.sh" "$ELF"
