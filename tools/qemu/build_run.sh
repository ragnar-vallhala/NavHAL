#!/usr/bin/env bash
# One-shot: configure + build the x86-64 sample, then boot it in QEMU.
#
#   tools/qemu/build_run.sh [sample] [--window] [--iso-only]
#     default sample: hal_x86_hello
#     --window / --iso-only are forwarded to tools/qemu/run.sh (see its help).
#
# Env overrides: BUILD_DIR (default build-x86).
#
# NavHAL's Kconfig uses a single root .config as source of truth, and the
# toolchain only seeds the x86 defconfig when .config is absent — so we remove
# .config first to guarantee a clean x86 configure (this reconfigures the tree
# for x86, same as switching to any other target).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SAMPLE="hal_x86_hello"
RUN_FLAGS=()
for arg in "$@"; do
  case "$arg" in
    -*) RUN_FLAGS+=("$arg") ;;   # forward flags to run.sh
    *)  SAMPLE="$arg" ;;
  esac
done
BUILD="${BUILD_DIR:-$ROOT/build-x86}"

# Accept the sample directory name too: strip any path and a leading NN_ index
# so "02_hal_x86_interrupt" or "x86/02_hal_x86_interrupt" resolve to the
# registered slug "hal_x86_interrupt".
SAMPLE="$(printf '%s' "$SAMPLE" | sed -E 's#.*/##; s/^[0-9]+_//')"

cd "$ROOT"
rm -f .config
cmake -S "$ROOT" -B "$BUILD" \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-qemu-toolchain.cmake \
  -DSAMPLE="$SAMPLE" -G Ninja
cmake --build "$BUILD"

ELF="$(find "$BUILD/samples" -name "$SAMPLE.elf" -print -quit)"
[ -n "$ELF" ] || { echo "error: $SAMPLE.elf not found under $BUILD/samples" >&2; exit 1; }

exec "$ROOT/tools/qemu/run.sh" "$ELF" "${RUN_FLAGS[@]}"
