#!/usr/bin/env bash
# Run the on-target navtest ELF inside Renode, capture USART2, exit on
# the suite's failure count.
#
# Usage:
#   tools/renode/run_tests.sh <path-to-tests.elf>
#
# Requires `renode` on PATH (headless build is fine).

set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <tests.elf>" >&2
  exit 2
fi

ELF="$(realpath "$1")"
if [[ ! -f "$ELF" ]]; then
  echo "error: ELF not found: $ELF" >&2
  exit 2
fi

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
RESC="$REPO_ROOT/tools/renode/navhal_f401re.resc"
LOGFILE="$(mktemp -t navhal-uart-XXXXXX.log)"

trap 'rm -f "$LOGFILE"' EXIT

echo ">> Renode: booting F401RE with $ELF"
echo ">> uart log → $LOGFILE"
echo ">> ===== live UART output ====="

# Pre-create the log file so `tail -F` has something to follow from t=0,
# then stream it in the background. The user sees the navtest output as
# the emulated firmware writes it (instead of staring at a black box).
: > "$LOGFILE"
tail -F -q --pid=$$ "$LOGFILE" 2>/dev/null &
TAIL_PID=$!
trap 'kill "$TAIL_PID" 2>/dev/null; rm -f "$LOGFILE"' EXIT

# Renode >= 1.15 doesn't accept the legacy `--variable name=value` flag;
# variables are now set through the Monitor via `-e "$name = value"` and
# the script is included via `i @path`. `--hide-log` suppresses Renode's
# own info-level chatter (peripheral instantiation, load messages); the
# `.resc` raises logLevel on the noisy peripherals separately.
renode \
  --disable-xwt \
  --console \
  --hide-log \
  -e "\$bin = @$ELF; \$logfile = @$LOGFILE; i @$RESC"

# Give tail a beat to flush the final lines before we kill it.
sleep 0.5
kill "$TAIL_PID" 2>/dev/null || true
echo ">> ===== end UART output ====="

# The on-target main.c prints "Total failures:  <N>" at the end.
# Treat missing summary as failure (probably timed out before completion).
if ! grep -q "Total failures:" "$LOGFILE"; then
  echo "error: navtest summary not found in UART log — emulation may have timed out" >&2
  exit 1
fi

FAILURES=$(grep "Total failures:" "$LOGFILE" | tail -1 | awk '{print $3}')
echo ">> failures reported: $FAILURES"
exit "$FAILURES"
