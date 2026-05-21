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

RENODE_PID=""
WATCHER_PID=""
TAIL_PID=""
cleanup() {
  [[ -n "$WATCHER_PID" ]] && kill "$WATCHER_PID" 2>/dev/null || true
  [[ -n "$RENODE_PID"  ]] && kill "$RENODE_PID"  2>/dev/null || true
  [[ -n "$TAIL_PID"    ]] && kill "$TAIL_PID"    2>/dev/null || true
  rm -f "$LOGFILE"
}
trap cleanup EXIT INT TERM

echo ">> Renode: booting F401RE with $ELF"
echo ">> uart log → $LOGFILE"
echo ">> ===== live UART output ====="

# Pre-create the log file so `tail -F` has something to follow from t=0,
# then stream it in the background. The user sees the navtest output as
# the emulated firmware writes it (instead of staring at a black box).
: > "$LOGFILE"
tail -F -q --pid=$$ "$LOGFILE" 2>/dev/null &
TAIL_PID=$!

# Renode >= 1.15 doesn't accept the legacy `--variable name=value` flag;
# variables are now set through the Monitor via `-e "$name = value"` and
# the script is included via `i @path`. `--hide-log` suppresses Renode's
# own info-level chatter; the `.resc` raises logLevel on noisy peripherals.
#
# We run Renode headless (no `--console`) and in the background so the
# wrapper can watch the UART log and SIGTERM Renode the instant the suite
# reports its summary. Without that, the `RunFor` budget inside the .resc
# keeps Renode emulating the firmware's post-main idle loop until the full
# 3 emulated hours elapse — looking exactly like "Renode hangs at the end."
renode \
  --disable-xwt \
  --hide-log \
  -e "\$bin = @$ELF; \$logfile = @$LOGFILE; i @$RESC" \
  >/dev/null 2>&1 &
RENODE_PID=$!

# Watcher: poll the UART log for the navtest summary line. When it shows
# up, give tail a beat to flush the last few lines, then SIGTERM Renode.
# If the suite never reports a summary (hang / crash), the loop simply
# exits when Renode does on its own (via the .resc's RunFor backstop).
(
  while kill -0 "$RENODE_PID" 2>/dev/null; do
    if grep -q "Total failures:" "$LOGFILE" 2>/dev/null; then
      sleep 0.5
      kill -TERM "$RENODE_PID" 2>/dev/null || true
      break
    fi
    sleep 0.5
  done
) &
WATCHER_PID=$!

# Wait for Renode to exit (either via the watcher's SIGTERM on success,
# or naturally when the RunFor budget expires on a hang).
wait "$RENODE_PID" 2>/dev/null || true
RENODE_PID=""

# Let tail flush the final lines before we tear it down.
sleep 0.3
kill "$WATCHER_PID" 2>/dev/null || true
WATCHER_PID=""
kill "$TAIL_PID" 2>/dev/null || true
TAIL_PID=""
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
