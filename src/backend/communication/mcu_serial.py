"""
Serial link to VTTester firmware (v0.5 wire format, 9600 8N1 default).

Run from repo with PYTHONPATH including ./src, e.g.:

  cd /path/to/tubetester && PYTHONPATH=src python -m backend.communication
"""

from __future__ import annotations

import serial

from backend.communication.protocol import (
    FrameSize,
    ResponseCode,
    CommandCode,
    ErrorCode,
    ResetKind,
    prepare_request,
    parse_response,
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

    def send_message(self, frame: bytes) -> None:
        if len(frame) != FrameSize.FRAME_RX_BYTES:
            raise ValueError(f"request must be {FrameSize.FRAME_RX_BYTES} bytes")
        self._ser.write(frame)
        self._ser.flush()

    def drain(self) -> None:
        self._ser.reset_input_buffer()

    def _read_exact(self, n: int) -> bytes | None:
        buf = bytearray()
        while len(buf) < n:
            chunk = self._ser.read(n - len(buf))
            if not chunk:
                return None
            buf.extend(chunk)
        return bytes(buf)

    def _read_and_parse(self, prefix: bytes, frame_size: int):
        """Read ``frame_size - len(prefix)`` bytes and parse the full frame."""
        rest = self._read_exact(frame_size - len(prefix))
        if rest is None:
            return None
        try:
            return parse_response(prefix + rest)
        except ValueError:
            return None

    def get_response(self, first_byte: bytes, frame_size: int):
        return self._read_and_parse(first_byte, frame_size)

    def read_response(self, timeout_s: float | None = None):
        """
        Read one MCU frame. Length follows the tag (firmware communication.c):
        RSP_ACK / RSP_USER_BREAK → 2 B; RSP_ALARM / RSP_ERROR → 3 B or 5 B
        (ERR_OUT_OF_RANGE); RSP_DATA → 20 B (v0.5 §5).
        Returns None on timeout or malformed frame.
        """
        old_timeout = self._ser.timeout
        if timeout_s is not None:
            self._ser.timeout = timeout_s
        try:
            first_byte = self._read_exact(1)
            if not first_byte:
                return None
            tag = first_byte[0]

            match tag:
                case ResponseCode.RSP_ACK | ResponseCode.RSP_USER_BREAK:
                    return self.get_response(first_byte, FrameSize.FRAME_TX_ACK)
                case ResponseCode.RSP_ALARM:
                    return self.get_response(first_byte, FrameSize.FRAME_TX_ERROR)
                case ResponseCode.RSP_ERROR:
                    second = self._read_exact(1)
                    if not second:
                        return None
                    prefix = first_byte + second
                    if second[0] == ErrorCode.ERR_OUT_OF_RANGE:
                        return self._read_and_parse(
                            prefix, FrameSize.FRAME_TX_OOR_ERROR
                        )
                    return self._read_and_parse(prefix, FrameSize.FRAME_TX_ERROR)
                case ResponseCode.RSP_DATA:
                    return self.get_response(first_byte, FrameSize.FRAME_TX_DATA)
                case _:
                    return None
        finally:
            if timeout_s is not None:
                self._ser.timeout = old_timeout

    def send_set(
        self,
        a1_a2: int,
        ug: int,
        uh: int,
        ih: int,
        ua: int,
        ue: int,
        timeout_s: float = 0.5,
    ):
        frame = prepare_request(CommandCode.CMD_SET, a1_a2, ug, uh, ih, ua, ue)
        self.send_message(frame)
        r = self.read_response()
        return r

    def send_reset(self, reset_kind: int):
        frame = prepare_request(CommandCode.CMD_RESET, reset_kind)
        self.send_message(frame)
        r = self.read_response()
        return r

    def get_status(self):
        frame = prepare_request(CommandCode.CMD_STATUS)
        self.send_message(frame)
        r = self.read_response()
        return r