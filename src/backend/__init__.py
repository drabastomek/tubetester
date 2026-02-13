# VTTester desktop backend: serial driver + 8-byte binary protocol (SET/MEAS, CRC-8).

from .protocol import (
    Protocol,
    FRAME_LEN,
    CMD_SET,
    CMD_MEAS,
    ERR_OK,
    ERR_CRC,
    ERR_OUT_OF_RANGE,
    ERR_INVALID_CMD,
    ALARM_OVERIH,
    ALARM_OVERIA,
    ALARM_OVERIG,
    ALARM_OVERTE,
    crc8,
    build_set_frame,
    build_meas_frame,
    parse_response,
)
from .serial_driver import SerialDriver
from .api import VTTesterClient

__all__ = [
    "Protocol",
    "SerialDriver",
    "VTTesterClient",
    "FRAME_LEN",
    "CMD_SET",
    "CMD_MEAS",
    "ERR_OK",
    "ERR_CRC",
    "ERR_OUT_OF_RANGE",
    "ERR_INVALID_CMD",
    "ALARM_OVERIH",
    "ALARM_OVERIA",
    "ALARM_OVERIG",
    "ALARM_OVERTE",
    "crc8",
    "build_set_frame",
    "build_meas_frame",
    "parse_response",
]
