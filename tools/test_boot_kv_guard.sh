#!/usr/bin/env bash
# Checks that a boot image (-DBOOT_IMAGE=...) is refused at configure time when
# the flash KV store is not in sectors 2 and 3.
#
# The loader layout (include/common/hal_bootmap.h) puts the store in sectors 2-3,
# below the stage-2 and app partitions. The store's defaults, 6 and 7, are inside
# the app partition; an image that keeps them can still link (the linker only
# objects once the image reaches the store) and then fails every update at the
# erase. Cases, each a configure-only run for the production board's app image:
#
#   (1) DRV_FLASH off            -> configures: there is no KV store to misplace.
#   (2) DRV_FLASH on, KV 2/3     -> configures.
#   (3) DRV_FLASH on, KV default -> refused, naming both Kconfig lines.
#   (4) DRV_FLASH on, KV 4/5     -> refused (any pair but 2/3).
#
# Configure only, no compile, so it costs seconds. Usage: tools/test_boot_kv_guard.sh

set -uo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

BASE=cmake/defconfigs/boot_cortex-m4_stm32f4_nucleo_f401re.defconfig
TOOLCHAIN=cmake/toolchains/arm-none-eabi-toolchain.cmake
WORK="$(mktemp -d)"
SAVED_CONFIG=""

cleanup() {
  if [ -n "$SAVED_CONFIG" ] && [ -f "$SAVED_CONFIG" ]; then
    mv -f "$SAVED_CONFIG" .config
  else
    rm -f .config
  fi
  rm -rf "$WORK"
}
trap cleanup EXIT

# The Kconfig step reads (and writes) ./.config, which would otherwise carry one
# case's answers into the next, so each case starts from none.
if [ -f .config ]; then
  SAVED_CONFIG=$(mktemp)
  cp .config "$SAVED_CONFIG"
fi

PASS=0
FAIL=0
ok()   { printf '  \033[32mOK\033[0m   %s\n' "$1"; PASS=$((PASS+1)); }
nope() { printf '  \033[31mFAIL\033[0m %s\n' "$1"; FAIL=$((FAIL+1)); }

# configure <name> <defconfig> : sets RC and LOG
configure() {
  local name=$1 defconfig=$2
  rm -f .config
  LOG="$WORK/$name.log"
  cmake -S . -B "$WORK/$name" -DBOARD=navixsmf401re -DBOOT_IMAGE=app \
    -DNAVHAL_DEFCONFIG="$defconfig" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" >"$LOG" 2>&1
  RC=$?
}

# A defconfig that is the Nucleo loader one, with the board swapped for the
# production board and the KV lines and DRV_FLASH set as each case wants.
make_defconfig() {
  local out=$1 flash=$2 kv=$3   # flash: y|n ; kv: "" | "P S"
  sed -e 's/CONFIG_BOARD_NUCLEO_F401RE=y/CONFIG_BOARD_NAVIXSM_F401RE=y/' \
      -e '/CONFIG_FLASH_KV_/d' "$BASE" >"$out"
  if [ "$flash" = n ]; then
    sed -i -e '/CONFIG_DRV_FLASH=y/d' -e '/CONFIG_BOOT_SNIFFER=y/d' "$out"
  fi
  if [ -n "$kv" ]; then
    set -- $kv
    printf 'CONFIG_FLASH_KV_PRIMARY_SECTOR=%s\nCONFIG_FLASH_KV_SECONDARY_SECTOR=%s\n' "$1" "$2" >>"$out"
  fi
}

refused_with_hint() {
  [ "$RC" -ne 0 ] \
    && grep -q 'needs the flash KV store in sectors 2 and 3' "$LOG" \
    && grep -q 'CONFIG_FLASH_KV_PRIMARY_SECTOR=2' "$LOG" \
    && grep -q 'CONFIG_FLASH_KV_SECONDARY_SECTOR=3' "$LOG"
}

echo "boot image KV placement guard"

make_defconfig "$WORK/off.defconfig" n ""
configure off "$WORK/off.defconfig"
if [ "$RC" -eq 0 ] && ! grep -q 'KV store in sectors' "$LOG"; then
  ok "(1) DRV_FLASH off: configures"
else nope "(1) DRV_FLASH off: expected to configure (rc=$RC, see $LOG)"; tail -5 "$LOG"; fi

make_defconfig "$WORK/good.defconfig" y "2 3"
configure good "$WORK/good.defconfig"
if [ "$RC" -eq 0 ] && grep -q 'Flash KV store: sectors 2,3' "$LOG"; then
  ok "(2) DRV_FLASH on, KV 2/3: configures"
else nope "(2) DRV_FLASH on, KV 2/3: expected to configure (rc=$RC)"; tail -5 "$LOG"; fi

make_defconfig "$WORK/default.defconfig" y ""
configure default "$WORK/default.defconfig"
if refused_with_hint; then
  ok "(3) DRV_FLASH on, KV default 6/7: refused, names both Kconfig lines"
else nope "(3) DRV_FLASH on, KV default: expected a refusal with the hint (rc=$RC)"; tail -8 "$LOG"; fi

make_defconfig "$WORK/other.defconfig" y "4 5"
configure other "$WORK/other.defconfig"
if refused_with_hint; then
  ok "(4) DRV_FLASH on, KV 4/5: refused"
else nope "(4) DRV_FLASH on, KV 4/5: expected a refusal (rc=$RC)"; tail -8 "$LOG"; fi

echo "  $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
