#!/usr/bin/env python3
#
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
#/
"""Push an image into stage-1's recovery, over UART or CDC.

The other end is boot/stage1/recovery.c; the frame format is in recovery.h and
this file is deliberately the only other place it is written down.

    tools/recover.py --port /dev/ttyACM0 --image stage2.img
    tools/recover.py --port /dev/ttyACM0 --info

Exits 0 when the board reports the image verifies, 1 otherwise. The board resets
itself on --run, which is the normal end of a session: the usual boot path then
re-decides, so success here is the same check that will gate the next boot.
"""
import argparse
import binascii
import sys
import time

import serial

SYNC = b"NH"
CMD_INFO, CMD_ERASE, CMD_WRITE, CMD_VERIFY, CMD_RUN = b"I", b"E", b"W", b"V", b"R"
ACK, NAK = 0x4B, 0x4E
CHUNK = 256

ERRS = {
    0x00: "none",
    0x01: "frame CRC mismatch",
    0x02: "unknown command",
    0x03: "bad payload length",
    0x04: "write outside the partition",
    0x05: "flash erase or program failed",
    0x06: "image does not verify",
    0x07: "write before erase",
}


def frame(cmd: bytes, payload: bytes = b"") -> bytes:
    head = cmd + len(payload).to_bytes(2, "little")
    # STM32 hardware CRC-32, which is the Ethernet polynomial with no final XOR
    # and no reflection -- not zlib's crc32, so it is computed here rather than
    # borrowed.
    crc = stm32_crc32(head + payload)
    return SYNC + head + payload + crc.to_bytes(4, "little")


def stm32_crc32(data: bytes) -> int:
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte << 24
        for _ in range(8):
            crc = ((crc << 1) ^ 0x04C11DB7) & 0xFFFFFFFF if crc & 0x80000000 \
                else (crc << 1) & 0xFFFFFFFF
    return crc


def exchange(ser, cmd, payload=b"", what="", timeout=None):
    ser.write(frame(cmd, payload))
    ser.flush()
    if timeout is not None:
        # An erase is not a quick reply: the app partition is three 128 KiB
        # sectors and each can approach two seconds on this part. The board kicks
        # its watchdog throughout; the host has to be willing to wait.
        was, ser.timeout = ser.timeout, timeout
        try:
            reply = ser.read(2)
        finally:
            ser.timeout = was
    else:
        reply = ser.read(2)
    if len(reply) != 2:
        print(f"  {what}: no reply")
        return None
    status, code = reply[0], reply[1]
    if status == ACK:
        return code
    print(f"  {what}: refused -- {ERRS.get(code, hex(code))}")
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--image")
    ap.add_argument("--info", action="store_true")
    ap.add_argument("--no-run", action="store_true",
                    help="leave the board in recovery instead of resetting it")
    args = ap.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=5)
    # The board may have been printing before this; anything already buffered is
    # not a reply to us.
    ser.reset_input_buffer()

    version = exchange(ser, CMD_INFO, what="info")
    if version is None:
        return 1
    print(f"  stage-1 recovery protocol v{version}")
    if args.info:
        return 0

    if not args.image:
        print("  nothing to do: pass --image or --info")
        return 1

    body = open(args.image, "rb").read()
    print(f"  image {args.image}: {len(body)} bytes")

    if exchange(ser, CMD_ERASE, what="erase", timeout=30) is None:
        return 1
    print("  erased")

    sent = 0
    while sent < len(body):
        chunk = body[sent:sent + CHUNK]
        payload = sent.to_bytes(4, "little") + chunk
        if exchange(ser, CMD_WRITE, payload, what=f"write at {sent}") is None:
            return 1
        sent += len(chunk)
    print(f"  wrote {sent} bytes")

    # Verify hashes the whole body, which is sub-second here but scales.
    if exchange(ser, CMD_VERIFY, what="verify", timeout=15) is None:
        return 1
    print("  the board verifies the image it now holds")

    if not args.no_run:
        # No reply is parsed for RUN. The board acks and resets in the same
        # breath, so by the time the ack would arrive its console banner is
        # already in the stream -- reading two bytes there returns b"\r\n" and
        # looks like a refusal. Fire and let the next boot speak for itself.
        ser.write(frame(CMD_RUN))
        ser.flush()
        print("  reset requested; the normal boot path decides from here")
    return 0


if __name__ == "__main__":
    sys.exit(main())
