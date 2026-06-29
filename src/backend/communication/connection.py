"""Serial connection lifecycle for VTTester (port open, handshake, disconnect)."""

from __future__ import annotations

import threading
from collections.abc import Callable
from dataclasses import dataclass
from enum import StrEnum
from typing import TYPE_CHECKING, Any

from backend.communication.mcu_serial import DEFAULT_BAUD, VTTesterSerial
from backend.communication.ports import (
    SerialPortInfo,
    find_port_by_dedupe_key,
    find_port_by_device,
    list_serial_ports,
)
from backend.communication.protocol import CommandCode, Data, prepare_request
from backend.communication.serial_prefs import (
    SerialPreferences,
    load_serial_preferences,
    save_serial_preferences,
)

if TYPE_CHECKING:
    import serial


class ConnectionState(StrEnum):
    DISCONNECTED = "disconnected"
    CONNECTING = "connecting"
    CONNECTED = "connected"
    DISCONNECTING = "disconnecting"


class SerialConnectionError(Exception):
    """Base error for the connection service."""


class SerialNotConnectedError(SerialConnectionError):
    """Raised when a serial operation requires an active link."""


Listener = Callable[["ConnectionStatus"], None]
SerialFactory = Callable[..., VTTesterSerial]


@dataclass(frozen=True)
class ConnectionStatus:
    state: ConnectionState
    device: str | None = None
    message: str | None = None


def verify_tester_handshake(
    link: VTTesterSerial,
    *,
    timeout_s: float = 0.5,
) -> bool:
    """Return True when STATUS yields a valid DATA frame."""
    link.drain()
    link.send_message(prepare_request(CommandCode.CMD_STATUS))
    response = link.read_response(timeout_s=timeout_s)
    return isinstance(response, Data)


class SerialConnectionService:
    """
    Owns VTTester serial connect/disconnect and exposes thread-safe status.

  UI / measurement code should call ``connect`` / ``disconnect`` and use
  ``with service.session()`` (or ``require_link()``) for protocol traffic.
    """

    def __init__(
        self,
        *,
        baudrate: int = DEFAULT_BAUD,
        handshake_timeout_s: float = 0.5,
        serial_factory: SerialFactory | None = None,
        prefs_path: str | None = None,
        on_status_changed: Listener | None = None,
    ) -> None:
        self._baudrate = baudrate
        self._handshake_timeout_s = handshake_timeout_s
        self._serial_factory = serial_factory or self._default_serial_factory
        self._prefs_path = prefs_path
        self._on_status_changed = on_status_changed
        self._lock = threading.RLock()
        self._link: VTTesterSerial | None = None
        self._status = ConnectionStatus(ConnectionState.DISCONNECTED)
        self._listeners: list[Listener] = []
        self._ports: list[SerialPortInfo] = []

    @staticmethod
    def _default_serial_factory(port: str, **kwargs: Any) -> VTTesterSerial:
        return VTTesterSerial(port, **kwargs)

    def add_listener(self, listener: Listener) -> None:
        self._listeners.append(listener)

    def remove_listener(self, listener: Listener) -> None:
        self._listeners.remove(listener)

    @property
    def status(self) -> ConnectionStatus:
        with self._lock:
            return self._status

    @property
    def is_connected(self) -> bool:
        return self.status.state == ConnectionState.CONNECTED

    @property
    def ports(self) -> list[SerialPortInfo]:
        with self._lock:
            return list(self._ports)

    def refresh_ports(self, *, include_all: bool = True) -> list[SerialPortInfo]:
        ports = list_serial_ports(include_all=include_all)
        with self._lock:
            self._ports = ports
        return list(ports)

    def load_preferences(self) -> SerialPreferences:
        return load_serial_preferences(self._prefs_path)

    def save_preferences(self, prefs: SerialPreferences) -> None:
        save_serial_preferences(prefs, self._prefs_path)

    def preferred_port(
        self,
        ports: list[SerialPortInfo] | None = None,
    ) -> SerialPortInfo | None:
        """Re-select last device, falling back to dedupe key after replug."""
        available = ports if ports is not None else self.refresh_ports()
        prefs = self.load_preferences()
        if prefs.last_device:
            found = find_port_by_device(available, prefs.last_device)
            if found is not None:
                return found
        if prefs.last_dedupe_key:
            return find_port_by_dedupe_key(available, prefs.last_dedupe_key)
        return None

    def connect(self, device: str) -> ConnectionStatus:
        with self._lock:
            if self._status.state == ConnectionState.CONNECTED:
                if self._status.device == device:
                    return self._status
                self._disconnect_locked()

            self._set_status_locked(
                ConnectionStatus(ConnectionState.CONNECTING, device=device)
            )

        try:
            link = self._serial_factory(
                device,
                baudrate=self._baudrate,
                timeout=0.05,
            )
            if not verify_tester_handshake(link, timeout_s=self._handshake_timeout_s):
                link.close()
                raise SerialConnectionError(
                    "No VTTester response on STATUS (wrong port or firmware not ready)"
                )
        except Exception as exc:
            message = _format_connect_error(exc)
            with self._lock:
                self._link = None
                self._set_status_locked(
                    ConnectionStatus(
                        ConnectionState.DISCONNECTED,
                        device=device,
                        message=message,
                    )
                )
                return self._status

        with self._lock:
            self._link = link
            self._set_status_locked(
                ConnectionStatus(
                    ConnectionState.CONNECTED,
                    device=device,
                    message="Connected",
                )
            )
            self._remember_device_locked(device)
            return self._status

    def connect_async(
        self,
        device: str,
        *,
        callback: Listener | None = None,
    ) -> threading.Thread:
        def work() -> None:
            status = self.connect(device)
            if callback is not None:
                callback(status)

        thread = threading.Thread(target=work, daemon=True, name="vttester-connect")
        thread.start()
        return thread

    def disconnect(self) -> ConnectionStatus:
        with self._lock:
            self._disconnect_locked()
            return self._status

    def require_link(self) -> VTTesterSerial:
        with self._lock:
            if self._link is None or self._status.state != ConnectionState.CONNECTED:
                raise SerialNotConnectedError("serial port is not connected")
            return self._link

    def session(self) -> _SerialSession:
        return _SerialSession(self)

    def _remember_device_locked(self, device: str) -> None:
        prefs = self.load_preferences()
        port = find_port_by_device(self._ports, device)
        if port is None:
            self._ports = list_serial_ports()
            port = find_port_by_device(self._ports, device)
        prefs.last_device = device
        prefs.last_dedupe_key = port.dedupe_key if port else prefs.last_dedupe_key
        self.save_preferences(prefs)

    def _disconnect_locked(self) -> None:
        if self._status.state == ConnectionState.DISCONNECTED:
            return
        previous_device = self._status.device
        self._set_status_locked(
            ConnectionStatus(ConnectionState.DISCONNECTING, device=previous_device)
        )
        if self._link is not None:
            try:
                self._link.close()
            except Exception:
                pass
            self._link = None
        self._set_status_locked(ConnectionStatus(ConnectionState.DISCONNECTED))

    def _set_status_locked(self, status: ConnectionStatus) -> None:
        self._status = status
        listeners = list(self._listeners)
        if self._on_status_changed is not None:
            listeners.append(self._on_status_changed)
        for listener in listeners:
            listener(status)


class _SerialSession:
    def __init__(self, service: SerialConnectionService) -> None:
        self._service = service

    def __enter__(self) -> VTTesterSerial:
        return self._service.require_link()

    def __exit__(self, *exc: object) -> None:
        return None


def _format_connect_error(exc: Exception) -> str:
    if isinstance(exc, SerialConnectionError):
        return str(exc)
    try:
        import serial
    except ImportError:
        return str(exc)
    if isinstance(exc, serial.SerialException):
        text = str(exc).strip() or "serial port error"
        if "permission" in text.lower() or "access is denied" in text.lower():
            return (
                f"{text} — check cable, close other apps using the port, "
                "or on Linux add your user to the dialout group"
            )
        return text
    return str(exc)
