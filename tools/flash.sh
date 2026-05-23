#!/usr/bin/env bash
# Build and flash a NavHAL sample to a connected board.
#
# Usage:
#   tools/flash.sh [sample] [target] [port]
#
#   sample   Short sample name (see samples/README.md). Default: hal_blink.
#   target   stm32 | avr.  If omitted, the existing .config is used as-is.
#   port     Serial / programmer port. Defaults: /dev/ttyACM0 (stm32),
#            /dev/ttyUSB0 (avr).
#
# Samples live in capability tiers under samples/ (portable/, cortex-m/,
# no_hal/); the build resolves a short name to its tier directory, so this
# script does not care which tier a sample is in. A cortex-m / no_hal sample
# is not selectable for the avr target — that build will fail by design.
#
# When `target` is given the script swaps in a minimal .config for that
# architecture (your existing .config is backed up and restored on exit);
# default drivers are used, which covers the portable acceptance samples.
# For a sample needing non-default drivers, configure .config yourself and
# run without the `target` argument.
#
# Each target builds in its own directory (build/ for stm32, build-avr/ for
# avr) and the flash step uses the arch-aware `flash` CMake target —
# st-flash for Cortex-M, avrdude for AVR.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

SAMPLE="${1:-hal_blink}"
TARGET="${2:-}"
PORT="${3:-}"

CMAKE_ARGS=(-DSTANDALONE=OFF -DSAMPLE="$SAMPLE" -DTEST=OFF -G "Unix Makefiles")
BUILD_DIR="build"
CONFIG_BACKUP=""

# Restore the user's .config if we swapped it out.
restore_config() {
  if [[ -n "$CONFIG_BACKUP" && -f "$CONFIG_BACKUP" ]]; then
    mv -f "$CONFIG_BACKUP" "$REPO_ROOT/.config"
  fi
}
trap restore_config EXIT

case "$TARGET" in
  stm32)
    BUILD_DIR="build"
    PORT="${PORT:-/dev/ttyACM0}"
    [[ -f .config ]] && CONFIG_BACKUP="$(mktemp)" && cp .config "$CONFIG_BACKUP"
    printf 'CONFIG_ARCH_CORTEX_M4=y\n' > .config
    ;;
  avr)
    BUILD_DIR="build-avr"
    PORT="${PORT:-/dev/ttyUSB0}"
    CMAKE_ARGS+=(-DAVR_PORT="$PORT")
    [[ -f .config ]] && CONFIG_BACKUP="$(mktemp)" && cp .config "$CONFIG_BACKUP"
    printf 'CONFIG_ARCH_AVR8=y\n' > .config
    ;;
  "")
    # No target given — flash whatever the current .config selects.
    ;;
  *)
    echo "error: unknown target '$TARGET' (expected 'stm32' or 'avr')" >&2
    exit 2
    ;;
esac

echo ">> sample=$SAMPLE  target=${TARGET:-<.config>}  build=$BUILD_DIR"
rm -rf "$BUILD_DIR"
cmake -B "$BUILD_DIR" "${CMAKE_ARGS[@]}" .

echo ">> building + flashing (CMake 'flash' target)"
cmake --build "$BUILD_DIR" --target flash
