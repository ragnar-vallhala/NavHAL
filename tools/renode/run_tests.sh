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

# Renode >= 1.15 doesn't accept the legacy `--variable name=value` flag;
# variables are now set through the Monitor via `-e "$name = value"` and
# the script is included via `i @path`.
renode \
  --disable-xwt \
  --console \
  -e "\$bin = @$ELF; \$logfile = @$LOGFILE; i @$RESC"

echo
echo ">> ===== captured UART log ====="
cat "$LOGFILE"
echo ">> ===== end UART log ====="
echo

# The on-target main.c prints "Total failures:  <N>" at the end.
# Treat missing summary as failure (probably timed out before completion).
if ! grep -q "Total failures:" "$LOGFILE"; then
  echo "error: navtest summary not found in UART log — emulation may have timed out" >&2
  exit 1
fi

FAILURES=$(grep "Total failures:" "$LOGFILE" | tail -1 | awk '{print $3}')
echo ">> failures reported: $FAILURES"
exit "$FAILURES"
