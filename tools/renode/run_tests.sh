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
# Per-board Renode script. Defaults to the F401RE; a board's PIL conf can
# override via the RESC env var (e.g. tools/pil/boards/nucleo_f767zi.conf).
# A relative override is resolved against the repo root.
RESC="${RESC:-$REPO_ROOT/tools/renode/navhal_f401re.resc}"
case "$RESC" in /*) ;; *) RESC="$REPO_ROOT/$RESC" ;; esac
LOGFILE="$(mktemp -t navhal-uart-XXXXXX.log)"

RENODE_PID=""
RENODE_PGID=""
WATCHER_PID=""
TAIL_PID=""

# Stop Renode and the dotnet/mono child it spawns. When we launched it under
# its own process group (setsid) we signal the whole group, so the child can't
# be orphaned (a lingering 'dotnet' otherwise survives into CI teardown);
# otherwise we fall back to signalling just the launcher pid.
stop_renode() {
  if [[ -n "$RENODE_PGID" ]]; then
    kill -TERM -- "-$RENODE_PGID" 2>/dev/null || true
  elif [[ -n "$RENODE_PID" ]]; then
    kill -TERM "$RENODE_PID" 2>/dev/null || true
  fi
}

cleanup() {
  [[ -n "$WATCHER_PID" ]] && kill "$WATCHER_PID" 2>/dev/null || true
  stop_renode
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
# Run Renode in its own process group (setsid) so the cleanup above can reap
# the whole tree — the launcher plus its dotnet/mono child. setsid ships with
# util-linux on every CI runner and dev box we target; degrade to a plain
# background run (launcher-pid signalling only) if it is somehow unavailable.
if command -v setsid >/dev/null 2>&1; then
  setsid renode \
    --disable-xwt \
    --hide-log \
    -e "\$bin = @$ELF; \$logfile = @$LOGFILE; i @$RESC" \
    >/dev/null 2>&1 &
  RENODE_PID=$!
  RENODE_PGID=$RENODE_PID   # setsid makes Renode its own process-group leader
else
  renode \
    --disable-xwt \
    --hide-log \
    -e "\$bin = @$ELF; \$logfile = @$LOGFILE; i @$RESC" \
    >/dev/null 2>&1 &
  RENODE_PID=$!
fi

# Watcher: poll the UART log for the navtest summary line. When it shows
# up, give tail a beat to flush the last few lines, then SIGTERM Renode.
# If the suite never reports a summary (hang / crash), the loop simply
# exits when Renode does on its own (via the .resc's RunFor backstop).
(
  while kill -0 "$RENODE_PID" 2>/dev/null; do
    if grep -q "Total failures:" "$LOGFILE" 2>/dev/null; then
      sleep 0.5
      stop_renode
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

## The on-target firmware prints via UART with \r\n line endings, so awk's
## third field ends in a stray CR. `$(...)` strips trailing LF but not CR,
## which would make `exit "0\r"` fail with "numeric argument required" (exit
## 2) — masking a 128/128 pass as a CI red. tr -d '\r' strips it.
FAILURES=$(grep "Total failures:" "$LOGFILE" | tail -1 | awk '{print $3}' | tr -d '\r')
echo ">> failures reported: $FAILURES"
exit "$FAILURES"
