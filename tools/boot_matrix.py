#!/usr/bin/env python3
# Copyright (C) 2025 NAVRobotec Pvt Ltd
# Author: Ragnar Vallhala
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""The bootloader validation matrix, run against a board.

Each case puts the board in a state and asserts what the console says about it.
The cases that matter are the refusals: a loader that boots a good image proves
very little, and the three-image chain already does that every time it starts.

What this does NOT cover is read protection. Every case here runs at RDP0,
because setting RDP1 costs a mass erase to undo -- it loses flash and the
key-value store. The RDP1 pass is the same matrix on a provisioned unit, and the
only thing it adds is that none of this depends on being able to read flash over
SWD. Signing does not depend on RDP, which is why these results stand on their
own.

  tools/boot_matrix.py --location 3-1 --port /dev/ttyACM0 --key k.sec \\
      --stage1 s1.bin --stage2 s2.bin --app app.bin
"""

import argparse
import re
import subprocess
import sys
import time

import serial


def erase_kv(loc):
    """Erase the bootloader's key-value sectors so the run starts from a known
    floor.

    Not housekeeping -- without it the rollback case passes or fails depending on
    what was last on the board. The HIL test image is 52 KiB and so occupies
    sectors 0-3, which includes the store the loader keeps at 2-3, and it leaves
    the store holding test-image code rather than records. A loader then reads no
    floor at all, treats it as zero, and accepts any version it is offered. For a
    shipped unit that never happens; on a bench that shares a board with the test
    suite it happens on every run.
    """
    subprocess.run(
        ["openocd", "-f", "interface/stlink.cfg",
         "-c", f"adapter usb location {loc}",
         "-f", "target/stm32f4x.cfg",
         "-c", "init", "-c", "reset halt",
         "-c", "flash erase_sector 0 2 3", "-c", "shutdown"],
        capture_output=True, text=True, timeout=180)


def resolve_port(loc):
    """Find the console tty that belongs to the probe at this USB location.

    Device nodes shuffle: unplug a board and ttyACM0 can become another board's
    port entirely, which is how a whole matrix run once read a different board's
    console and reported six failures with empty output. The location is the
    stable name, so the port is derived from it rather than passed in.
    """
    import glob
    import os
    want = None
    for d in glob.glob("/sys/bus/usb/devices/*/"):
        if os.path.basename(d.rstrip("/")) == loc:
            try:
                want = open(d + "serial").read().strip()
            except OSError:
                pass
    if not want:
        raise RuntimeError(f"no USB device at location {loc}")
    for tty in sorted(glob.glob("/dev/ttyACM*")):
        r = subprocess.run(["udevadm", "info", "-q", "property", "-n", tty],
                           capture_output=True, text=True, timeout=10)
        if f"ID_SERIAL_SHORT={want}" in r.stdout:
            return tty
    raise RuntimeError(f"probe {want} at {loc} has no ttyACM")


def flash(loc, path, addr):
    r = subprocess.run(
        ["openocd", "-f", "interface/stlink.cfg",
         "-c", f"adapter usb location {loc}",
         "-f", "target/stm32f4x.cfg",
         "-c", f"program {path} {addr} verify reset exit"],
        capture_output=True, text=True, timeout=180)
    if "Verified OK" not in (r.stdout + r.stderr):
        raise RuntimeError(f"flashing {path} at {addr} failed")


def reset(loc):
    subprocess.run(
        ["openocd", "-f", "interface/stlink.cfg",
         "-c", f"adapter usb location {loc}",
         "-f", "target/stm32f4x.cfg", "-c", "init; reset run; shutdown"],
        capture_output=True, text=True, timeout=90)


def console(port, seconds, stop=None):
    """Collect console output for a while, stopping early on a marker."""
    out = []
    with serial.Serial(port, 115200, timeout=1) as s:
        s.reset_input_buffer()
        end = time.time() + seconds
        while time.time() < end:
            line = s.readline().decode(errors="replace").strip()
            if line:
                out.append(line)
                if stop and stop in line:
                    break
    return out


def sign(key, partition, version, body, out):
    subprocess.run(["./build-sign/navhal_sign", "--partition", partition,
                    "--key", key, "--version", str(version),
                    "--in", body, "--out", out],
                   capture_output=True, text=True, check=True, timeout=60)


def corrupt(path, out, offset):
    """Flip one bit in a signed image. One bit is the whole point: a digest that
    only catches large damage is not a digest."""
    data = bytearray(open(path, "rb").read())
    data[offset] ^= 0x01
    open(out, "wb").write(data)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--location", required=True)
    ap.add_argument("--port", help="console tty; derived from --location when omitted")
    ap.add_argument("--key", required=True)
    ap.add_argument("--stage1", required=True)
    ap.add_argument("--stage2", required=True, help="stage-2 body, unsigned")
    ap.add_argument("--app", required=True, help="app body, unsigned")
    ap.add_argument("--tmp", default="/tmp")
    a = ap.parse_args()

    port = a.port or resolve_port(a.location)
    erase_kv(a.location)
    print(f"  console: {port}\n")
    t = a.tmp.rstrip("/")
    s2_img, app_img = f"{t}/m_s2.img", f"{t}/m_app.img"
    sign(a.key, "stage2", 3, a.stage2, s2_img)
    sign(a.key, "app", 9, a.app, app_img)
    # Body corruption: past the 0x200 header, so the signature still covers it.
    corrupt(s2_img, f"{t}/m_s2_bad.img", 0x300)
    corrupt(app_img, f"{t}/m_app_bad.img", 0x300)
    # Signature corruption: inside the 64-byte signature at 0x2C.
    corrupt(s2_img, f"{t}/m_s2_badsig.img", 0x30)
    corrupt(app_img, f"{t}/m_app_badsig.img", 0x30)
    sign(a.key, "app", 1, a.app, f"{t}/m_app_old.img")

    cases = [
        ("good chain boots the app",
         [(a.stage1, "0x08000000"), (s2_img, "0x08010000"), (app_img, "0x08020000")],
         r"app: running", 25),
        ("corrupt app body -> stage-2 takes an update",
         [(f"{t}/m_app_bad.img", "0x08020000")],
         r"stage2: update mode", 20),
        ("bad app signature -> stage-2 takes an update",
         [(f"{t}/m_app_badsig.img", "0x08020000")],
         r"stage2: update mode", 20),
        ("app older than the floor -> refused",
         [(f"{t}/m_app_old.img", "0x08020000")],
         r"rollback floor", 20),
        ("corrupt stage-2 body -> stage-1 goes to recovery",
         [(f"{t}/m_s2_bad.img", "0x08010000")],
         r"stage1: recovery", 20),
        ("bad stage-2 signature -> stage-1 goes to recovery",
         [(f"{t}/m_s2_badsig.img", "0x08010000")],
         r"stage1: recovery", 20),
    ]

    failures = 0
    for name, images, expect, secs in cases:
        for path, addr in images:
            flash(a.location, path, addr)
        reset(a.location)
        lines = console(port, secs, stop=None)
        text = "\n".join(lines)
        ok = re.search(expect, text) is not None
        print(f"  [{'PASS' if ok else 'FAIL'}] {name}")
        if not ok:
            failures += 1
            print(f"         expected /{expect}/, console said:")
            for l in lines[-6:]:
                print(f"           {l}")

    print(f"\n  {len(cases) - failures}/{len(cases)} cases passed (all at RDP0)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
