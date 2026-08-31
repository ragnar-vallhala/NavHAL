#!/usr/bin/env python3
"""Exercise a NavHAL USB CDC-ACM device against a real host.

The committed on-target suite (tests/cap/usb_cdc) covers what the driver does
with no host on the bus: refuse transfers, report a closed port, validate
arguments. The other half — enumeration, the class control requests, the bulk
data path, endpoint recovery — only means anything with a host driving it, and
that host is this script.

Flash samples/cortex-m/34_hal_usb_cdc, plug the board's USB port into this
machine, and run:

    tools/hil/usb_cdc_check.py [--port /dev/ttyACM0] [--bytes 16384]

Exits 0 if every check passes, 1 on a failure, 2 if no device is attached
(nothing to test is not a failure). The sample echoes what it receives, so the
data checks assume that behaviour.

Requires: pyserial, and pyusb for the control-transfer and halt checks
(apt install python3-serial python3-usb).
"""

import argparse
import os
import sys
import termios
import time

VID, PID = 0x0483, 0x5740  # matches USB_VID / USB_PID in src/vendor/stm32/usb/usb_cdc.c

# CDC PSTN 1.2 §6.3 class requests, on the communication interface.
CDC_SET_LINE_CODING = 0x20
CDC_GET_LINE_CODING = 0x21
CDC_SET_CONTROL_LINE_STATE = 0x22
CDC_LINE_DTR_RTS = 0x0003
COMM_INTERFACE = 0

npass = nfail = 0


def ok(what):
    global npass
    npass += 1
    print(f"  OK   {what}")


def bad(what, detail=""):
    global nfail
    nfail += 1
    print(f"  FAIL {what}" + (f" — {detail}" if detail else ""))


def find_port(explicit):
    if explicit:
        return explicit if os.path.exists(explicit) else None
    import glob
    for p in sorted(glob.glob("/dev/ttyACM*")):
        return p
    return None


def open_raw(port, baud):
    """Open the port with local echo off.

    A tty comes up with ECHO set, and open() asserts DTR — so between those two
    moments the host echoes back whatever the device sends, the device echoes
    that, and the pair spin at line rate. Raw mode has to be set before any
    traffic, not after the first read.
    """
    import serial
    ser = serial.Serial(port, baud, timeout=2)
    attrs = termios.tcgetattr(ser.fileno())
    attrs[3] &= ~(termios.ECHO | termios.ECHOE | termios.ECHONL | termios.ICANON)
    attrs[0] &= ~(termios.IXON | termios.IXOFF | termios.ICRNL | termios.INLCR)
    attrs[1] &= ~termios.OPOST
    termios.tcsetattr(ser.fileno(), termios.TCSANOW, attrs)
    ser.reset_input_buffer()
    return ser


def check_enumeration():
    try:
        import usb.core
    except ImportError:
        print("  skip enumeration/halt checks (pyusb not installed)")
        return None
    dev = usb.core.find(idVendor=VID, idProduct=PID)
    if dev is None:
        bad("device enumerated", f"no {VID:04x}:{PID:04x} on the bus")
        return None
    ok(f"device enumerated as {VID:04x}:{PID:04x}")
    cfg = dev.get_active_configuration()
    # A CDC-ACM device is two interfaces: comm (class 0x02) and data (0x0A).
    classes = sorted({i.bInterfaceClass for i in cfg})
    if 0x02 in classes and 0x0A in classes:
        ok("comm + data interfaces present")
    else:
        bad("comm + data interfaces present", f"interface classes {classes}")
    return dev


def check_line_coding(dev):
    """The host's baud change has to reach the device and read back the same.

    Nothing on a virtual COM port depends on the rate, which is exactly why a
    driver can get this wrong and look fine: an application that mirrors the
    setting onto a real UART is the one that finds out.
    """
    import usb.util
    coding = (57600).to_bytes(4, "little") + bytes([0, 0, 8])  # 57600 8N1
    try:
        dev.ctrl_transfer(
            usb.util.build_request_type(usb.util.CTRL_OUT,
                                        usb.util.CTRL_TYPE_CLASS,
                                        usb.util.CTRL_RECIPIENT_INTERFACE),
            CDC_SET_LINE_CODING, 0, COMM_INTERFACE, coding)
        raw = dev.ctrl_transfer(
            usb.util.build_request_type(usb.util.CTRL_IN,
                                        usb.util.CTRL_TYPE_CLASS,
                                        usb.util.CTRL_RECIPIENT_INTERFACE),
            CDC_GET_LINE_CODING, 0, COMM_INTERFACE, 7)
    except Exception as e:  # noqa: BLE001 - report whatever the stack raised
        bad("line coding round-trips", str(e))
        return
    if len(raw) != 7:
        bad("line coding round-trips", f"{len(raw)} bytes, expected 7")
        return
    baud = int.from_bytes(bytes(raw[0:4]), "little")
    if baud == 57600 and raw[6] == 8:
        ok("line coding round-trips (57600 8N1)")
    else:
        bad("line coding round-trips", f"baud={baud} data_bits={raw[6]}")


def check_echo(ser, nbytes):
    """Bulk round-trip, reading while writing.

    The device's RX ring is 512 bytes and it drains only as fast as it can echo,
    so a host that writes the whole payload before reading any of it wedges the
    pair: the device NAKs OUT once the ring is full, and it cannot empty the
    ring because nobody is draining the IN side. Concurrency here is not about
    throughput, it is what keeps the transfer moving at all.
    """
    import threading

    payload = bytes((i * 7 + 13) & 0xFF for i in range(nbytes))
    got = bytearray()

    def reader():
        while len(got) < nbytes:
            chunk = ser.read(min(4096, nbytes - len(got)))
            if not chunk:
                break
            got.extend(chunk)

    ser.reset_input_buffer()
    t = threading.Thread(target=reader, daemon=True)
    start = time.time()
    t.start()
    ser.write(payload)
    t.join(timeout=30)
    elapsed = time.time() - start

    if bytes(got) != payload:
        bad("echo is byte-exact", f"{len(got)}/{nbytes} bytes back")
        return
    rate = nbytes / elapsed / 1024 if elapsed else 0
    ok(f"echo is byte-exact ({nbytes} B, {rate:.0f} KiB/s)")


def check_zero_length_boundary(ser):
    """A write that lands exactly on a packet boundary needs a trailing ZLP,
    or the host's read blocks waiting for a short packet that never comes."""
    ser.reset_input_buffer()
    ser.write(bytes(64))
    got = ser.read(64)
    if len(got) == 64:
        ok("64-byte (packet-boundary) transfer completes")
    else:
        bad("64-byte (packet-boundary) transfer completes", f"{len(got)}/64 back")


def _bulk_endpoints(dev):
    ep_in = ep_out = None
    for intf in dev.get_active_configuration():
        for ep in intf:
            if ep.bmAttributes & 0x03 != 0x02:  # bulk only
                continue
            if ep.bEndpointAddress & 0x80:
                ep_in = ep.bEndpointAddress
            else:
                ep_out = ep.bEndpointAddress
    return ep_in, ep_out


def check_halt_recovery(dev):
    """Stalling the bulk IN endpoint and clearing it must leave the port usable.

    USB 2.0 §9.4.5: CLEAR_FEATURE(ENDPOINT_HALT) also resets the data toggle, so
    a device that clears the stall without resetting its own toggle goes on
    NAKing forever and the port is dead rather than recovered. Driving this over
    raw bulk transfers rather than the tty keeps the endpoint state ours: the
    kernel's cdc_acm would clear the halt behind our back.
    """
    ep_in, ep_out = _bulk_endpoints(dev)
    if ep_in is None or ep_out is None:
        bad("bulk endpoint pair found", f"in={ep_in} out={ep_out}")
        return
    ok("bulk endpoint pair found")
    try:
        dev.ctrl_transfer(0x02, 0x03, 0x00, ep_in, None)   # SET_FEATURE HALT
        dev.clear_halt(ep_in)                              # CLEAR_FEATURE HALT
    except Exception as e:  # noqa: BLE001
        bad("halt then clear-halt", str(e))
        return
    ok("halt then clear-halt accepted")
    # The sample only echoes while DTR is asserted, and closing the tty in phase
    # 1 dropped it. Raise it again or the read below times out on a device that
    # is behaving correctly.
    try:
        dev.ctrl_transfer(0x21, CDC_SET_CONTROL_LINE_STATE,
                          CDC_LINE_DTR_RTS, COMM_INTERFACE, None)
    except Exception as e:  # noqa: BLE001
        bad("assert DTR", str(e))
        return
    time.sleep(0.05)
    try:
        dev.write(ep_out, b"recovered", timeout=2000)
        got = bytes(dev.read(ep_in, 64, timeout=2000))
    except Exception as e:  # noqa: BLE001
        bad("endpoint recovers after halt + clear-halt", str(e))
        return
    if got == b"recovered":
        ok("endpoint recovers after halt + clear-halt")
    else:
        bad("endpoint recovers after halt + clear-halt", f"read {got!r}")


def check_break(ser):
    """tcsendbreak reaches the device only if the ACM functional descriptor
    claims bmCapabilities bit 2; without it the host silently drops the call."""
    try:
        termios.tcsendbreak(ser.fileno(), 0)
    except Exception as e:  # noqa: BLE001
        bad("send break", str(e))
        return
    time.sleep(0.2)
    ser.reset_input_buffer()
    ser.write(b"after-break")
    if ser.read(11) == b"after-break":
        ok("port still works after a break")
    else:
        bad("port still works after a break")


def _detached(dev):
    """Take both CDC interfaces away from the kernel's cdc_acm and give them
    back afterwards. libusb cannot touch an interface the kernel has claimed —
    "Resource busy" — and /dev/ttyACM* has to come back for the next run."""
    import contextlib
    import usb.util

    @contextlib.contextmanager
    def ctx():
        taken = []
        for ifnum in (0, 1):
            try:
                if dev.is_kernel_driver_active(ifnum):
                    dev.detach_kernel_driver(ifnum)
                    taken.append(ifnum)
            except Exception:  # noqa: BLE001 - nothing to give back if it failed
                pass
        try:
            yield
        finally:
            usb.util.dispose_resources(dev)
            for ifnum in taken:
                try:
                    dev.attach_kernel_driver(ifnum)
                except Exception:  # noqa: BLE001
                    pass
    return ctx()


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", default=None, help="default: first /dev/ttyACM*")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--bytes", type=int, default=16384, help="echo payload size")
    args = ap.parse_args()

    port = find_port(args.port)
    if not port:
        print("usb_cdc_check: no /dev/ttyACM* — is the board flashed with "
              "34_hal_usb_cdc and plugged in?")
        return 2

    print(f"==== NavHAL USB CDC-ACM host checks on {port} ====")
    dev = check_enumeration()

    # Phase 1: through the kernel's cdc_acm, the way an application sees it.
    try:
        ser = open_raw(port, args.baud)
    except Exception as e:  # noqa: BLE001
        bad("open port", str(e))
        print(f"\npass={npass} fail={nfail}")
        return 1
    try:
        check_echo(ser, args.bytes)
        check_zero_length_boundary(ser)
        check_break(ser)
    finally:
        ser.close()

    # Phase 2: with cdc_acm out of the way, so the class requests and the
    # endpoint state are ours to drive.
    if dev is not None:
        with _detached(dev):
            check_line_coding(dev)
            check_halt_recovery(dev)

    print(f"\npass={npass} fail={nfail}")
    return 1 if nfail else 0


if __name__ == "__main__":
    sys.exit(main())
