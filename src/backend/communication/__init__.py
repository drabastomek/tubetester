"""VTTester desktop ↔ MCU serial protocol."""

from backend.communication.connection import (
    ConnectionState,
    ConnectionStatus,
    SerialConnectionError,
    SerialConnectionService,
    SerialNotConnectedError,
    verify_tester_handshake,
)
from backend.communication.ports import SerialPortInfo, format_port_label, list_serial_ports
from backend.communication.protocol import crc8_run, prepare_request
from backend.communication.serial_prefs import (
    SerialPreferences,
    load_serial_preferences,
    save_serial_preferences,
)

__all__ = [
    "ConnectionState",
    "ConnectionStatus",
    "SerialConnectionError",
    "SerialConnectionService",
    "SerialNotConnectedError",
    "SerialPortInfo",
    "SerialPreferences",
    "crc8_run",
    "format_port_label",
    "list_serial_ports",
    "load_serial_preferences",
    "prepare_request",
    "save_serial_preferences",
    "verify_tester_handshake",
]
