"""
High-level API and event callbacks for VTTester desktop app.
- set_params(...), start_measurement(...), read_status(...) placeholder, reset_params(...) placeholder
- Events: on_measurement_result(point), on_status_update(status), on_alarm(alarm_info)
"""

from typing import Callable, Optional, Any

from .protocol import (
    Protocol,
    ERR_OK,
    ERR_CRC,
    ERR_OUT_OF_RANGE,
    ALARM_OVERIH,
    ALARM_OVERIA,
    ALARM_OVERIG,
    ALARM_OVERTE,
)
from .serial_driver import SerialDriver


# Type aliases for callbacks
OnMeasurementResult = Callable[[dict], None]
OnStatusUpdate = Callable[[dict], None]
OnAlarm = Callable[[dict], None]


class VTTesterClient:
    """
    High-level client: open port, handshake, then call set_params / start_measurement.
    One command at a time; each call waits for response (with timeout) before returning.
    Placeholders: read_status(), reset_params() — not yet in protocol.
    Events: assign on_measurement_result, on_status_update, on_alarm (called when data arrives).
    """

    def __init__(
        self,
        port: Optional[str] = None,
        baudrate: int = 9600,
        timeout_s: float = 2.0,
        protocol: Optional[Protocol] = None,
    ):
        self._protocol = protocol or Protocol()
        self._driver = SerialDriver(baudrate=baudrate, protocol=self._protocol)
        self._port = port
        self._timeout_s = timeout_s
        self._index = 0

        # Callbacks (assign from outside)
        self.on_measurement_result: Optional[OnMeasurementResult] = None
        self.on_status_update: Optional[OnStatusUpdate] = None
        self.on_alarm: Optional[OnAlarm] = None

    def open(self, port: Optional[str] = None) -> None:
        """Open COM port. Use port argument or the one passed to __init__."""
        p = port or self._port
        if not p:
            raise ValueError("Port not specified")
        self._driver.open(p)
        self._port = p

    def close(self) -> None:
        self._driver.close()

    @property
    def is_open(self) -> bool:
        return self._driver.is_open

    def handshake(self) -> bool:
        """Check that the device on the other side is VTTester (simple MEAS ping)."""
        return self._driver.handshake(timeout_s=self._timeout_s)

    def _request(self, frame: bytes) -> dict:
        """Send one frame, receive one response; return parsed dict. Raises on timeout or CRC error."""
        self._driver.send_frame(frame)
        raw = self._driver.recv_frame(timeout_s=self._timeout_s)
        if raw is None:
            return {"type": "error", "err_code": ERR_CRC, "timeout": True}
        return self._protocol.parse_response(raw)

    def set_params(
        self,
        heat_idx: int = 0,
        ua_v: int = 0,
        ug2_v: int = 0,
        ug1_def: int = 240,
        tuh_ticks: int = 0,
    ) -> dict:
        """
        Send SET command. heat_idx 0..7, ua_v/ug2_v in V (max 300),
        ug1_def 0..240 (0.1V magnitude), tuh_ticks = tuh_index*TUH_TICK_SCALE (tuh_index 0..SET_TUH_INDEX_MAX).
        Returns parsed ACK dict (err_code, error_param, error_value if OUT_OF_RANGE).
        """
        self._index = (self._index + 1) & 0xFF
        frame = self._protocol.build_set_frame(
            index=self._index,
            heat_idx=heat_idx,
            ua_v=ua_v,
            ug2_v=ug2_v,
            ug1_def=ug1_def,
            tuh_ticks=tuh_ticks,
        )
        out = self._request(frame)
        if out.get("type") == "ack" and self.on_status_update:
            self.on_status_update(out)
        return out

    def start_measurement(self) -> dict:
        """
        Send MEAS command; wait for measurement result (one 8-byte frame).
        Returns parsed dict (type='measurement' with ihlcd, ialcd, ig2lcd, slcd, alarm_bits).
        If on_measurement_result is set, calls it with the result dict.
        If response indicates alarm, calls on_alarm if set.
        """
        self._index = (self._index + 1) & 0xFF
        frame = self._protocol.build_meas_frame(self._index)
        out = self._request(frame)
        if out.get("type") == "measurement":
            if self.on_measurement_result:
                self.on_measurement_result(out)
            if out.get("alarm_bits", 0) and self.on_alarm:
                self.on_alarm(_alarm_info(out))
        return out

    def read_status(self) -> dict:
        """
        Placeholder: read status — not yet part of the protocol.
        Returns a stub dict so callers can be written against the API.
        """
        return {"type": "status", "stub": True, "message": "Not in protocol yet"}

    def reset_params(self) -> dict:
        """
        Placeholder: reset params — not yet part of the protocol.
        Returns a stub dict so callers can be written against the API.
        """
        return {"type": "reset_params", "stub": True, "message": "Not in protocol yet"}


def _alarm_info(meas: dict) -> dict:
    """Build alarm_info dict from measurement alarm_bits."""
    bits = meas.get("alarm_bits", 0)
    return {
        "alarm_bits": bits,
        "over_ih": bool(bits & ALARM_OVERIH),
        "over_ia": bool(bits & ALARM_OVERIA),
        "over_ig": bool(bits & ALARM_OVERIG),
        "over_te": bool(bits & ALARM_OVERTE),
        "measurement": meas,
    }
