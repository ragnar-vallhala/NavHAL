#!/usr/bin/env bash
# Builds every portable sample under the AVR (ATmega328P) configuration.
# Catches AVR-port regressions and verifies the portable-tier contract:
# every sample in samples/portable/ must compile on both Cortex-M4 and AVR.
# Samples under samples/cortex-m/ are skipped (arch-specific by design).
#
# Uses cmake/toolchains/avr-toolchain.cmake — that file points at the AVR
# defconfig (cmake/defconfigs/avr_atmega328p.defconfig) which the top-level
# CMakeLists.txt seeds into .config when none exists. No sed/Kconfig
# patching here.
#
# Requires:  avr-gcc, avr-libc, binutils-avr.
#
# Usage:  tools/build_all_avr_samples.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

command -v avr-gcc >/dev/null || {
  echo "error: avr-gcc not on PATH (apt install gcc-avr avr-libc binutils-avr)" >&2
  exit 2
}

SAVED_CONFIG=""
BUILD=build-avr-sample

cleanup() {
  if [ -n "$SAVED_CONFIG" ] && [ -f "$SAVED_CONFIG" ]; then
    mv -f "$SAVED_CONFIG" .config
  else
    rm -f .config
  fi
  rm -rf "$BUILD"
}
trap cleanup EXIT

# Stash any existing .config so the toolchain file's defconfig seeds a fresh
# one for the AVR build. Restored on exit.
if [ -f .config ]; then
  SAVED_CONFIG=$(mktemp)
  mv .config "$SAVED_CONFIG"
fi

TOOLCHAIN="cmake/toolchains/avr-toolchain.cmake"

# Portable samples: directory names are "NN_<slug>"; -DSAMPLE takes the slug.
SAMPLES=$(ls samples/portable/ | sed 's/^[0-9]\+_//' | sort -u)
[ -n "$SAMPLES" ] || { echo "no portable samples found" >&2; exit 2; }

PASS=0
FAIL=0
FAIL_LIST=""

for sample in $SAMPLES; do
  rm -rf "$BUILD" .config
  if cmake -B "$BUILD" \
           -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
           -DSAMPLE="$sample" >/dev/null 2>&1 \
     && cmake --build "$BUILD" -j >/dev/null 2>&1; then
    printf '  \033[32mOK\033[0m   %s\n' "$sample"
    PASS=$((PASS+1))
  else
    printf '  \033[31mFAIL\033[0m %s\n' "$sample"
    FAIL=$((FAIL+1))
    FAIL_LIST="$FAIL_LIST $sample"
  fi
done

echo
echo "==== AVR portable-sample summary ===="
printf 'pass=%d fail=%d\n' "$PASS" "$FAIL"
if [ "$FAIL" -gt 0 ]; then
  echo "Failed:$FAIL_LIST" >&2
  echo "Re-run a single failure with:" >&2
  echo "  rm -f .config && cmake -B build-X -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN -DSAMPLE=<name> && cmake --build build-X -j" >&2
  exit 1
fi
