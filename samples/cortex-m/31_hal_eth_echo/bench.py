#!/usr/bin/env python3
# Copyright (C) 2025 NAVRobotec Pvt Ltd
# Author: Ragnar Vallhala
# SPDX-License-Identifier: Apache-2.0
#
# Host side of the Ethernet echo bandwidth benchmark (samples/cortex-m/31_hal_eth_echo).
#
# Sends EtherType-0x88B5 frames to the board's echo firmware, counts the frames
# it reflects back, and ramps the offered load step by step — printing offered
# vs. echoed bandwidth and loss at each level so you can see where the link (and
# the board's echo loop) saturates.
#
# Needs raw-socket privileges:  sudo python3 bench.py [iface] [--size N] [--dur S]
#   iface   Ethernet interface cabled to the board (default: enp2s0)
#   --size  payload bytes per frame (default 1024; L2 frame = size + 16)
#   --dur   seconds per load level (default 2.0)
#
# Pacing yields the GIL between small batches so the receive thread keeps
# draining — a busy-wait would starve it and report false loss.

import argparse
import ctypes
import fcntl
import socket
import struct
import sys
import threading
import time

ETH_TYPE = 0x88B5
ETYPE_BYTES = struct.pack("!H", ETH_TYPE)
BOARD_MAC = b"\x02\x00\x00\x00\x00\x01"
MIN_FRAME = 60
HDR = 16  # dst(6) + src(6) + ethertype(2) + length(2)
TICK = 0.005  # pacing quantum: send a batch, then sleep the remainder

# Offered-load ramp, in Mbit/s. 0 means "no pacing — send as fast as possible".
LEVELS_MBPS = [1, 2, 5, 10, 20, 40, 60, 80, 100, 0]


def iface_mac(sock, iface):
    info = fcntl.ioctl(sock.fileno(), 0x8927, struct.pack("256s", iface.encode()[:15]))
    return info[18:24]


# Kernel BPF that passes only frames whose source MAC == mac, so the socket
# never queues the host's own (looped-back) outgoing frames. Without it the
# receive thread wastes its time discarding tens of thousands of our own frames
# per second and under-counts the board's echoes. Returns the buffer to keep
# alive (the kernel holds a pointer into it).
def attach_src_mac_filter(sock, mac):
    hi = struct.unpack("!I", mac[0:4])[0]  # src MAC bytes 0..3 (frame offset 6)
    lo = struct.unpack("!H", mac[4:6])[0]  # src MAC bytes 4..5 (frame offset 10)
    prog = b"".join([
        struct.pack("HBBI", 0x20, 0, 0, 6),           # ld  [6]
        struct.pack("HBBI", 0x15, 0, 3, hi),          # jeq hi ? : drop
        struct.pack("HBBI", 0x28, 0, 0, 10),          # ldh [10]
        struct.pack("HBBI", 0x15, 0, 1, lo),          # jeq lo ? accept : drop
        struct.pack("HBBI", 0x06, 0, 0, 0xFFFFFFFF),  # ret 0xFFFF (accept)
        struct.pack("HBBI", 0x06, 0, 0, 0x00000000),  # ret 0 (drop)
    ])
    buf = ctypes.create_string_buffer(prog)
    fprog = struct.pack("HL", 6, ctypes.addressof(buf))
    so_attach_filter = getattr(socket, "SO_ATTACH_FILTER", 26)  # Linux SO_ATTACH_FILTER
    sock.setsockopt(socket.SOL_SOCKET, so_attach_filter, fprog)
    return buf


def make_frame(src_mac, size, tag=0xA5):
    """One frame whose payload is a single repeated byte, that byte given by tag.

    Repeating one value makes an echoed frame checkable without tracking which
    frame it was: every payload byte must equal every other. And varying the
    value between frames is what makes a stale read visible at all -- with a
    constant fill, a buffer the CPU read out of a cache line left over from an
    earlier frame holds exactly the bytes it expected, so cache staleness looks
    identical to correct behaviour.
    """
    payload = bytes([tag]) * size
    frame = BOARD_MAC + src_mac + ETYPE_BYTES + struct.pack("!H", size) + payload
    if len(frame) < MIN_FRAME:
        frame += b"\x00" * (MIN_FRAME - len(frame))
    return frame


class Receiver(threading.Thread):
    def __init__(self, sock):
        super().__init__(daemon=True)
        self.sock = sock
        self.lock = threading.Lock()
        self.count = 0
        self.bytes = 0
        self.corrupt = 0
        self.expect = None  # set per level: the payload byte being sent
        self._stop = threading.Event()

    def reset(self, expect=None):
        with self.lock:
            self.count = 0
            self.bytes = 0
            self.corrupt = 0
            self.expect = expect

    def snapshot(self):
        with self.lock:
            return self.count, self.bytes, self.corrupt

    def stop(self):
        self._stop.set()

    def run(self):
        self.sock.settimeout(0.1)
        while not self._stop.is_set():
            try:
                data, addr = self.sock.recvfrom(2048)
            except socket.timeout:
                continue
            except OSError:
                break
            if addr[2] == socket.PACKET_OUTGOING:
                continue  # our own transmit, echoed back by AF_PACKET
            if len(data) >= HDR and data[12:14] == ETYPE_BYTES and data[6:12] == BOARD_MAC:
                with self.lock:
                    self.count += 1
                    self.bytes += len(data)
                    # Payload integrity, which is what a cache-coherency bug
                    # breaks: the board would echo a frame of the right length
                    # from the right MAC carrying bytes it read stale.
                    if self.expect is not None:
                        n = struct.unpack("!H", data[14:HDR])[0]
                        body = data[HDR:HDR + n]
                        if len(body) != n or body != bytes([self.expect]) * n:
                            self.corrupt += 1


def run_level(sock, rx, frame, wire_bytes, target_mbps, dur, tag=None):
    rx.reset(tag)
    sent = 0
    t0 = time.perf_counter()
    deadline = t0 + dur
    if target_mbps:
        pps = target_mbps * 1e6 / (wire_bytes * 8)
        per_tick = max(1, int(round(pps * TICK)))
        while True:
            tick = time.perf_counter()
            if tick >= deadline:
                break
            for _ in range(per_tick):
                try:
                    sock.send(frame)
                except OSError:
                    break
                sent += 1
            rem = TICK - (time.perf_counter() - tick)
            if rem > 0:
                time.sleep(rem)
    else:
        while time.perf_counter() < deadline:
            for _ in range(128):
                try:
                    sock.send(frame)
                except OSError:
                    break
                sent += 1
            time.sleep(0.0002)  # yield so the receive thread drains
    tx_dur = time.perf_counter() - t0
    time.sleep(0.3)  # let the last echoes arrive
    rc, rb, corrupt = rx.snapshot()
    offered = sent * wire_bytes * 8 / tx_dur / 1e6
    echoed = rb * 8 / tx_dur / 1e6
    loss = 100.0 * (sent - rc) / sent if sent else 0.0
    return offered, echoed, loss, corrupt


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("iface", nargs="?", default="enp2s0")
    ap.add_argument("--size", type=int, default=1024, help="payload bytes/frame")
    ap.add_argument("--dur", type=float, default=2.0, help="seconds per level")
    args = ap.parse_args()

    try:
        sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(ETH_TYPE))
        sock.bind((args.iface, 0))
    except PermissionError:
        sys.exit("need root for a raw socket: sudo python3 bench.py " + args.iface)
    except OSError as e:
        sys.exit(f"cannot open {args.iface}: {e}")
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 16 * 1024 * 1024)
    _filter = attach_src_mac_filter(sock, BOARD_MAC)  # kept alive intentionally

    src_mac = iface_mac(sock, args.iface)
    frame = make_frame(src_mac, args.size)
    wire_bytes = args.size + HDR
    rx = Receiver(sock)
    rx.start()

    print(f"[bench] echo bandwidth over {args.iface}, {args.size}-byte payload "
          f"({wire_bytes}-byte L2 frames), {args.dur}s/level")
    print(f"{'target':>8} {'offered':>10} {'echoed':>10} {'loss':>7} {'bad':>6}")
    print(f"{'Mbit/s':>8} {'Mbit/s':>10} {'Mbit/s':>10} {'%':>7} {'frames':>6}")
    print("-" * 47)
    bad_total = 0
    try:
        for i, lvl in enumerate(LEVELS_MBPS):
            # A different fill byte per level. Constant-fill frames cannot show
            # a stale read: a buffer left holding the previous frame's bytes is
            # indistinguishable from one read correctly. Changing the value each
            # level makes the board's reads checkable.
            tag = 0x11 + ((i * 0x22) & 0xDD)
            frame = make_frame(src_mac, args.size, tag)
            offered, echoed, loss, bad = run_level(
                sock, rx, frame, wire_bytes, lvl, args.dur, tag)
            bad_total += bad
            label = "max" if lvl == 0 else str(lvl)
            print(f"{label:>8} {offered:>10.1f} {echoed:>10.1f} {loss:>7.1f} {bad:>6}")
        print()
        if bad_total:
            print(f"!! {bad_total} echoed frame(s) carried the wrong payload -- "
                  "the board read bytes the DMA had not made visible to it. "
                  "That is a cache-maintenance failure, not congestion.")
        else:
            print("payload integrity: every echoed frame matched what was sent.")
            print("Loss at the higher levels is the echo loop saturating; it is "
                  "the 'bad frames' column that would show a coherency fault.")
    except KeyboardInterrupt:
        pass
    finally:
        rx.stop()


if __name__ == "__main__":
    main()
