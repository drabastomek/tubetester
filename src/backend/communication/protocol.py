from __future__ import annotations

from dataclasses import dataclass

# Frame size
class FrameSize:
    FRAME_RX_BYTES = 10
    FRAME_TX_DATA = 20
    FRAME_TX_DATA_ERR = 18
    FRAME_TX_ACK = 2
    FRAME_TX_ERROR = 3
    FRAME_TX_OOR_ERROR = 5

class CommandCode:
    CMD_SET = 0x00
    CMD_RESET = 0x01
    CMD_STATUS = 0x02
    CMD_BEEP = 0x03

class BeepDuration:
    """DUR byte: duration in 10 ms units; 0 = firmware default (50 ms)."""
    DEFAULT = 0
    UNIT_MS = 10
    FIRMWARE_DEFAULT_MS = 50

class ResponseCode:
    RSP_DATA = 0x00
    RSP_ERROR = 0x01
    RSP_ACK = 0x02
    RSP_USER_BREAK = 0x03
    RSP_ALARM = 0x04

class ResetKind:
    RESET_HEATER = 0x01
    RESET_FULL = 0x02

class ErrorCode:
    ERR_UNKNOWN = 0x00
    ERR_OUT_OF_RANGE = 0x02
    ERR_CRC = 0x03
    ERR_INVALID_CMD = 0x04
    ERR_HARDWARE = 0x06


class ErrBits:
    """ERR byte in 20-byte DATA response (v0.5 §6)."""
    RNG200 = 0x80
    OVERTM = 0x08
    OVERIE = 0x04
    OVERIA = 0x02
    OVERIH = 0x01


# Desktop measurement timing — see src/backend/MEASUREMENT_FLOW.md
STATUS_POLL_INTERVAL_MS = 125
HEATER_PREHEAT_DEFAULT_S = 60

# Maps reverse
RESPONSE_MAP = {getattr(ResponseCode, name): name 
                for name in dir(ResponseCode) 
                if name.startswith('RSP_')}

PARAM_ID_UH = 0x01
PARAM_ID_IH = 0x02
PARAM_ID_UG = 0x03
PARAM_ID_UA = 0x04
PARAM_ID_IA = 0x05
PARAM_ID_UE = 0x06
PARAM_ID_IE = 0x07
PARAM_ID_TM = 0x08

# Harness-only fault ids (STATUS with A1_A2='H', ih byte).
HARNS_FAULT_ALARM_ON_SET = 1
HARNS_FAULT_SILENT_ONCE = 2
HARNS_FAULT_HWERR_ON_STATUS = 3

# CRC-8 poly 0x07, init 0 — same 256-byte table as firmware
CRC8_TABLE: tuple[int, ...] = (
    0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
    157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
    35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
    190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
    70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
    219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
    101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
    248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
    140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
    17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
    175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
    50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
    202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
    87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
    233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
    116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53,
)


def crc8_run(data: bytes) -> int:
    """CRC-8 over all bytes in ``data`` (init 0)."""
    c = 0
    for x in data:
        c = CRC8_TABLE[c ^ x]
    return c


def prepare_request(
    cmd: int,
    a1_a2: str = "0",
    ug: int = 240,
    uh: int = 0,
    ih: int = 0,
    ua: int = 0,
    ue: int = 0,
    reset_kind: int = ResetKind.RESET_HEATER,
) -> bytes:
    match cmd:
        case CommandCode.CMD_SET:
            params = (
                CommandCode.CMD_SET, 
                ord(a1_a2), 
                ug, 
                uh, 
                ih, 
                ua & 0xFF,
                (ua >> 8) & 0xFF,
                ue,
                (ue >> 8) & 0xFF
            )
        case CommandCode.CMD_RESET:
            params = (CommandCode.CMD_RESET, reset_kind, 0, 0, 0, 0, 0, 0, 0)
        case CommandCode.CMD_STATUS:
            params = (CommandCode.CMD_STATUS, ord(a1_a2), 0, 0, 0, 0, 0, 0, 0)
        case CommandCode.CMD_BEEP:
            raise ValueError("use prepare_beep() for CMD_BEEP")
        case _:
            raise ValueError(f"Invalid command: {cmd}")
    return bytes(params + (crc8_run(bytes(params)),))


def prepare_beep(duration_units: int = BeepDuration.DEFAULT) -> bytes:
    """BEEP request (CMD 0x03). DUR = duration in 10 ms units; 0 → 50 ms default."""
    params = (
        CommandCode.CMD_BEEP,
        duration_units & 0xFF,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    )
    return bytes(params + (crc8_run(bytes(params)),))


def prepare_harness_arm(fault_id: int) -> bytes:
    """Harness-only: arm fault injection on the MCU (STATUS, A1_A2='H')."""
    params = (
        CommandCode.CMD_STATUS,
        ord("H"),
        0,
        0,
        fault_id & 0xFF,
        0,
        0,
        0,
        0,
    )
    return bytes(params + (crc8_run(bytes(params)),))


def prepare_raw_request(payload: tuple[int, ...]) -> bytes:
    """Build a 10-byte request from nine payload bytes (CRC appended)."""
    if len(payload) != 9:
        raise ValueError("payload must be 9 bytes")
    body = bytes(payload)
    return body + bytes([crc8_run(body)])


def corrupt_crc(frame: bytes) -> bytes:
    if len(frame) < 2:
        raise ValueError("frame too short")
    return frame[:-1] + bytes([(frame[-1] + 1) & 0xFF])


def _le16(buf: bytes, off: int) -> int:
    return buf[off] | (buf[off + 1] << 8)

@dataclass(frozen=True)
class OutOfRangeError:
    parameter_id: int
    value: int
    raw: bytes


@dataclass(frozen=True)
class ProtocolError:
    code: int
    raw: bytes


@dataclass(frozen=True)
class Data:
    a1_a2: str
    ug_sum: int
    uh_sum: int
    ih_sum: int
    ua_sum: int
    ia_sum: int
    ue_sum: int
    ie_sum: int
    tm_sum: int
    err: int
    raw: bytes

    @staticmethod
    def from_bytes(buf: bytes) -> Data:
        if len(buf) != FrameSize.FRAME_TX_DATA:
            raise ValueError(f"DATA frame must be {FrameSize.FRAME_TX_DATA} bytes")
        if buf[0] != ResponseCode.RSP_DATA:
            raise ValueError(f"expected RSP_DATA, got 0x{buf[0]:02x}")

        return Data(
            a1_a2=chr(buf[1]),
            ug_sum=_le16(buf, 2),
            uh_sum=_le16(buf, 4),
            ih_sum=_le16(buf, 6),
            ua_sum=_le16(buf, 8),
            ia_sum=_le16(buf, 10),
            ue_sum=_le16(buf, 12),
            ie_sum=_le16(buf, 14),
            tm_sum=_le16(buf, 16),
            err=buf[FrameSize.FRAME_TX_DATA_ERR],
            raw=bytes(buf),
        )
    
    def __str__(self) -> str:
        str_prep = "DATA: \n"

        str_prep += f"    A1/A2: \t{self.a1_a2}\n"
        str_prep += f"    UG: \t{-self.ug_sum / 64:4.2f} \tV\n"
        str_prep += f"    UH: \t{ self.uh_sum / 64 / 10.0:4.2f} \tV\n"
        str_prep += f"    IH: \t{ self.ih_sum / 64 * 10.0:4.2f} \tmA\n"
        str_prep += f"    UA: \t{ self.ua_sum / 64:4.2f}\tV\n"
        str_prep += f"    IA: \t{ self.ia_sum / 64 / 10.0:4.2f} \tmA\n"
        str_prep += f"    UE: \t{ self.ue_sum / 64:4.2f} \tV\n"
        str_prep += f"    IE: \t{ self.ie_sum / 64 / 10.0:4.2f} \tmA\n"
        str_prep += f"    TM: \t{ self.tm_sum / 64:4.2f} \t°C\n"
        str_prep += f"    ERR: \t0x{self.err:02x}\n"
        return str_prep

def _parse_error_frame(buf: bytes):
    if len(buf) < FrameSize.FRAME_TX_ERROR:
        raise ValueError("ERROR frame too short")
    if buf[0] != ResponseCode.RSP_ERROR:
        raise ValueError(f"expected RSP_ERROR, got 0x{buf[0]:02x}")

    code = buf[1]
    if code == ErrorCode.ERR_OUT_OF_RANGE and len(buf) >= FrameSize.FRAME_TX_OOR_ERROR:
        return OutOfRangeError(parameter_id=buf[2], value=buf[3], raw=bytes(buf))
    return ProtocolError(code=code, raw=bytes(buf))

def parse_response(buf: bytes):
    if not buf:
        raise ValueError("empty buffer")
    tag = buf[0]

    if crc8_run(buf[:-1]) != buf[-1]:
        raise ValueError("CRC mismatch")

    match tag:
        case ResponseCode.RSP_ACK:
            return RESPONSE_MAP[ResponseCode.RSP_ACK]
        case ResponseCode.RSP_USER_BREAK:
            return RESPONSE_MAP[ResponseCode.RSP_USER_BREAK]
        case ResponseCode.RSP_ALARM:
            return RESPONSE_MAP[ResponseCode.RSP_ALARM]
        case ResponseCode.RSP_ERROR:
            return _parse_error_frame(buf)
        case ResponseCode.RSP_DATA:
            return Data.from_bytes(buf)
        case _:
            raise ValueError(f"unknown response tag: 0x{tag:02x}")
