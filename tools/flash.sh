#!/usr/bin/env bash
# Build and flash a NavHAL sample to a connected board.
#
# Usage:
#   tools/flash.sh [sample] [target] [port]
#   tools/flash.sh --list          # list the available targets and exit
#
#   sample   Short sample name (see samples/README.md). Default: hal_blink.
#   target   Full chip name — stm32f401re | stm32f767zi | atmega328p (see
#            --list). If omitted, the existing .config is used as-is.
#   port     Serial / programmer port. Defaults: /dev/ttyACM0 (STM32),
#            /dev/ttyUSB0 (AVR).
#
# For an STM32 target the ST-Link is matched automatically by the target's
# chip-id (via `st-info --probe`, the same way tools/hil/run.sh does it), so the
# right board is flashed on a multi-probe bench without hard-coding serials —
# just name the board.
#
# Options:
#   --serial <id>   Override the auto-detected ST-Link serial (STM32 only), for
#                   the rare case of two identical boards. From `st-info --probe`.
#   --list          List the available targets and exit.
#
# Samples live in capability tiers under samples/ (portable/, cortex-m/,
# no_hal/); the build resolves a short name to its tier directory, so this
# script does not care which tier a sample is in. A cortex-m / no_hal sample
# is not selectable for the avr target — that build will fail by design.
#
# When `target` is given the script picks the matching CMake toolchain
# file under cmake/toolchains/ — the toolchain file points at a defconfig
# fragment that seeds .config (arch / vendor / family / board). Your
# existing .config is moved aside and restored on exit. Default drivers
# are used, which covers the portable acceptance samples; for a sample
# needing non-default drivers, configure .config yourself and run
# without the `target` argument.
#
# Each target builds in its own directory (build-<target>) and the flash step
# uses the arch-aware `flash` CMake target — st-flash for Cortex-M, avrdude for
# AVR.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

list_targets() {
  echo "Available targets:"
  printf '  %-13s %s\n' \
    "stm32f401re" "Nucleo-F401RE (Cortex-M4)  chipid 0x433" \
    "stm32f767zi" "Nucleo-F767ZI (Cortex-M7)  chipid 0x451" \
    "atmega328p"  "ATmega328P    (AVR)        avrdude via port"
}

# Serial of the connected ST-Link whose target chip-id matches $1 (portable —
# no hard-coded serials, mirrors tools/hil/run.sh's detect_probe).
detect_serial() {
  st-info --probe 2>/dev/null | awk -v want="$1" '
    /serial:/ {s=$2}
    /chipid:/ {if ($2==want) {print s; exit}}'
}

SERIAL=""
POSARGS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --list|-l) list_targets; exit 0 ;;
    --serial) SERIAL="${2:-}"; shift 2 ;;
    --serial=*) SERIAL="${1#*=}"; shift ;;
    *) POSARGS+=("$1"); shift ;;
  esac
done
if [[ ${#POSARGS[@]} -gt 0 ]]; then set -- "${POSARGS[@]}"; else set --; fi

SAMPLE="${1:-hal_blink}"
TARGET="${2:-}"
PORT="${3:-}"

CMAKE_ARGS=(-DSTANDALONE=OFF -DSAMPLE="$SAMPLE" -DTEST=OFF -G "Unix Makefiles")
BUILD_DIR="build"
CONFIG_BACKUP=""

# Restore the user's .config if we moved it out of the way.
restore_config() {
  if [[ -n "$CONFIG_BACKUP" && -f "$CONFIG_BACKUP" ]]; then
    mv -f "$CONFIG_BACKUP" "$REPO_ROOT/.config"
  fi
}
trap restore_config EXIT

# When swapping arch, the toolchain file's NAVHAL_DEFCONFIG only seeds
# .config when none exists — so we move any existing .config aside first.
stash_config() {
  if [[ -f .config ]]; then
    CONFIG_BACKUP="$(mktemp)"
    mv .config "$CONFIG_BACKUP"
  fi
}

CHIPID=""
case "$TARGET" in
  stm32f401re)
    BUILD_DIR="build-stm32f401re"
    CHIPID="0x433"
    CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-toolchain.cmake)
    stash_config
    ;;
  stm32f767zi)
    BUILD_DIR="build-stm32f767zi"
    CHIPID="0x451"
    CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi-f767-toolchain.cmake)
    stash_config
    ;;
  atmega328p)
    BUILD_DIR="build-avr"
    PORT="${PORT:-/dev/ttyUSB0}"
    CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/avr-toolchain.cmake -DAVR_PORT="$PORT")
    stash_config
    ;;
  "")
    # No target given — flash whatever the current .config selects.
    ;;
  *)
    echo "error: unknown target '$TARGET'" >&2
    list_targets >&2
    exit 2
    ;;
esac

echo ">> sample=$SAMPLE  target=${TARGET:-<.config>}  build=$BUILD_DIR${SERIAL:+  serial=$SERIAL}"
rm -rf "$BUILD_DIR"
cmake -B "$BUILD_DIR" "${CMAKE_ARGS[@]}" .

if [[ "$TARGET" == "atmega328p" ]]; then
  echo ">> building + flashing (avrdude)"
  cmake --build "$BUILD_DIR" --target flash
elif [[ -n "$CHIPID" || -n "$SERIAL" ]]; then
  # STM32: flash by ST-Link serial so the right board is programmed on a
  # multi-probe bench. The serial is auto-detected from the target's chip-id
  # unless --serial overrides it. All STM32 parts boot from 0x08000000; the
  # CMake 'flash' target can't pass a serial, so flash directly here.
  if [[ -z "$SERIAL" ]]; then
    SERIAL="$(detect_serial "$CHIPID")"
    [[ -n "$SERIAL" ]] || {
      echo "error: no connected ST-Link with a $CHIPID target; is the board plugged in? (or pass --serial <id>)" >&2
      exit 1
    }
    echo ">> matched ST-Link $SERIAL (chipid $CHIPID)"
  fi
  echo ">> building"
  cmake --build "$BUILD_DIR"
  elf="$(find "$BUILD_DIR/samples" -type f -name "$SAMPLE" -perm -u+x | head -1)"
  [[ -n "$elf" ]] || { echo "error: built ELF '$SAMPLE' not found in $BUILD_DIR" >&2; exit 1; }
  arm-none-eabi-objcopy -O binary "$elf" "$elf.bin"
  echo ">> flashing $SAMPLE to ST-Link $SERIAL"
  st-flash --serial "$SERIAL" --reset write "$elf.bin" 0x08000000
else
  echo ">> building + flashing (CMake 'flash' target)"
  cmake --build "$BUILD_DIR" --target flash
fi
