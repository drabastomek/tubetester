"""
Serial link to ATmega running VTTester v0.4.1 (9600 8N1 default).

Run from repo with PYTHONPATH including ./src, e.g.:

  cd /path/to/tubetester && PYTHONPATH=src python -m backend.communication
"""

from __future__ import annotations

import time
from typing import Iterator

import serial

from backend.communication.protocol import (
    FRAME_BYTES,
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

    def write_frame(self, frame9: bytes) -> None:
        if len(frame9) != FRAME_BYTES:
            raise ValueError(f"frame must be {FRAME_BYTES} bytes")
        self.write_raw(frame9)

    def read_frame(self, deadline: float | None = None) -> Frame | None:
        """
        Read exactly one validated 9-byte frame.
        If `deadline` is set (monotonic time), return None when time expires with no full frame.
        """
        buf = bytearray()
        while len(buf) < FRAME_BYTES:
            if deadline is not None and time.monotonic() > deadline:
                return None
            chunk = self._ser.read(FRAME_BYTES - len(buf))
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
        ua_12: int = 200,
        ug2_12: int = 150,
        p5_ug1_mag: int = 10,
        p6_time_idx: int = 10,
        total_timeout_s: float = 2.0,
    ) -> list[Frame]:
        """
        Send SET with draft-safe parameters, collect responses until idle or timeout.
        Firmware returns ACK + 3 DATA frames (4 total) for a valid SET.

        idx: default v0.4.1 double triode, system 1, parallel heater (pack_index(1,1,0)).
        heater_p1: 63 = 6.3 V in 0.1 V steps (parallel) per §6.
        ua_12 / ug2_12: 12-bit setpoint encoding (§6).
        """
        self.drain()
        if idx is None:
            idx = pack_index(1, 1, 0)
        req = build_set_frame(
            idx,
            heater_p1,
            ua_12,
            ug2_12,
            p5_ug1_mag,
            p6_time_idx,
        )
        self.write_frame(req)
        end = time.monotonic() + total_timeout_s
        out: list[Frame] = []
        for _ in range(4):
            f = self.read_frame(deadline=end)
            if f is None:
                break
            out.append(f)
        return out

    def status(self) -> Frame | None:
        self.drain()
        frame = build_status_frame(0)
        self.write_frame(frame)
        end = time.monotonic() + 0.5
        return self.read_frame(deadline=end)

    def iter_bytes(self) -> Iterator[int]:
        """Low-level: raw RX bytes (for debugging sync)."""
        while True:
            b = self._ser.read(1)
            if b:
                yield b[0]
