"""
VTTester protocol v0.4.1 framing (must match firmware/communication/communication.h).

Spec: firmware/protocol/VTTester_Protocol_v0.4.1.txt

DATA uint16 fields are sums of 64 ADC samples (§10.1); convert on the PC.
INDEX: bits 7-4 tube type, 3-1 system, bit 0 heater wiring (0=parallel, 1=series).
SET P1: heater — 0.1 V steps if parallel, 10 mA steps if series (§6).
SET P2–P4: 12-bit Ua/Ug2 encoding (§6). P5: |Ug1| magnitude. P6: time index.
"""

from __future__ import annotations

from dataclasses import dataclass

# --- CTRL / errors (mirror firmware #defines) ---
REQ_SET = 0x00
REQ_STATUS = 0x40
REQ_RESET = 0x80

RSP_ACK = 0x02
RSP_BUSY = 0x42
RSP_ERROR = 0x82
RSP_DATA = 0x20

ERR_UNKNOWN = 0x00
ERR_OUT_OF_RANGE = 0x02
ERR_CRC = 0x03
ERR_INVALID_CMD = 0x04
ERR_HARDWARE = 0x06

RESET_UA_UG2_UG1 = 0x01
RESET_FULL = 0x02

IDX_HEATER_SERIES = 0x01

FRAME_BYTES = 9

# CRC-8 poly 0x07, init 0 — same 256-byte table as firmware
CRC8_TABLE: tuple[int, ...] = (
    0, 94,188,226,97,63,221,131,194,156,126,32,163,253,31,65,157,195,33,127,252,162,64,30,95,1,227,189,62,96,130,220,35,125,159,193,66,28,254,160,225,191,93,3,128,222,60,98,190,224,2,92,223,129,99,61,124,34,192,158,29,67,161,255,70,24,250,164,39,121,155,197,132,218,56,102,229,187,89,7,219,133,103,57,186,228,6,88,25,71,165,251,120,38,196,154,101,59,217,135,4,90,184,230,167,249,27,69,198,152,122,36,248,166,68,26,153,199,37,123,58,100,134,216,91,5,231,185,140,210,48,110,237,179,81,15,78,16,242,172,47,113,147,205,17,79,173,243,112,46,204,146,211,141,111,49,178,236,14,80,175,241,19,77,206,144,114,44,109,51,209,143,12,82,176,238,50,108,142,208,83,13,239,177,240,174,76,18,145,207,45,115,202,148,118,40,171,245,23,73,8,86,180,234,105,55,213,139,87,9,235,181,54,104,138,212,149,203,41,119,244,170,72,22,233,183,85,11,136,214,52,106,43,117,151,201,74,20,246,168,116,42,200,150,21,75,169,247,182,232,10,84,215,137,107,53,
)


def crc8(payload08: bytes) -> int:
    """CRC over bytes 0–7; wire byte 8 is CRC (v0.4.1 §2.2)."""
    if len(payload08) != 8:
        raise ValueError("crc8 expects exactly 8 bytes")
    c = 0
    for x in payload08:
        c = CRC8_TABLE[c ^ x]
    return c


def pack_index(tube_type: int, system: int, heater_series: int = 0) -> int:
    """§5: tube_type 0-5 in bits 7-4; system 0-2 in bits 3-1; bit0 series heater."""
    return ((tube_type & 0x0F) << 4) | ((system & 0x07) << 1) | (heater_series & 1)


def unpack_index(idx: int) -> tuple[int, int, int]:
    """Return (tube_type, system, heater_series)."""
    return ((idx >> 4) & 0x0F, (idx >> 1) & 0x07, idx & 1)


def pack_ua_ug2_for_set(ua: int, ug2: int) -> tuple[int, int, int]:
    """
    §6: P2 = Ua low 8, P3 = high nibble Ua (11–8) | high nibble Ug2 (11–8), P4 = Ug2 low 8.
    ua and ug2 are 12-bit (0..0xFFF).
    """
    if not (0 <= ua <= 0xFFF and 0 <= ug2 <= 0xFFF):
        raise ValueError("ua and ug2 must be 12-bit (0..4095)")
    p2 = ua & 0xFF
    p3 = ((ua >> 8) & 0x0F) << 4 | ((ug2 >> 8) & 0x0F)
    p4 = ug2 & 0xFF
    return (p2, p3, p4)


@dataclass(frozen=True)
class Frame:
    """9-byte wire frame (v0.4.1 §2.1)."""

    ctrl: int
    idx: int
    p1: int
    p2: int
    p3: int
    p4: int
    p5: int
    p6: int
    crc: int

    def to_bytes(self) -> bytes:
        head = bytes(
            (
                self.ctrl,
                self.idx,
                self.p1,
                self.p2,
                self.p3,
                self.p4,
                self.p5,
                self.p6,
            )
        )
        c = crc8(head)
        return head + bytes((c,))

    @staticmethod
    def from_bytes_wire(buf: bytes) -> Frame:
        if len(buf) != FRAME_BYTES:
            raise ValueError(f"frame must be {FRAME_BYTES} bytes")

        got = buf[8]
        expect = crc8(buf[:8])
        if got != expect:
            raise ValueError(f"CRC mismatch: got 0x{got:02x}, expect 0x{expect:02x}")

        return Frame(
            ctrl=buf[0],
            idx=buf[1],
            p1=buf[2],
            p2=buf[3],
            p3=buf[4],
            p4=buf[5],
            p5=buf[6],
            p6=buf[7],
            crc=got,
        )

    def __str__(self) -> str:
        return (
            f"Frame(ctrl=0x{self.ctrl:02x}, idx=0x{self.idx:02x}, "
            f"p1=0x{self.p1:02x}, p2=0x{self.p2:02x}, p3=0x{self.p3:02x}, "
            f"p4=0x{self.p4:02x}, p5=0x{self.p5:02x}, p6=0x{self.p6:02x}, crc=0x{self.crc:02x})"
        )


def build_set_frame(
    idx: int,
    p1_heater: int,
    ua_12: int,
    ug2_12: int,
    p5_ug1_mag: int,
    p6_time_idx: int,
) -> bytes:
    """SET request (CTRL low 6 bits must be 0). Ua/Ug2 as 12-bit (§6)."""
    p2, p3, p4 = pack_ua_ug2_for_set(ua_12, ug2_12)
    return Frame(
        ctrl=REQ_SET,
        idx=idx,
        p1=p1_heater,
        p2=p2,
        p3=p3,
        p4=p4,
        p5=p5_ug1_mag & 0xFF,
        p6=p6_time_idx & 0xFF,
        crc=0,
    ).to_bytes()


def build_status_frame(idx: int = 0) -> bytes:
    return Frame(
        ctrl=REQ_STATUS,
        idx=idx,
        p1=0,
        p2=0,
        p3=0,
        p4=0,
        p5=0,
        p6=0,
        crc=0,
    ).to_bytes()


def build_reset_frame(idx: int = 0, p1: int = RESET_FULL) -> bytes:
    return Frame(
        ctrl=REQ_RESET,
        idx=idx,
        p1=p1,
        p2=0,
        p3=0,
        p4=0,
        p5=0,
        p6=0,
        crc=0,
    ).to_bytes()


def le16(lo: int, hi: int) -> int:
    return lo | (hi << 8)


def is_data_frame(ctrl: int) -> bool:
    return (ctrl & 0x20) != 0


@dataclass(frozen=True)
class MeasurementRawSums:
    """Three DATA frames after SET (v0.4.1 §10.2): uint16 LE sums of 64 ADC samples."""

    idx: int
    uh_sum: int
    ih_sum: int
    ua_sum: int
    ug2_sum: int
    ia_sum: int
    ig2_sum: int
    ug1_sum: int
    alarm: int


def parse_measurement_data_frames(frames: list[Frame]) -> MeasurementRawSums | None:
    """
    From responses that include exactly three consecutive DATA frames (CTRL bit5 set),
    in order frame1..frame3, build MeasurementRawSums. Returns None if not found/partial.
    """
    data = [f for f in frames if is_data_frame(f.ctrl)]
    if len(data) < 3:
        return None
    d = data[:3]
    if len({x.idx for x in d}) != 1:
        return None
    idx = d[0].idx
    uh = le16(d[0].p1, d[0].p2)
    ih = le16(d[0].p3, d[0].p4)
    ua = le16(d[1].p1, d[1].p2)
    ug2 = le16(d[1].p3, d[1].p4)
    ug1 = le16(d[1].p5, d[1].p6)
    ia = le16(d[2].p1, d[2].p2)
    ig2 = le16(d[2].p3, d[2].p4)
    alarm = d[2].p5
    return MeasurementRawSums(
        idx=idx,
        uh_sum=uh,
        ih_sum=ih,
        ua_sum=ua,
        ug2_sum=ug2,
        ia_sum=ia,
        ig2_sum=ig2,
        ug1_sum=ug1,
        alarm=alarm,
    )
