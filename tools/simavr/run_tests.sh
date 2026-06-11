#!/usr/bin/env bash
# Runs the AVR test ELF in simavr and scrapes the navtest "Total failures:"
# summary from the emulator's UART (USART0 → simavr stdout by default).
#
# Mirrors tools/renode/run_tests.sh in shape: same exit-code semantics
# (returns the failure count, 0 = green) and same "give up if the
# summary line never shows" timeout.
#
# Usage: tools/simavr/run_tests.sh <path/to/tests-elf>
# Defaults: -m atmega328p -f 16000000  (matches CMakeLists.txt AVR_F_CPU)

set -euo pipefail

ELF=${1:-build-test-avr/tests}
MCU=${SIMAVR_MCU:-atmega328p}
FREQ=${SIMAVR_FREQ:-16000000}
TIMEOUT_SEC=${SIMAVR_TIMEOUT:-120}

command -v simavr >/dev/null || { echo "error: simavr not on PATH (apt install simavr)" >&2; exit 2; }
[ -f "$ELF" ] || { echo "error: '$ELF' not found" >&2; exit 2; }

LOGFILE=$(mktemp)
cleanup() { rm -f "$LOGFILE"; }
trap cleanup EXIT

echo ">> simavr -m $MCU -f $FREQ $ELF (timeout ${TIMEOUT_SEC}s)"
simavr -m "$MCU" -f "$FREQ" "$ELF" > "$LOGFILE" 2>&1 &
PID=$!

# Poll the log for the summary line, kill simavr once it shows or the
# timeout expires. simavr doesn't auto-exit when main() returns on AVR
# (return from main lands in a trap loop), so we have to terminate it.
deadline=$((SECONDS + TIMEOUT_SEC))
while [ $SECONDS -lt $deadline ]; do
  if grep -q "Total failures:" "$LOGFILE"; then
    break
  fi
  sleep 1
done

kill -INT  "$PID" 2>/dev/null || true
sleep 0.3
kill -KILL "$PID" 2>/dev/null || true
wait "$PID" 2>/dev/null || true

echo ">> ===== begin UART output ====="
# Strip ANSI escape codes for readability in CI logs.
sed -E 's/\x1B\[[0-9;]*[A-Za-z]//g' "$LOGFILE"
echo ">> ===== end UART output ====="

if ! grep -q "Total failures:" "$LOGFILE"; then
  echo "error: navtest summary not found — emulation hit ${TIMEOUT_SEC}s timeout before tests completed" >&2
  exit 1
fi

# Strip ANSI escapes + CRs before parsing — navtest uses colored output,
# so $3 would otherwise be "0\x1B[…]" and exit would fail as non-numeric.
# `grep -oE '[0-9]+' | tail -1` then pulls the count out unambiguously.
FAILURES=$(sed -E 's/\x1B\[[0-9;]*[A-Za-z]//g' "$LOGFILE" \
           | tr -d '\r' \
           | grep "Total failures:" \
           | tail -1 \
           | grep -oE '[0-9]+' \
           | tail -1)
echo ">> failures reported: $FAILURES"
exit "$FAILURES"
