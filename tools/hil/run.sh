#!/usr/bin/env bash
# Build the on-target test ELF for <board> and run it on real hardware
# (HIL — hardware-in-the-loop). The sibling of tools/pil/run.sh: same
# board-registry shape, but instead of booting an emulator it flashes the
# physical board over its ST-Link and captures the board's UART console.
#
# Usage:
#   tools/hil/run.sh <board>              # build + flash + run one board
#   tools/hil/run.sh --all                # every board with a hardware match
#   tools/hil/run.sh --list               # list known boards
#
# Per-board config lives in tools/hil/boards/<name>.conf. Boards are matched
# to a physically-connected ST-Link by STM32 chip-id (stable across board
# instances), never by a hard-coded ST-Link serial — so a checkout works on
# any bench. Adding a board is one new .conf file.
#
# Requires: arm-none-eabi-gcc, st-flash / st-info (stlink-tools), python3 +
#           pyserial, and udevadm (to map an ST-Link serial to its ttyACM).

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$REPO_ROOT"

BOARDS_DIR="tools/hil/boards"
CAPTURE="tools/hil/uart_capture.py"

usage() {
  echo "Usage: $0 <board> | --all | --list" >&2
  exit 2
}

list_boards() {
  echo "Known HIL boards:"
  for f in "$BOARDS_DIR"/*.conf; do
    [ -f "$f" ] || continue
    name=$(basename "$f" .conf)
    arch=$(sed -n 's/^ARCH=//p' "$f" | head -1)
    chip=$(sed -n 's/^CHIPID=//p' "$f" | head -1)
    printf '  %-18s arch=%-10s chipid=%s\n' "$name" "$arch" "$chip"
  done
}

[ $# -ge 1 ] || usage
case "$1" in
  -h|--help) usage ;;
  --list) list_boards; exit 0 ;;
esac

# Find the ST-Link serial of a connected probe whose target chip-id matches,
# then the /dev/ttyACM* that belongs to that same ST-Link.
detect_probe() {  # $1 = chipid (e.g. 0x451); sets DETECTED_SERIAL / DETECTED_PORT
  local want="$1"
  DETECTED_SERIAL=$(st-info --probe 2>/dev/null | awk -v want="$want" '
    /serial:/ {s=$2}
    /chipid:/ {if ($2==want) {print s; exit}}')
  [ -n "$DETECTED_SERIAL" ] || return 1
  DETECTED_PORT=""
  local p ps
  for p in /dev/ttyACM*; do
    [ -e "$p" ] || continue
    ps=$(udevadm info -q property -n "$p" 2>/dev/null | sed -n 's/^ID_SERIAL_SHORT=//p')
    if [ "$ps" = "$DETECTED_SERIAL" ]; then DETECTED_PORT="$p"; break; fi
  done
  [ -n "$DETECTED_PORT" ] || return 2
}

run_board() {  # $1 = board name; returns the on-target failure count
  local board="$1"
  local conf="$BOARDS_DIR/$board.conf"
  if [ ! -f "$conf" ]; then
    echo "error: no board config at $conf" >&2
    list_boards >&2
    return 2
  fi

  # shellcheck source=/dev/null
  . "$conf"

  local v
  for v in ARCH TOOLCHAIN_FILE BUILD_DIR DEFCONFIG CHIPID; do
    if [ -z "${!v:-}" ]; then
      echo "error: $conf is missing required variable $v" >&2
      return 2
    fi
  done
  local baud="${BAUD:-9600}"
  local timeout="${TIMEOUT:-120}"
  local flash_addr="${FLASH_ADDR:-0x08000000}"

  echo "=================================================================="
  echo ">> HIL board=$board arch=$ARCH chipid=$CHIPID"

  # Match this board to a connected probe before spending time on a build.
  if ! detect_probe "$CHIPID"; then
    echo "!! no connected ST-Link with a $CHIPID target (or no ttyACM for it)"
    echo "!! skipping $board"
    return 3
  fi
  echo ">> probe: st-link $DETECTED_SERIAL  console $DETECTED_PORT @ $baud"

  # Deterministic target config: DEFCONFIG + any opt-in caps. Stash the
  # user's .config and restore on return (mirrors tools/pil/run.sh).
  local saved=""
  if [ -f .config ]; then saved=$(mktemp); mv .config "$saved"; fi
  # shellcheck disable=SC2064
  trap "if [ -n '$saved' ] && [ -f '$saved' ]; then mv -f '$saved' .config; else rm -f .config; fi" RETURN

  rm -rf "$BUILD_DIR"
  cat "$DEFCONFIG" > .config
  if [ -n "${TEST_EXTRA_CONFIG:-}" ]; then
    for kv in $TEST_EXTRA_CONFIG; do printf '%s\n' "$kv" >> .config; done
    echo ">> caps: $DEFCONFIG + [$TEST_EXTRA_CONFIG]"
  fi

  echo ">> building test ELF ($BUILD_DIR)"
  cmake -B "$BUILD_DIR" -DTEST=ON -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" >/dev/null
  cmake --build "$BUILD_DIR" --target tests -j >/dev/null
  arm-none-eabi-objcopy -O binary "$BUILD_DIR/tests" "$BUILD_DIR/tests.bin"

  # Start the UART reader BEFORE the flash reset so the startup banner is not
  # missed, then flash the matched probe (its software reset kicks off the run).
  echo ">> capturing $DETECTED_PORT (timeout ${timeout}s)"
  python3 "$CAPTURE" "$DETECTED_PORT" "$baud" "$timeout" &
  local cap=$!
  sleep 1
  echo ">> flashing $board (st-link $DETECTED_SERIAL)"
  st-flash --serial "$DETECTED_SERIAL" --reset write "$BUILD_DIR/tests.bin" "$flash_addr" >/dev/null 2>&1

  local rc=0
  wait "$cap" || rc=$?
  echo ">> $board: uart_capture exit=$rc (0 = all pass, N = failures, 124 = timeout)"
  return "$rc"
}

# ---- dispatch --------------------------------------------------------------
if [ "$1" = "--all" ]; then
  overall=0
  for f in "$BOARDS_DIR"/*.conf; do
    [ -f "$f" ] || continue
    b=$(basename "$f" .conf)
    run_board "$b" || { c=$?; [ "$c" = 3 ] || overall=$((overall + c)); }
  done
  echo "=================================================================="
  echo ">> HIL --all aggregate failure count: $overall"
  exit "$overall"
fi

run_board "$1"
