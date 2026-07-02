#!/usr/bin/env python3
# Copyright (C) 2025 NAVRobotec Pvt Ltd
# Author: Ragnar Vallhala
# SPDX-License-Identifier: Apache-2.0
#
# Host side of the UART<->Ethernet chat bridge (samples/cortex-m/30_hal_eth_bridge).
#
# Opens a raw L2 socket on an Ethernet interface and talks to the board's bridge
# firmware over EtherType 0x88B5: a line typed here (Enter) is framed and sent to
# the board (which prints it to its UART / minicom), and frames the board sends
# (lines typed in its minicom) are printed here.
#
# Needs raw-socket privileges:  sudo python3 chat.py [iface]   (default: enp2s0)
# Point minicom at the board's VCP in another terminal:  minicom -D /dev/ttyACM0 -b 9600

import fcntl
import os
import select
import socket
import struct
import sys

ETH_TYPE = 0x88B5
BOARD_MAC = b"\x02\x00\x00\x00\x00\x01"
MIN_FRAME = 60


def iface_mac(sock, iface):
    # SIOCGIFHWADDR = 0x8927
    info = fcntl.ioctl(sock.fileno(), 0x8927, struct.pack("256s", iface.encode()[:15]))
    return info[18:24]


def build_frame(src_mac, text):
    payload = text.encode("utf-8", "replace")
    body = struct.pack("!H", len(payload)) + payload
    frame = BOARD_MAC + src_mac + struct.pack("!H", ETH_TYPE) + body
    if len(frame) < MIN_FRAME:
        frame += b"\x00" * (MIN_FRAME - len(frame))
    return frame


def main():
    iface = sys.argv[1] if len(sys.argv) > 1 else "enp2s0"
    try:
        sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(ETH_TYPE))
        sock.bind((iface, 0))
    except PermissionError:
        sys.exit("need root for a raw socket: sudo python3 chat.py " + iface)
    except OSError as e:
        sys.exit(f"cannot open {iface}: {e}")
    src_mac = iface_mac(sock, iface)

    print(f"[chat] bridged over {iface} (EtherType 0x{ETH_TYPE:04x}). Type + Enter; Ctrl-C to quit.")
    sys.stdout.write("> ")
    sys.stdout.flush()

    while True:
        r, _, _ = select.select([sock, sys.stdin], [], [])
        if sock in r:
            frame = sock.recv(2048)
            if len(frame) < 16:
                continue
            src = frame[6:12]
            etype = struct.unpack("!H", frame[12:14])[0]
            if etype != ETH_TYPE or src != BOARD_MAC:
                continue  # ignore our own outgoing frames and other traffic
            plen = struct.unpack("!H", frame[14:16])[0]
            text = frame[16:16 + plen].decode("utf-8", "replace")
            sys.stdout.write("\r[board] " + text + "\n> ")
            sys.stdout.flush()
        if sys.stdin in r:
            line = sys.stdin.readline()
            if not line:  # EOF
                break
            line = line.rstrip("\n")
            if line:
                sock.send(build_frame(src_mac, line))
            sys.stdout.write("> ")
            sys.stdout.flush()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print()
