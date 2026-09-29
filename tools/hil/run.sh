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
# A board that is not attached exits 77 — the status vtest's check adapter and
# ctest both read as SKIP. A bench carrying two of the five boards is the
# normal case, not four failures.
#
# A board whose conf sets CONSOLE=swd routed no console UART: it is flashed and
# read through its debug probe instead (swd_capture). Such a board can also
# have its probe pinned by USB location, which is a fact about one bench and so
# lives in the environment — NAVHAL_HIL_LOCATION_<BOARD>=<bus>-<port>, see
# hil_usb_location.
#
# Requires: arm-none-eabi-gcc, st-flash / st-info (stlink-tools), python3 +
#           pyserial, and udevadm (to map an ST-Link serial to its ttyACM);
#           openocd + gdb-multiarch for the CONSOLE=swd boards.
#
# A board whose VCP is root-owned looks disconnected here, because the
# console cannot be read. Distributions leave that to TAG+="uaccess", which
# is per-seat and does not always apply. Install
# tools/hil/99-navhal-stlink.rules for group-based access that does not
# depend on how the probe enumerated.

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
  # Every probe with this chip-id, not just the first: a bench can hold more
  # than one board of a family (NavHAL has two F401 configs), and the first
  # match is not necessarily the one whose ST-Link exposes a usable ttyACM.
  local serials
  serials=$(st-info --probe 2>/dev/null | awk -v want="$want" '
    /serial:/ {s=$2}
    /chipid:/ {if ($2==want) print s}')
  [ -n "$serials" ] || return 1

  DETECTED_PORT=""
  # The first matching probe, whether or not it brings a readable VCP: a board
  # driven over SWD has none, and only the caller knows whether that matters.
  DETECTED_SERIAL=${serials%%$'\n'*}
  local cand p ps
  for cand in $serials; do
    for p in /dev/ttyACM*; do
      [ -e "$p" ] || continue
      ps=$(udevadm info -q property -n "$p" 2>/dev/null | sed -n 's/^ID_SERIAL_SHORT=//p')
      if [ "$ps" = "$cand" ] && [ -r "$p" ] && [ -w "$p" ]; then
        DETECTED_SERIAL="$cand"
        DETECTED_PORT="$p"
        break 2
      fi
    done
  done
  [ -n "$DETECTED_PORT" ] || return 2
}

# The AVR bench has no debug probe: an Arduino-class board is a USB-serial
# bridge wired to the MCU's bootloader, so the same port both flashes it and
# carries the test console. Matched by USB vendor id, since the bridge chip
# varies (CH340 clones, FTDI, genuine 2341) while the board does not.
detect_avr_port() {  # $1 = comma-separated vendor ids; sets DETECTED_PORT
  local want="$1" p vid
  DETECTED_PORT=""
  DETECTED_SERIAL=""
  for p in /dev/ttyUSB* /dev/ttyACM*; do
    [ -e "$p" ] || continue
    vid=$(udevadm info -q property -n "$p" 2>/dev/null | sed -n 's/^ID_VENDOR_ID=//p')
    [ -n "$vid" ] || continue
    case ",$want," in
      *",$vid,"*)
        if [ -r "$p" ] && [ -w "$p" ]; then
          DETECTED_PORT="$p"
          DETECTED_SERIAL="$vid"
          return 0
        fi
        ;;
    esac
  done
  return 1
}

# Which port a probe is plugged into is a property of one bench, not of the
# project, so it lives in the environment and never in a committed board conf:
#
#   NAVHAL_HIL_LOCATION_<BOARD>=<bus>-<port>   # one named board
#   NAVHAL_HIL_LOCATION=<bus>-<port>           # the only probe on the bench
#
# Unset is the normal case and changes nothing: the runner falls back to
# enumerating probes, which is correct on a bench with one of them.
hil_usb_location() {  # $1 = board name
  local key
  key=$(printf '%s' "$1" | tr -c '[:alnum:]' '_' | tr '[:lower:]' '[:upper:]')
  eval "printf '%s' \"\${NAVHAL_HIL_LOCATION_$key:-\${NAVHAL_HIL_LOCATION:-}}\""
}

# Run the test ELF on a board with no serial console: flash and drive it over
# SWD, reading the console strings out of the target as it prints them. A probe
# with no virtual COM port (a bare ST-LINK/V2, or a board that never routed its
# console UART) still has to be testable; breaking on hal_uart_write_string and
# printing the argument reconstructs the transcript the wire would have carried.
# Prints that transcript, and returns the on-target failure count (124 = the
# suite never reached its summary).
swd_capture() {  # $1 = test ELF, $2 = timeout s, $3 = usb location (may be empty)
  local elf="$1" to="$2" loc="$3" script out rc=0 fails a placed=0
  local -a ocd=()
  for a in ${OPENOCD_ARGS:-}; do
    ocd+=("$a")
    # `adapter usb location` is a pre-init command that is only accepted once
    # the interface config has loaded the driver, so it goes after that -f
    # rather than at the end of the argument list.
    if [ "$placed" = 0 ] && [ -n "$loc" ]; then
      case "$a" in
        interface/*) ocd+=(-c "adapter usb location $loc"); placed=1 ;;
      esac
    fi
  done
  if [ -n "$loc" ] && [ "$placed" = 0 ]; then
    ocd+=(-c "adapter usb location $loc")
  fi

  local ocdlog
  ocdlog=$(mktemp)
  openocd "${ocd[@]}" >"$ocdlog" 2>&1 &
  local ocd_pid=$!
  # OpenOCD exits at once when the adapter is not there, leaving gdb nothing to
  # attach to. That is an absent board (77), not a failed suite -- but a
  # different early exit (its port already in use, a bad config) is a real
  # error, so report which of the two happened rather than skipping both.
  sleep 1
  if ! kill -0 "$ocd_pid" 2>/dev/null; then
    wait "$ocd_pid" 2>/dev/null || true
    cat "$ocdlog"
    if grep -qiE 'no device|unable to find|open failed|no such device' "$ocdlog"; then
      rm -f "$ocdlog"
      echo "!! no debug adapter${loc:+ at usb location $loc}"
      return 77
    fi
    rm -f "$ocdlog"
    return 1
  fi
  script=$(mktemp --suffix=.gdb)
  # navhal_post_main runs once main() returns, so it is where the suite is over
  # and the totals have already been printed.
  cat > "$script" <<'GDB'
set confirm off
set pagination off
set print elements 0
target extended-remote localhost:3333
monitor reset halt
load
monitor reset halt
break navhal_post_main
break hal_uart_write_string
commands 2
  silent
  printf "%s", s
  continue
end
continue
monitor reset halt
detach
quit
GDB
  out=$(timeout "$to" gdb-multiarch -batch -x "$script" "$elf" 2>/dev/null) || rc=$?
  kill "$ocd_pid" 2>/dev/null || true
  wait "$ocd_pid" 2>/dev/null || true
  rm -f "$script" "$ocdlog"

  # OpenOCD prints this to gdb's console on every breakpoint stop, interleaved
  # with the target's own output, where it lands between a label and the number
  # that follows it and breaks the totals match below.
  out=$(printf '%s' "$out" | sed 's/halted: PC: 0x[0-9a-f]*//g')
  printf '%s\n' "$out"
  [ "$rc" != 124 ] || return 124
  fails=$(printf '%s' "$out" |
    sed -n 's/.*Total failures:[[:space:]]*\([0-9]\{1,\}\).*/\1/p' | tail -1)
  [ -n "$fails" ] || return 124
  return "$fails"
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

  # CHIPID is what st-info reports, so it is required only for the probe-based
  # boards. An AVR board is identified by the USB vendor id of its serial
  # bridge instead, and would never have one.
  local required="ARCH TOOLCHAIN_FILE BUILD_DIR DEFCONFIG CHIPID"
  if [ "$ARCH" = "avr" ]; then
    required="ARCH TOOLCHAIN_FILE BUILD_DIR DEFCONFIG MCU PROGRAMMER USB_VID"
  fi

  local v
  for v in $required; do
    if [ -z "${!v:-}" ]; then
      echo "error: $conf is missing required variable $v" >&2
      return 2
    fi
  done
  local baud="${BAUD:-9600}"
  local timeout="${TIMEOUT:-120}"
  local flash_addr="${FLASH_ADDR:-0x08000000}"

  # A board that routed no console UART, and whose probe has no virtual COM
  # port, is driven over SWD instead -- see swd_capture.
  local swd=0 loc=""
  case "$(printf '%s' "${CONSOLE:-}" | tr '[:upper:]' '[:lower:]')" in
    swd) swd=1 ;;
  esac

  echo "=================================================================="
  echo ">> HIL board=$board arch=$ARCH ${CHIPID:+chipid=$CHIPID}${MCU:+mcu=$MCU}"

  if [ "$ARCH" = "avr" ]; then
    if ! detect_avr_port "$USB_VID"; then
      echo "!! no readable USB-serial port with vendor id in [$USB_VID]"
      echo "!! plug the board in, and check you are in the port's group"
      echo "!! (dialout for /dev/ttyUSB*, plugdev for /dev/ttyACM*)"
      echo "!! skipping $board"
      return 77
    fi
    echo ">> port: $DETECTED_PORT (usb vendor $DETECTED_SERIAL) @ $baud"
  elif [ "$swd" = 1 ]; then
    # A pinned location addresses the probe through OpenOCD and skips
    # enumeration entirely. `st-info --probe` opens every ST-Link on the bus to
    # read its target's chip-id, which resets the boards behind the other
    # probes -- so on a bench with more than one, the act of looking disturbs
    # the boards we are not testing. It is also useless when those probes
    # report identical serials, which the cheap V2 clones do.
    loc=$(hil_usb_location "$board")
    DETECTED_SERIAL=""
    if [ -n "$loc" ]; then
      echo ">> probe: usb location $loc (enumeration skipped) -- console over swd"
    else
      # No VCP is expected here, so a chip-id match with no ttyACM behind it is
      # still this board -- the opposite of the console-over-UART case below.
      detect_probe "$CHIPID" || true
      if [ -z "$DETECTED_SERIAL" ]; then
        echo "!! no connected ST-Link with a $CHIPID target"
        echo "!! pin this board's probe with NAVHAL_HIL_LOCATION_<BOARD>=<bus>-<port>"
        echo "!! to address it without enumerating (see $conf)"
        echo "!! skipping $board"
        return 77
      fi
      echo ">> probe: st-link $DETECTED_SERIAL -- console over swd"
    fi
  # Match this board to a connected probe before spending time on a build.
  elif ! detect_probe "$CHIPID"; then
    echo "!! no connected ST-Link with a $CHIPID target (or no usable ttyACM)"
    # Distinguish "not plugged in" from "plugged in but the console is
    # root-owned", because the fix is completely different.
    if st-info --probe 2>/dev/null | grep -q "$CHIPID"; then
      echo "!! a $CHIPID probe IS attached, but none has a readable VCP."
      echo "!! install tools/hil/99-navhal-stlink.rules, then replug the board."
    fi
    echo "!! skipping $board"
    return 77
  else
    echo ">> probe: st-link $DETECTED_SERIAL  console $DETECTED_PORT @ $baud"
  fi

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

  local cap rc=0
  if [ "$swd" = 1 ]; then
    # gdb loads the ELF itself, so there is nothing to objcopy and no st-flash.
    echo ">> flashing + capturing over swd (timeout ${timeout}s)"
    swd_capture "$BUILD_DIR/tests" "$timeout" "$loc" || rc=$?
    echo ">> $board: swd_capture exit=$rc (0 = all pass, N = failures, 124 = no summary)"
    return "$rc"
  fi

  if [ "$ARCH" = "avr" ]; then
    avr-objcopy -O ihex "$BUILD_DIR/tests" "$BUILD_DIR/tests.hex"

    # One port does both jobs here, so the order has to be the opposite of the
    # probe boards': flash first, then capture. Opening the port toggles DTR,
    # which resets the board and restarts the run from its banner -- the same
    # auto-reset avrdude just used to enter the bootloader.
    echo ">> flashing $board (avrdude -c $PROGRAMMER -p $MCU)"
    if ! avrdude -c "$PROGRAMMER" -p "$MCU" -P "$DETECTED_PORT" \
                 -b "${UPLOAD_BAUD:-115200}" -U "flash:w:$BUILD_DIR/tests.hex:i" \
                 >/dev/null 2>&1; then
      echo "!! avrdude failed on $DETECTED_PORT"
      return 2
    fi
    echo ">> capturing $DETECTED_PORT (timeout ${timeout}s)"
    python3 "$CAPTURE" "$DETECTED_PORT" "$baud" "$timeout" &
    cap=$!
  else
    arm-none-eabi-objcopy -O binary "$BUILD_DIR/tests" "$BUILD_DIR/tests.bin"

    # Start the UART reader BEFORE the flash reset so the startup banner is not
    # missed, then flash the matched probe (its software reset kicks off the run).
    echo ">> capturing $DETECTED_PORT (timeout ${timeout}s)"
    python3 "$CAPTURE" "$DETECTED_PORT" "$baud" "$timeout" &
    cap=$!
    sleep 1
    echo ">> flashing $board (st-link $DETECTED_SERIAL)"
    st-flash --serial "$DETECTED_SERIAL" --reset write "$BUILD_DIR/tests.bin" "$flash_addr" >/dev/null 2>&1
  fi

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
    run_board "$b" || { c=$?; [ "$c" = 77 ] || overall=$((overall + c)); }
  done
  echo "=================================================================="
  echo ">> HIL --all aggregate failure count: $overall"
  exit "$overall"
fi

run_board "$1"
