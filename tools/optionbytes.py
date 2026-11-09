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
"""Provisioning: the option bytes a shipped STM32F4 unit needs.

Sets write protection on the stage-1 sectors, the brown-out level, and read
protection Level 1. It refuses Level 2, because the rule that this project stops
at Level 1 belongs in the tool that would otherwise make the mistake rather than
only in a document -- Level 2 is the one step here that no mass erase undoes, and
it also permanently ends JTAG/SWD on the part.

Everything else it does is reversible at the cost of a mass erase: lifting RDP1
erases flash, which loses the firmware and the key-value store but returns a
usable board. A returned unit can be examined; a provisioning mistake costs a
reflash.

The probe is addressed by USB location rather than by serial, because finding a
probe by serial means opening every ST-Link on the bus and a bench with several
boards on it does not want that.

  tools/optionbytes.py --location 3-1 show
  tools/optionbytes.py --location 3-1 set --wrp-stage1 --bor 3 --commit
  tools/optionbytes.py --location 3-1 set --rdp1 --commit      # ships the unit
  tools/optionbytes.py --location 3-1 unlock --commit          # mass erase, back to RDP0
"""

import argparse
import re
import subprocess
import sys

# RM0368 3.8: the option bytes live in a register, not in flash.
OPTCR = 0x40023C14
OPTKEYR = 0x40023C08
FLASH_SR = 0x40023C0C
OPTKEY1 = 0x08192A3B
OPTKEY2 = 0x4C5D6E7F

RDP_LEVEL0 = 0xAA
RDP_LEVEL1 = 0x55  # "any value other than 0xAA or 0xCC"; ST's own HAL uses 0x55
RDP_LEVEL2 = 0xCC  # refused -- see the module docstring

# F401RE has 8 sectors, so nWRP occupies OPTCR bits 23:16. A zero bit means the
# sector is write protected, which is the opposite of what the name suggests.
NWRP_SHIFT = 16
STAGE1_SECTORS = (0, 1)


def openocd(location, commands, target="stm32f4x"):
    """Run one openocd session with the given commands, return its output."""
    argv = ["openocd", "-f", "interface/stlink.cfg",
            "-c", f"adapter usb location {location}",
            "-f", f"target/{target}.cfg", "-c", "init"]
    for c in commands:
        argv += ["-c", c]
    argv += ["-c", "shutdown"]
    p = subprocess.run(argv, capture_output=True, text=True, timeout=120)
    out = p.stdout + p.stderr
    if "Error:" in out and "mdw" not in " ".join(commands):
        raise RuntimeError(f"openocd failed:\n{out}")
    return out


def read_word(location, addr):
    out = openocd(location, [f"mdw 0x{addr:08X}"])
    m = re.search(rf"0x{addr:08x}:\s*([0-9a-f]{{8}})", out, re.I)
    if not m:
        raise RuntimeError(f"could not read 0x{addr:08X} from:\n{out}")
    return int(m.group(1), 16)


def describe(optcr):
    rdp = (optcr >> 8) & 0xFF
    level = {RDP_LEVEL0: "0 (no protection)", RDP_LEVEL2: "2 (PERMANENT, refused by this tool)"}
    nwrp = (optcr >> NWRP_SHIFT) & 0xFF
    protected = [s for s in range(8) if not (nwrp >> s) & 1]
    bor = (optcr >> 2) & 0x3
    # BOR_LEV is inverted: 3 = off, 0 = the highest threshold.
    bor_v = {0: "2.70-3.60 V (level 3)", 1: "2.40-2.70 V (level 2)",
             2: "2.10-2.40 V (level 1)", 3: "off"}
    print(f"  OPTCR          0x{optcr:08X}")
    print(f"  RDP            {level.get(rdp, f'1 (protected, 0x{rdp:02X})')}")
    print(f"  write-protect  {protected if protected else 'none'}")
    print(f"  BOR            {bor_v[bor]}")
    print(f"  OPTLOCK        {'locked' if optcr & 1 else 'unlocked'}")


def write_optcr(location, value):
    """Unlock the option bytes, write OPTCR, start the programming, wait."""
    cmds = [
        "reset halt",
        f"mww 0x{OPTKEYR:08X} 0x{OPTKEY1:08X}",
        f"mww 0x{OPTKEYR:08X} 0x{OPTKEY2:08X}",
        f"mww 0x{OPTCR:08X} 0x{value & ~0x3:08X}",   # value with OPTSTRT clear
        f"mww 0x{OPTCR:08X} 0x{(value & ~0x1) | 0x2:08X}",  # OPTSTRT, OPTLOCK clear
        f"mdw 0x{FLASH_SR:08X}",
        # Re-lock. Leaving OPTLOCK clear leaves the option bytes writable by
        # anything that can reach the bus, which undoes the point of setting
        # them -- and the first run of this tool did exactly that.
        f"mww 0x{OPTCR:08X} 0x{(value | 0x1) & ~0x2:08X}",
        f"mdw 0x{OPTCR:08X}",
    ]
    return openocd(location, cmds)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--location", required=True,
                    help="probe USB location, e.g. 3-1 (never a serial: that means a bus scan)")
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("show", help="read and explain the current option bytes")
    s = sub.add_parser("set", help="change the option bytes")
    s.add_argument("--wrp-stage1", action="store_true",
                   help="write protect sectors 0-1 (stage-1)")
    s.add_argument("--no-wrp", action="store_true", help="clear all write protection")
    s.add_argument("--bor", type=int, choices=[0, 1, 2, 3],
                   help="BOR_LEV field: 0 is the highest threshold, 3 is off")
    s.add_argument("--rdp1", action="store_true", help="set read protection Level 1")
    s.add_argument("--rdp2", action="store_true", help=argparse.SUPPRESS)
    s.add_argument("--commit", action="store_true",
                   help="actually write; without it the change is only printed")
    u = sub.add_parser("unlock", help="mass erase back to RDP0 -- loses flash and the KV store")
    u.add_argument("--commit", action="store_true")
    a = ap.parse_args()

    if a.cmd == "show":
        describe(read_word(a.location, OPTCR))
        return 0

    if a.cmd == "unlock":
        if not a.commit:
            print("  would mass erase the part and return it to RDP0.")
            print("  Everything in flash is lost, including the key-value store.")
            print("  Re-run with --commit.")
            return 0
        print("  mass erasing...")
        openocd(a.location, ["reset halt", "stm32f2x unlock 0", "reset halt",
                             "stm32f2x mass_erase 0"])
        print("  done; the part is at RDP0 and empty. Reflash stage-1.")
        return 0

    if a.rdp2:
        print("Refused: RDP Level 2 is permanent -- no mass erase lifts it and SWD is",
              file=sys.stderr)
        print("gone for good. This project ships at Level 1 and has no tier above it.",
              file=sys.stderr)
        return 2

    before = read_word(a.location, OPTCR)
    print("current:")
    describe(before)

    value = before
    if a.wrp_stage1:
        for s_ in STAGE1_SECTORS:
            value &= ~(1 << (NWRP_SHIFT + s_))   # 0 = protected
    if a.no_wrp:
        value |= 0xFF << NWRP_SHIFT
    if a.bor is not None:
        value = (value & ~(0x3 << 2)) | ((a.bor & 0x3) << 2)
    if a.rdp1:
        value = (value & ~(0xFF << 8)) | (RDP_LEVEL1 << 8)

    if value == before:
        print("\nnothing to change.")
        return 0

    print("\nwould become:")
    describe(value)

    if not a.commit:
        print("\n  Not written. Re-run with --commit.")
        if a.rdp1:
            print("  Note: RDP1 blocks SWD access to flash. Lifting it means a mass")
            print("  erase, which loses the firmware and the KV store.")
        return 0

    write_optcr(a.location, value)
    after = read_word(a.location, OPTCR)
    print("\nnow:")
    describe(after)
    if (after >> 8) & 0xFF != (value >> 8) & 0xFF or \
       (after >> NWRP_SHIFT) & 0xFF != (value >> NWRP_SHIFT) & 0xFF:
        print("\n  WARNING: the part did not take the value asked for.", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
