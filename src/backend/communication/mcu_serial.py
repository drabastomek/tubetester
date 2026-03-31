"""
Serial link to ATmega running VTTester v0.4 (9600 8N1 default).

Run from repo with PYTHONPATH including ./src, e.g.:

  cd /path/to/tubetester && PYTHONPATH=src python -m backend.communication
"""

from __future__ import annotations

import time
from typing import Iterator

import serial

from backend.communication.protocol import (
    Frame,
    build_set_frame,
    build_status_frame,
    pack_index,
)

DEFAULT_BAUD = 9600


class VTTesterSerial:
    def __init__(
        self,
        port: str,
        *,
        baudrate: int = DEFAULT_BAUD,
        timeout: float = 0.05,
    ) -> None:
        self._ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=timeout,
        )

    def close(self) -> None:
        self._ser.close()

    def __enter__(self) -> VTTesterSerial:
        return self

    def __exit__(self, *exc: object) -> None:
        self.close()

    def write_raw(self, data: bytes) -> None:
        self._ser.write(data)
        self._ser.flush()

    def write_frame(self, frame8: bytes) -> None:
        if len(frame8) != 8:
            raise ValueError("frame must be 8 bytes")
        self.write_raw(frame8)

    def read_frame(self, deadline: float | None = None) -> Frame | None:
        """
        Read exactly one validated 8-byte frame.
        If `deadline` is set (monotonic time), return None when time expires with no full frame.
        """
        buf = bytearray()
        while len(buf) < 8:
            if deadline is not None and time.monotonic() > deadline:
                return None
            chunk = self._ser.read(8 - len(buf))
            if not chunk:
                continue
            buf.extend(chunk)
        try:
            return Frame.from_bytes_wire(bytes(buf))
        except ValueError:
            return None

    def drain(self) -> None:
        self._ser.reset_input_buffer()

    def frames_after_set(
        self,
        *,
        idx: int | None = None,
        heater_p1: int = 63,
        ua_steps: int = 20,
        ug2_steps: int = 15,
        ug1_steps: int = 10,
        time_idx: int = 10,
        total_timeout_s: float = 2.0,
    ) -> list[Frame]:
        """
        Send SET with draft-safe parameters, collect responses until idle or timeout.
        Firmware returns ACK + 4 DATA frames (5 total) for a valid SET.

        idx: default v0.4 double triode, system 1, parallel heater (pack_index(1,1,0)).
        heater_p1: 63 = 6.3 V in 0.1 V steps (parallel) per v0.4 §6.
        """
        self.drain()
        if idx is None:
            idx = pack_index(1, 1, 0)
        req = build_set_frame(idx, heater_p1, ua_steps, ug2_steps, ug1_steps, time_idx)
        self.write_frame(req)
        end = time.monotonic() + total_timeout_s
        out: list[Frame] = []
        # Draft firmware: ACK + 4× DATA for a valid SET.
        for _ in range(5):
            f = self.read_frame(deadline=end)
            if f is None:
                break
            out.append(f)
        return out

    def status(self) -> Frame | None:
        self.drain()
        frame = build_status_frame(0)
        print(f"building status frame: {Frame.from_bytes_wire(frame)}")
        self.write_frame(frame)
        end = time.monotonic() + 0.5
        return self.read_frame(deadline=end)

    def iter_bytes(self) -> Iterator[int]:
        """Low-level: raw RX bytes (for debugging sync)."""
        while True:
            b = self._ser.read(1)
            if b:
                yield b[0]
