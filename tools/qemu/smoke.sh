#!/usr/bin/env bash
# CI smoke test: build an x86 sample, boot it headless in QEMU, and assert an
# expected string appears on COM1. Non-interactive; exits non-zero on failure.
#
#   smoke.sh <sample> <expected-substring> [timeout-secs]
#
# Example:
#   smoke.sh hal_x86_hello "Hello from NavHAL on x86-64 (QEMU)!"
#
# Reconfigures the tree for x86 (rewrites the root .config, like any target
# switch) — intended for CI / throwaway checkouts.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SAMPLE="${1:?usage: smoke.sh <sample> <expected> [timeout]}"
EXPECT="${2:?usage: smoke.sh <sample> <expected> [timeout]}"
TMO="${3:-30}"
BUILD="${BUILD_DIR:-$ROOT/build-x86-smoke}"

# Accept a sample directory name too (strip path + leading NN_).
SAMPLE="$(printf '%s' "$SAMPLE" | sed -E 's#.*/##; s/^[0-9]+_//')"

cd "$ROOT"
rm -f .config
cmake -S "$ROOT" -B "$BUILD" \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-qemu-toolchain.cmake \
  -DSAMPLE="$SAMPLE" -G Ninja >/dev/null
cmake --build "$BUILD" >/dev/null

ELF="$(find "$BUILD/samples" -name "$SAMPLE.elf" -print -quit)"
[ -n "$ELF" ] || { echo "smoke: $SAMPLE.elf not found" >&2; exit 2; }
"$ROOT/tools/qemu/run.sh" "$ELF" --iso-only >/dev/null
ISO="${ELF%.elf}.iso"

SER="$(mktemp)"
QPID=""
cleanup() { [ -n "$QPID" ] && kill "$QPID" 2>/dev/null; wait "$QPID" 2>/dev/null; rm -f "$SER"; }
trap cleanup EXIT

# The kernel hlt-loops forever, so run QEMU in the background and stop it as soon
# as the expected line appears (or after the timeout).
qemu-system-x86_64 -cdrom "$ISO" -serial "file:$SER" -display none -no-reboot \
  >/dev/null 2>&1 &
QPID=$!

found=0
for ((i = 0; i < TMO * 10; i++)); do
  if grep -qF -- "$EXPECT" "$SER" 2>/dev/null; then found=1; break; fi
  kill -0 "$QPID" 2>/dev/null || break # QEMU exited early
  sleep 0.1
done

echo "--- $SAMPLE serial output ---"
cat "$SER"
echo "-----------------------------"
if [ "$found" = 1 ]; then
  echo "PASS [$SAMPLE]: found '$EXPECT'"
else
  echo "FAIL [$SAMPLE]: expected '$EXPECT' not in serial output" >&2
  exit 1
fi
