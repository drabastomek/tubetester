from dataclasses import dataclass
from typing import Union

# Frame size
class FrameSize:
    FRAME_RX_BYTES = 10
    FRAME_TX_DATA = 19
    FRAME_TX_ACK = 2
    FRAME_TX_ERROR = 3
    FRAME_TX_OOR_ERROR = 5

class CommandCode:
    CMD_SET = 0x00
    CMD_RESET = 0x01
    CMD_STATUS = 0x02

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
        case _:
            raise ValueError(f"Invalid command: {cmd}")
    return bytes(params + (crc8_run(bytes(params)),))


def _le16(buf: bytes, off: int) -> int:
    return buf[off] | (buf[off + 1] << 8)

@dataclass(frozen=True)
class Data:
    a1_a2: str
    uh_sum: int
    ih_sum: int
    ug_sum: int
    ua_sum: int
    ia_sum: int
    ue_sum: int
    ie_sum: int
    tm_sum: int
    raw: bytes

    @staticmethod
    def from_bytes(buf: bytes) -> Data:
        if len(buf) != FrameSize.FRAME_TX_DATA:
            raise ValueError(f"DATA frame must be {FrameSize.FRAME_TX_DATA} bytes")
        if buf[0] != ResponseCode.RSP_DATA:
            raise ValueError(f"expected RSP_DATA, got 0x{buf[0]:02x}")

        return Data(
            a1_a2=chr(buf[1]),
            uh_sum=_le16(buf, 2),
            ih_sum=_le16(buf, 4),
            ug_sum=_le16(buf, 6),
            ua_sum=_le16(buf, 8),
            ia_sum=_le16(buf, 10),
            ue_sum=_le16(buf, 12),
            ie_sum=_le16(buf, 14),
            tm_sum=_le16(buf, 16),
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
        return str_prep

def parse_response(buf: bytes):
    if not buf:
        raise ValueError("empty buffer")
    tag = buf[0]
    crc = buf[-1]

    if crc != crc8_run(buf[:-1]):
        raise ValueError("CRC mismatch")

    match tag:
        case ResponseCode.RSP_ACK :
            return RESPONSE_MAP[ResponseCode.RSP_ACK]
        case ResponseCode.RSP_USER_BREAK:
            return RESPONSE_MAP[ResponseCode.RSP_USER_BREAK]
        case ResponseCode.RSP_ALARM:
            return RESPONSE_MAP[ResponseCode.RSP_ALARM]
        case ResponseCode.RSP_ERROR:
            return RESPONSE_MAP[ResponseCode.RSP_ERROR]
        case ResponseCode.RSP_DATA:
            return Data.from_bytes(buf)
        case _:
            raise ValueError(f"unknown response tag: {RESPONSE_MAP[tag]}")
