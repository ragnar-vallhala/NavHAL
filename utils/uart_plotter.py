#!/usr/bin/env python3
"""
uart_plotter.py - Live plot numeric data coming from a UART/serial device.

Usage examples:
    python uart_plotter.py --port /dev/ttyUSB0 --baud 115200 --channels 1
    python uart_plotter.py --port COM3 --baud 9600 --channels 3 --delimiter "," --window 500
    python uart_plotter.py --test --channels 2   # simulate data, no serial required
"""

import argparse
import sys
import threading
import time
from collections import deque

import matplotlib.pyplot as plt
import matplotlib.animation as animation

# optional dependency; only required if not using --test
try:
    import serial
except Exception:
    serial = None


def parse_line_to_floats(line: str, delim: str = None):
    """
    Parse a line into float values.
    If delim is None, split on whitespace; otherwise split on delim.
    Returns list of floats (may be empty).
    """
    if delim:
        parts = line.strip().split(delim)
    else:
        parts = line.strip().split()
    floats = []
    for p in parts:
        if p == "":
            continue
        # allow trailing/leading commas/characters, try to sanitize
        try:
            f = float(p)
            floats.append(f)
        except ValueError:
            # try to strip non-numeric chars (common on microcontrollers: \r or extra chars)
            try:
                cleaned = ''.join(ch for ch in p if (ch.isdigit() or ch in '.-+eE'))
                if cleaned:
                    floats.append(float(cleaned))
            except Exception:
                continue
    return floats


class UARTReader(threading.Thread):
    """
    Thread that reads lines from serial (or generates simulated data) and pushes parsed floats
    into a queue (list of lists). Thread-safe append via provided callback.
    """
    def __init__(self, port, baud, delim, channels, callback, test_mode=False):
        super().__init__(daemon=True)
        self.port = port
        self.baud = baud
        self.delim = delim
        self.channels = channels
        self.callback = callback
        self.test_mode = test_mode
        self._stop = threading.Event()
        self.serial = None

    def stop(self):
        self._stop.set()

    def run(self):
        if self.test_mode:
            self._run_test_mode()
            return

        if serial is None:
            print("pyserial not found. Install with: pip install pyserial", file=sys.stderr)
            return

        try:
            self.serial = serial.Serial(self.port, self.baud, timeout=1)
        except Exception as e:
            print(f"Failed to open serial port {self.port}: {e}", file=sys.stderr)
            return

        with self.serial:
            while not self._stop.is_set():
                try:
                    raw = self.serial.readline().decode(errors='ignore')
                except Exception:
                    # fallback: read bytes and convert
                    try:
                        raw = self.serial.readline().decode('ascii', errors='ignore')
                    except Exception:
                        raw = ''
                if not raw:
                    continue
                vals = parse_line_to_floats(raw, self.delim)
                if not vals:
                    continue
                # If fewer values than channels, pad with last known / zeros
                if len(vals) < self.channels:
                    vals += [0.0] * (self.channels - len(vals))
                # truncate if more
                vals = vals[:self.channels]
                self.callback(vals)

    def _run_test_mode(self):
        t = 0.0
        import math
        while not self._stop.is_set():
            # generate channels many sinusoids with small noise
            vals = []
            for ch in range(self.channels):
                v = 1.0 * math.sin(t * (0.5 + ch * 0.2)) + 0.1 * (ch + 1) * math.sin(t * 3.7)
                vals.append(v)
            self.callback(vals)
            t += 0.05
            time.sleep(0.05)


def main():
    p = argparse.ArgumentParser(description="Live UART numeric plotter")
    p.add_argument("--port", "-p", type=str, default=None, help="Serial port (e.g. /dev/ttyUSB0 or COM3)")
    p.add_argument("--baud", "-b", type=int, default=115200, help="Baud rate")
    p.add_argument("--channels", "-c", type=int, default=1, help="Number of numeric channels per line")
    p.add_argument("--delimiter", "-d", type=str, default=None,
                   help="Delimiter for numbers in a line (default: any whitespace). e.g. ','")
    p.add_argument("--window", "-w", type=int, default=200, help="Samples to keep in rolling window")
    p.add_argument("--test", action="store_true", help="Test mode: generate simulated data instead of reading serial")
    p.add_argument("--title", type=str, default="UART Live Plot", help="Plot title")
    p.add_argument("--scatter", action="store_true", help="Use scatter plot instead of line plot")
    args = p.parse_args()

    if not args.test and args.port is None:
        p.error("Either --port must be provided, or use --test for simulated data.")

    # Data buffers
    buffers = [deque([0.0] * args.window, maxlen=args.window) for _ in range(args.channels)]
    xdata = deque([i for i in range(-args.window + 1, 1)], maxlen=args.window)

    def push_vals(vals):
        for i, v in enumerate(vals):
            buffers[i].append(v)

    reader = UARTReader(args.port, args.baud, args.delimiter, args.channels, callback=push_vals, test_mode=args.test)
    reader.start()

    # Set up plot
    plt.style.use('default')
    fig, ax = plt.subplots()
    plots = []
    for ch in range(args.channels):
        if args.scatter:
            sc = ax.scatter(list(xdata), list(buffers[ch]), label=f"ch{ch}", s=10)
            plots.append(sc)
        else:
            (ln,) = ax.plot(list(xdata), list(buffers[ch]), label=f"ch{ch}")
            plots.append(ln)

    ax.set_xlabel("Samples")
    ax.set_ylabel("Value")
    ax.set_title(args.title)
    if args.channels <= 6:
        ax.legend(loc='upper right')

    def update(frame):
        xs = list(range(len(buffers[0])))
        for ch in range(args.channels):
            if args.scatter:
                # scatter expects Nx2 array for offsets
                plots[ch].set_offsets(list(zip(xs, list(buffers[ch]))))
            else:
                plots[ch].set_data(xs, list(buffers[ch]))
        ax.relim()
        ax.autoscale_view()
        return plots

    ani = animation.FuncAnimation(fig, update, interval=50, blit=False)

    try:
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        reader.stop()
        reader.join(timeout=1)

if __name__ == "__main__":
    main()
