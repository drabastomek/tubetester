"""
Example: talk to the comms-test firmware.

  PYTHONPATH=src python -m backend.communication /dev/cu.SLAB_USBtoUART
"""

from __future__ import annotations

import argparse
import sys

import serial

from backend.communication.mcu_serial import VTTesterSerial
from backend.communication.protocol import RSP_ERROR, RSP_ACK


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description="VTTester v0.4 serial skeleton")
    p.add_argument("port", help="Serial device (e.g. /dev/cu.SLAB_USBtoUART)")
    p.add_argument("-b", "--baud", type=int, default=9600)
    args = p.parse_args(argv)

    try:
        with VTTesterSerial(args.port, baudrate=args.baud) as link:
            print("STATUS …")
            st = link.status()
            print(f'STATUS ECHO: {st}')
            print("SET (draft params) …")
            frames = link.frames_after_set()
            for i, fr in enumerate(frames):
                kind = "?"
                if fr.ctrl & 0x20:
                    kind = "DATA"
                elif (fr.ctrl & 0xC0) == (RSP_ERROR & 0xC0):
                    kind = "ERROR"
                elif (fr.ctrl & 0xC0) == (RSP_ACK & 0xC0) and fr.ctrl & 0x20 == 0:
                    kind = "ACK/STAT"
                print(f"  [{i}] {kind} {fr}")
    except serial.SerialException as e:
        print(e, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
