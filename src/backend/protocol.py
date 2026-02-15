"""
VTTester protocol v0.2: 8-byte binary frames, CRC-8 on bytes 0..6.
Matches firmware protocol (vttester_remote.c).
"""

from typing import Dict, Any

# -----------------------------------------------------------------------------
# Constants (module-level and on Protocol for access either way)
# -----------------------------------------------------------------------------

FRAME_LEN = 8

# CTRL command bits (PC -> FW): bits 7-6 = CMD, bit 5 = 1 (valid)
CMD_SET = 0
CMD_MEAS = 1
CTRL_SET = 0x20   # SET, bit 5 set
CTRL_MEAS = 0x60  # MEAS, bit 5 set
# TODO: add CTRL_STATUS and CTRL_RESET commands

# Response status (FW -> PC): buf[0] & 0xC0
STATUS_OK = 0x00
STATUS_BUSY = 0x40
STATUS_ERROR = 0x80
STATUS_ALARM = 0xC0

# Error codes in ACK R1
ERR_OK = 0x00
ERR_UNKNOWN = 0x01
ERR_OUT_OF_RANGE = 0x02
ERR_CRC = 0x03
ERR_INVALID_CMD = 0x04
ERR_HARDWARE = 0x06

# Alarm bitmap in MEAS result R5
ALARM_OVERIH = 0x10
ALARM_OVERIA = 0x20
ALARM_OVERIG = 0x40
ALARM_OVERTE = 0x80

# SET limits (must match firmware config)
SET_HEAT_IDX_MAX = 7
SET_UA_MAX = 300
SET_UG2_MAX = 300
SET_UG1_MAX = 240 # 0.1V steps, -24V 
SET_TUH_INDEX_MAX = 63  # heating time index 0..63 (500 ms per step) -> tuh_ticks = index * TUH_TICK_SCALE
UA_SCALE = 10
UG1_P4_STEP = 5
TUH_TICK_SCALE = 2

# CRC-8 table from firmware (vttester_remote.c) - same polynomial/algorithm
_CRC8_TABLE = bytes([
    0x00,0x07,0x0E,0x09,0x1C,0x1B,0x12,0x15,0x38,0x3F,0x36,0x31,0x24,0x23,0x2A,0x2D,
    0x70,0x77,0x7E,0x79,0x6C,0x6B,0x62,0x65,0x48,0x4F,0x46,0x41,0x54,0x53,0x5A,0x5D,
    0xE0,0xE7,0xEE,0xE9,0xFC,0xFB,0xF2,0xF5,0xD8,0xDF,0xD6,0xD1,0xC4,0xC3,0xCA,0xCD,
    0x90,0x97,0x9E,0x99,0x8C,0x8B,0x82,0x85,0xA8,0xAF,0xA6,0xA1,0xB4,0xB3,0xBA,0xBD,
    0xC7,0xC0,0xC9,0xCE,0xDB,0xDC,0xD5,0xD2,0xFF,0xF8,0xF1,0xF6,0xE3,0xE4,0xED,0xEA,
    0xB7,0xB0,0xB9,0xBE,0xAB,0xAC,0xA5,0xA2,0x8F,0x88,0x81,0x86,0x93,0x94,0x9D,0x9A,
    0x27,0x20,0x29,0x2E,0x3B,0x3C,0x35,0x32,0x1F,0x18,0x11,0x16,0x03,0x04,0x0D,0x0A,
    0x57,0x50,0x59,0x5E,0x4B,0x4C,0x45,0x42,0x6F,0x68,0x61,0x66,0x73,0x74,0x7D,0x7A,
    0x89,0x8E,0x87,0x80,0x95,0x92,0x9B,0x9C,0xB1,0xB6,0xBF,0xB8,0xAD,0xAA,0xA3,0xA4,
    0xF9,0xFE,0xF7,0xF0,0xE5,0xE2,0xEB,0xEC,0xC1,0xC6,0xCF,0xC8,0xDD,0xDA,0xD3,0xD4,
    0x69,0x6E,0x67,0x60,0x75,0x72,0x7B,0x7C,0x51,0x56,0x5F,0x58,0x4D,0x4A,0x43,0x44,
    0x19,0x1E,0x17,0x10,0x05,0x02,0x0B,0x0C,0x21,0x26,0x2F,0x28,0x3D,0x3A,0x33,0x34,
    0x4E,0x49,0x40,0x47,0x52,0x55,0x5C,0x5B,0x76,0x71,0x78,0x7F,0x6A,0x6D,0x64,0x63,
    0x3E,0x39,0x30,0x37,0x22,0x25,0x2C,0x2B,0x06,0x01,0x08,0x0F,0x1A,0x1D,0x14,0x13,
    0xAE,0xA9,0xA0,0xA7,0xB2,0xB5,0xBC,0xBB,0x96,0x91,0x98,0x9F,0x8A,0x8D,0x84,0x83,
    0xDE,0xD9,0xD0,0xD7,0xC2,0xC5,0xCC,0xCB,0xE6,0xE1,0xE8,0xEF,0xFA,0xFD,0xF4,0xF3,
])


class Protocol:
    """
    VTTester 8-byte binary protocol: encode/decode frames, CRC-8.
    Stateless; safe to reuse one instance across the app.
    """

    # Expose constants on the class for convenience
    FRAME_LEN = FRAME_LEN
    CMD_SET = CMD_SET
    CMD_MEAS = CMD_MEAS
    ERR_OK = ERR_OK
    ERR_UNKNOWN = ERR_UNKNOWN
    ERR_OUT_OF_RANGE = ERR_OUT_OF_RANGE
    ERR_CRC = ERR_CRC
    ERR_INVALID_CMD = ERR_INVALID_CMD
    ERR_HARDWARE = ERR_HARDWARE
    ALARM_OVERIH = ALARM_OVERIH
    ALARM_OVERIA = ALARM_OVERIA
    ALARM_OVERIG = ALARM_OVERIG
    ALARM_OVERTE = ALARM_OVERTE

    def crc8(self, data: bytes) -> int:
        """CRC-8 over data (bytes 0..len-1). Returns single byte as int."""
        crc = 0
        for b in data:
            crc = _CRC8_TABLE[crc ^ b]
        return crc

    def verify_crc(self, frame: bytes) -> bool:
        """Return True if frame has valid CRC-8 on bytes 0..6."""
        if len(frame) < self.FRAME_LEN:
            return False
        return self.crc8(frame[:7]) == frame[7]

    def build_set_frame(
        self,
        index: int,
        heat_idx: int,
        ua_v: int,
        ug2_v: int,
        ug1_def: int,
        tuh_ticks: int,
    ) -> bytes:
        """
        Build 8-byte SET request. heat_idx 0..7, ua_v/ug2_v in V (max 300),
        ug1_def 0..240 (0.1V magnitude), tuh_ticks = tuh_index*TUH_TICK_SCALE (tuh_index 0..SET_TUH_INDEX_MAX).
        """
        p2 = min(ua_v // UA_SCALE, 255)
        p3 = min(ug2_v // UA_SCALE, 255)
        p4 = min(ug1_def // UG1_P4_STEP, 255)
        tuh_index = min(tuh_ticks // TUH_TICK_SCALE, SET_TUH_INDEX_MAX)
        frame = bytearray([
            CTRL_SET,
            index & 0xFF,
            heat_idx & 0x0F,
            p2,
            p3,
            p4,
            tuh_index,
            0,
        ])
        frame[7] = self.crc8(bytes(frame[:7]))
        return bytes(frame)

    def build_meas_frame(self, index: int) -> bytes:
        """Build 8-byte MEAS request."""
        frame = bytearray([CTRL_MEAS, index & 0xFF, 0, 0, 0, 0, 0, 0])
        frame[7] = self.crc8(bytes(frame[:7]))
        return bytes(frame)

    def parse_response(self, frame: bytes) -> Dict[str, Any]:
        """
        Parse 8-byte response from device. Returns a dict:
        - 'type': 'ack' | 'measurement' | 'error'
        - 'index': int
        - For ack: 'err_code', 'error_param', 'error_value'
        - For measurement: 'ihlcd', 'ialcd', 'rangelcd', 'ig2lcd', 'slcd', 'alarm_bits'
        - For error: 'err_code'
        """
        if len(frame) < self.FRAME_LEN or not self.verify_crc(frame):
            return {"type": "error", "err_code": ERR_CRC}

        ctrl = frame[0]
        status = ctrl & 0xC0
        index = frame[1]

        if status == STATUS_ERROR or status == STATUS_ALARM:
            return {
                "type": "error" if status == STATUS_ERROR else "alarm",
                "index": index,
                "err_code": frame[2],
                "error_param": frame[3] if frame[2] == ERR_OUT_OF_RANGE else 0,
                "error_value": (frame[4] << 8) | frame[5] if frame[2] == ERR_OUT_OF_RANGE else 0,
            }

        err_code = frame[2]
        is_ack_err = err_code in (0x00, 0x01, 0x02, 0x03, 0x04, 0x06)
        if is_ack_err and (err_code != ERR_OUT_OF_RANGE and frame[3] == 0 and frame[4] == 0 and frame[5] == 0):
            return {
                "type": "ack",
                "index": index,
                "err_code": err_code,
                "error_param": 0,
                "error_value": 0,
            }
        if err_code == ERR_OUT_OF_RANGE:
            return {
                "type": "ack",
                "index": index,
                "err_code": err_code,
                "error_param": frame[3],
                "error_value": (frame[4] << 8) | frame[5],
            }

        ialcd_raw = (frame[3] * 10) if frame[3] < 26 else 2550
        ig2lcd_raw = ((frame[4] * 16 - 8) // 10) if frame[4] < 256 else 0
        slcd_raw = (frame[5] * 10) if frame[5] < 100 else 999
        return {
            "type": "measurement",
            "index": index,
            "ihlcd": frame[2],
            "ialcd": ialcd_raw,
            "rangelcd": 0,
            "ig2lcd": ig2lcd_raw,
            "slcd": slcd_raw,
            "alarm_bits": frame[6] & 0xF0,
        }


# Default instance for module-level API
_default_protocol = Protocol()


# -----------------------------------------------------------------------------
# Module-level API (delegate to default Protocol for backward compatibility)
# -----------------------------------------------------------------------------

def crc8(data: bytes) -> int:
    return _default_protocol.crc8(data)


def build_set_frame(
    index: int,
    heat_idx: int,
    ua_v: int,
    ug2_v: int,
    ug1_def: int,
    tuh_ticks: int,
) -> bytes:
    return _default_protocol.build_set_frame(index, heat_idx, ua_v, ug2_v, ug1_def, tuh_ticks)


def build_meas_frame(index: int) -> bytes:
    return _default_protocol.build_meas_frame(index)


def verify_crc(frame: bytes) -> bool:
    return _default_protocol.verify_crc(frame)


def parse_response(frame: bytes) -> Dict[str, Any]:
    return _default_protocol.parse_response(frame)
