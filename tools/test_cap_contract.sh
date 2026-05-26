#!/usr/bin/env bash
# Verifies the NavHAL NAVHAL_HAS_* capability contract at link time.
#
# Three scenarios are exercised against the on-target test ELF:
#   (1) default config — DMA / UART_DMA capabilities should LINK.
#   (2) no-cap strip   — all optional driver symbols should be ABSENT.
#   (3) sub-cap isolation — DRV_SDIO_DMA=y AND DRV_UART_DMA=n: SDIO async
#       symbols present, UART DMA symbols absent. Catches regressions where
#       a driver's DMA code is gated on the global _DMA_ENABLED instead of
#       its per-driver sub-cap.
#
# Exit non-zero on any unexpected presence/absence. Used both locally
# (.githooks/pre-push) and in CI (.github/workflows/ci.yml).
#
# Usage: tools/test_cap_contract.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

NM=${NM:-arm-none-eabi-nm}
command -v "$NM" >/dev/null || { echo "error: '$NM' not on PATH" >&2; exit 2; }

SAVED_CONFIG=""
STUB=build-cap-stub
DEFAULT=build-cap-default
NOCAP=build-cap-nocap
MIX=build-cap-mix

cleanup() {
  if [ -n "$SAVED_CONFIG" ] && [ -f "$SAVED_CONFIG" ]; then
    mv -f "$SAVED_CONFIG" .config
  else
    rm -f .config
  fi
  rm -rf "$STUB" "$DEFAULT" "$NOCAP" "$MIX"
}
trap cleanup EXIT

if [ -f .config ]; then
  SAVED_CONFIG=$(mktemp)
  cp .config "$SAVED_CONFIG"
fi

PASS=0
FAIL=0
ok()   { printf '  \033[32mOK\033[0m   %s\n' "$1"; PASS=$((PASS+1)); }
nope() { printf '  \033[31mFAIL\033[0m %s\n' "$1"; FAIL=$((FAIL+1)); }

# Note: grep -q would early-exit on first match, closing the pipe and causing
# nm to SIGPIPE (exit 141). Under `set -o pipefail` the pipe's exit status
# is then 141, which the surrounding `if` interprets as a non-match. Drop
# the -q so grep reads to EOF.
has_sym() { "$NM" -gC "$1" | grep -E "\b$2\b" >/dev/null; }
expect_present() {
  if has_sym "$1" "$2"; then ok "$2 present"; else nope "$2 MISSING from $(basename "$1")"; fi
}
expect_absent() {
  if has_sym "$1" "$2"; then nope "$2 LEAKED into $(basename "$1")"; else ok "$2 absent"; fi
}

materialize_default_config() {
  rm -f .config
  cmake -B "$STUB" -DTEST=ON >/dev/null
  rm -rf "$STUB"
}

build_test() {
  local dir=$1
  rm -rf "$dir"
  cmake -B "$dir" -DTEST=ON >/dev/null
  cmake --build "$dir" --target tests -j >/dev/null
}

echo "==== Scenario 1: default config (caps on) ===="
materialize_default_config
build_test "$DEFAULT"
expect_present "$DEFAULT/tests" hal_dma_init
expect_present "$DEFAULT/tests" hal_uart_write_dma

echo "==== Scenario 2: no-cap strip (everything off) ===="
materialize_default_config
sed -i \
  -e 's/^CONFIG_DRV_DMA=y/# CONFIG_DRV_DMA is not set/' \
  -e 's/^CONFIG_DRV_FPU=y/# CONFIG_DRV_FPU is not set/' \
  -e 's/^CONFIG_DRV_SDIO=y/# CONFIG_DRV_SDIO is not set/' \
  -e 's/^CONFIG_DRV_DWT=y/# CONFIG_DRV_DWT is not set/' \
  -e 's/^CONFIG_USE_FPU=y/# CONFIG_USE_FPU is not set/' \
  -e 's/^CONFIG_DRV_UART_DMA=y/# CONFIG_DRV_UART_DMA is not set/' \
  -e 's/^CONFIG_DRV_I2C_DMA=y/# CONFIG_DRV_I2C_DMA is not set/' \
  -e 's/^CONFIG_DRV_SDIO_DMA=y/# CONFIG_DRV_SDIO_DMA is not set/' \
  .config
build_test "$NOCAP"
for sym in hal_dma_init hal_sdio_init hal_cycle_counter_init hal_fpu_enable \
           hal_uart_write_dma hal_i2c_read_regs_dma hal_sdio_read_block_async; do
  expect_absent "$NOCAP/tests" "$sym"
done

echo "==== Scenario 3: sub-cap isolation (SDIO_DMA on, UART_DMA off) ===="
materialize_default_config
sed -i \
  -e 's/^CONFIG_DRV_UART_DMA=y/# CONFIG_DRV_UART_DMA is not set/' \
  -e 's/^# CONFIG_DRV_SDIO is not set/CONFIG_DRV_SDIO=y/' \
  .config
# Re-materialize so the SDIO select cascade fires and SDIO_DMA defaults y.
cmake -B "$STUB" -DTEST=ON >/dev/null && rm -rf "$STUB"
build_test "$MIX"
expect_absent  "$MIX/tests" hal_uart_write_dma
expect_present "$MIX/tests" hal_sdio_read_block_async
expect_present "$MIX/tests" hal_dma_init

echo
echo "==== Summary ===="
printf 'pass=%d fail=%d\n' "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ]
