#!/usr/bin/env bash
# Build, flash, and run the NavHAL on-target test suite on a connected
# Nucleo-F401RE. Captures USART2 to stdout and exits on the navtest
# "Total failures" line.
#
# Usage:
#   tools/run_target_tests.sh [port] [baud] [timeout_seconds]
#
# Defaults:
#   port    = /dev/ttyACM0      (ST-Link Virtual COM Port)
#   baud    = 9600
#   timeout = 120
#
# Requirements:
#   - arm-none-eabi-gcc on PATH
#   - st-flash (from stlink-tools)
#   - python3 with pyserial

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build-test"
PORT="${1:-/dev/ttyACM0}"
BAUD="${2:-9600}"
TIMEOUT="${3:-120}"

echo ">> configuring  $BUILD_DIR (TEST=ON)"
cmake -B "$BUILD_DIR" -DTEST=ON "$REPO_ROOT" >/dev/null

echo ">> building tests"
cmake --build "$BUILD_DIR" --target tests -j >/dev/null
arm-none-eabi-size "$BUILD_DIR/tests"

if [[ ! -e "$PORT" ]]; then
  echo "error: serial port $PORT not present — is the Nucleo plugged in?" >&2
  exit 2
fi

echo ">> configuring serial line  $PORT @ $BAUD"
stty -F "$PORT" "$BAUD" cs8 -cstopb -parenb raw -echo

# Start the UART reader BEFORE the flash reset so we don't miss the
# startup banner.
echo ">> starting UART capture (timeout ${TIMEOUT}s)"
python3 "$REPO_ROOT/tools/uart_capture.py" "$PORT" "$BAUD" "$TIMEOUT" &
CAPTURE_PID=$!

# Give the reader a moment to open the port, then flash. The flash
# performs a software reset, which kicks off the test run.
sleep 1
echo ">> flashing to board"
cmake --build "$BUILD_DIR" --target flash_tests >/dev/null

# Propagate the capture exit code (0 = all green, N = failure count,
# 124 = capture timed out before the summary arrived).
wait "$CAPTURE_PID"
EXIT_CODE=$?
echo ">> uart_capture exited with $EXIT_CODE"
exit "$EXIT_CODE"
