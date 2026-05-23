#!/usr/bin/env python3
"""Read the navtest UART log from a NavHAL on-target run.

Connects to the given serial port (default `/dev/ttyACM0` — the Nucleo
ST-Link Virtual COM Port) and copies bytes to stdout until the
`Total failures: <N>` line printed by `tests/main.c` arrives, then
exits with that failure count as the exit code. On timeout, exits 124.

Usage:
    tools/uart_capture.py [port] [baud] [timeout_seconds]

Defaults:
    port    = /dev/ttyACM0
    baud    = 9600
    timeout = 120  (seconds)

Requires:  python3-serial  (apt: python3-serial,  pip: pyserial)
"""

import re
import sys
import time

try:
    import serial
except ImportError:  # pragma: no cover
    sys.stderr.write(
        "uart_capture.py: pyserial not installed.  apt install python3-serial\n"
    )
    sys.exit(2)


def main() -> int:
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 9600
    timeout_s = int(sys.argv[3]) if len(sys.argv) > 3 else 120

    summary_re = re.compile(rb"Total failures:\s*(\d+)")

    ser = serial.Serial(port, baud, timeout=1)
    deadline = time.time() + timeout_s
    buf = b""

    while time.time() < deadline:
        chunk = ser.read(1024)
        if not chunk:
            continue
        sys.stdout.buffer.write(chunk)
        sys.stdout.flush()
        buf += chunk
        m = summary_re.search(buf)
        if not m:
            continue
        # Drain any trailing bytes so the log is complete before we exit.
        time.sleep(0.2)
        extra = ser.read(2048)
        if extra:
            sys.stdout.buffer.write(extra)
            sys.stdout.flush()
        failures = int(m.group(1))
        sys.stderr.write(
            f"\n[uart_capture: detected summary, failures={failures}]\n"
        )
        return failures

    sys.stderr.write("\n[uart_capture: timed out before seeing summary]\n")
    return 124


if __name__ == "__main__":
    sys.exit(main())
