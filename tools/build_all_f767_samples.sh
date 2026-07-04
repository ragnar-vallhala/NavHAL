#!/usr/bin/env bash
# Builds every Cortex-M7-buildable sample under the STM32F767ZI configuration —
# the portable tier plus the cortex-m samples whose Kconfig arch gate admits the
# M7 (Ethernet, SDIO, DMA-backed I2C/UART, etc.). Samples gated to another arch
# (the M4-only no_hal / systick / clock demos) are skipped; see
# tools/samples_for_arch.sh. Catches F7-port regressions across the whole
# M7-capable sample set.
#
# Uses cmake/toolchains/arm-none-eabi-f767-toolchain.cmake — that file points at
# the F767 defconfig (cmake/defconfigs/cortex-m7_stm32f7_nucleo_f767zi.defconfig)
# which the top-level CMakeLists.txt seeds into .config when none exists.
#
# Requires:  arm-none-eabi-gcc, binutils-arm-none-eabi, libnewlib-arm-none-eabi.
#
# Usage:  tools/build_all_f767_samples.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

command -v arm-none-eabi-gcc >/dev/null || {
  echo "error: arm-none-eabi-gcc not on PATH (apt install gcc-arm-none-eabi)" >&2
  exit 2
}

SAVED_CONFIG=""
BUILD=build-f767-sample

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
# one for the F767 build. Restored on exit.
if [ -f .config ]; then
  SAVED_CONFIG=$(mktemp)
  mv .config "$SAVED_CONFIG"
fi

TOOLCHAIN="cmake/toolchains/arm-none-eabi-f767-toolchain.cmake"

# Every sample whose Kconfig arch gate admits the Cortex-M7.
SAMPLES=$("$REPO_ROOT/tools/samples_for_arch.sh" ARCH_CORTEX_M7)
[ -n "$SAMPLES" ] || { echo "no ARCH_CORTEX_M7 samples found" >&2; exit 2; }

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
echo "==== F767 portable-sample summary ===="
printf 'pass=%d fail=%d\n' "$PASS" "$FAIL"
if [ "$FAIL" -gt 0 ]; then
  echo "Failed:$FAIL_LIST" >&2
  echo "Re-run a single failure with:" >&2
  echo "  rm -f .config && cmake -B build-X -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN -DSAMPLE=<name> && cmake --build build-X -j" >&2
  exit 1
fi
