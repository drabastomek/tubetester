"""
Serial driver: open/close COM port, handshake with VTTester, send one frame / receive one frame with timeout.
Uses pyserial. Command queue: one command at a time, wait for response (with timeout) before next.
"""

import serial
from typing import Optional

from .protocol import Protocol


class SerialDriver:
    """
    Low-level serial driver for VTTester 8-byte protocol.
    - open(port) / close()
    - handshake() -> bool: verify device responds (e.g. send MEAS, expect valid 8-byte reply)
    - send_frame(data: bytes) / recv_frame(timeout_s: float) -> bytes | None
    """

    def __init__(self, baudrate: int = 9600, protocol: Optional[Protocol] = None):
        self._baudrate = baudrate
        self._protocol = protocol or Protocol()
        self._ser: Optional[serial.Serial] = None

    def open(self, port: str) -> None:
        """Open COM port. Raises if already open or port unavailable."""
        if self._ser is not None and self._ser.is_open:
            raise RuntimeError("Port already open")
        self._ser = serial.Serial(
            port=port,
            baudrate=self._baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.1,
        )

    def close(self) -> None:
        """Close COM port."""
        if self._ser is not None:
            try:
                self._ser.close()
            finally:
                self._ser = None

    @property
    def is_open(self) -> bool:
        return self._ser is not None and self._ser.is_open

    def send_frame(self, data: bytes) -> None:
        """Send exactly one 8-byte frame."""
        if not self.is_open:
            raise RuntimeError("Port not open")
        if len(data) != self._protocol.FRAME_LEN:
            raise ValueError("Frame must be 8 bytes")
        self._ser.write(data)
        self._ser.flush()

    def recv_frame(self, timeout_s: float = 2.0) -> Optional[bytes]:
        """
        Read one 8-byte frame. Discards bytes until a valid start is found (optional).
        Returns 8 bytes if a complete frame was read and CRC is valid, else None.
        """
        if not self.is_open:
            raise RuntimeError("Port not open")
        self._ser.timeout = 0.05
        import time
        deadline = time.monotonic() + timeout_s

        buf = bytearray()
        while True:
            if time.monotonic() > deadline:
                return None
            chunk = self._ser.read(1)
            if not chunk:
                continue
            buf.append(chunk[0])
            if len(buf) > self._protocol.FRAME_LEN:
                buf = buf[-self._protocol.FRAME_LEN:]
            if len(buf) == self._protocol.FRAME_LEN:
                frame = bytes(buf)
                if self._protocol.verify_crc(frame):
                    return frame
                buf = buf[1:]

    def handshake(self, timeout_s: float = 1.0) -> bool:
        """
        Simple handshake: send MEAS request (index 0), expect valid 8-byte response with correct CRC.
        Returns True if we received a valid frame (device is likely VTTester).
        """
        if not self.is_open:
            return False
        self._ser.reset_input_buffer()
        self.send_frame(self._protocol.build_meas_frame(0))
        reply = self.recv_frame(timeout_s=timeout_s)
        return reply is not None and self._protocol.verify_crc(reply)
