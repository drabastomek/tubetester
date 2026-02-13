"""
Unit tests for backend.protocol: Protocol class (CRC-8, build_set_frame, build_meas_frame, parse_response).
No I/O; pure encoding/decoding.
"""
import pytest

from backend.protocol import (
    Protocol,
    FRAME_LEN,
    CTRL_SET,
    CTRL_MEAS,
    ERR_OK,
    ERR_CRC,
    ERR_OUT_OF_RANGE,
    ERR_INVALID_CMD,
    ALARM_OVERIH,
    ALARM_OVERTE,
    SET_TUH_INDEX_MAX,
    TUH_TICK_SCALE,
    UA_SCALE,
    UG1_P4_STEP,
    crc8,
    build_set_frame,
    build_meas_frame,
    verify_crc,
    parse_response,
)


# -----------------------------------------------------------------------------
# CRC-8
# -----------------------------------------------------------------------------

class TestProtocolCrc8:
    def test_crc8_empty(self):
        p = Protocol()
        assert p.crc8(b"") == 0

    def test_crc8_module_level(self):
        assert crc8(b"") == 0

    def test_crc8_seven_bytes_reproducible(self):
        """CRC of first 7 bytes of a frame; last byte is the CRC itself."""
        p = Protocol()
        data = bytes([0x20, 0x01, 0x06, 20, 15, 24, 10])  # SET frame without CRC
        c = p.crc8(data)
        assert isinstance(c, int)
        assert 0 <= c <= 255

    def test_verify_crc_valid(self):
        p = Protocol()
        frame = p.build_meas_frame(0)
        assert len(frame) == FRAME_LEN
        assert p.verify_crc(frame) is True
        assert verify_crc(frame) is True

    def test_verify_crc_invalid_wrong_byte7(self):
        p = Protocol()
        frame = bytearray(p.build_meas_frame(0))
        frame[7] ^= 0x01
        assert p.verify_crc(bytes(frame)) is False

    def test_verify_crc_short_frame(self):
        p = Protocol()
        assert p.verify_crc(b"") is False
        assert p.verify_crc(b"\x20\x00\x00\x00\x00\x00\x00") is False  # 7 bytes only


# -----------------------------------------------------------------------------
# build_set_frame
# -----------------------------------------------------------------------------

class TestProtocolBuildSetFrame:
    def test_length_and_crc(self):
        p = Protocol()
        frame = p.build_set_frame(
            index=1,
            heat_idx=6,
            ua_v=200,
            ug2_v=150,
            ug1_def=120,
            tuh_ticks=20,
        )
        assert len(frame) == FRAME_LEN
        assert p.verify_crc(frame) is True

    def test_set_frame_byte_layout(self):
        """SET: CTRL=0x20, index, heat_idx, P2, P3, P4, tuh_index, CRC."""
        p = Protocol()
        frame = p.build_set_frame(
            index=0xAB,
            heat_idx=3,
            ua_v=100,   # P2 = 10
            ug2_v=50,    # P3 = 5
            ug1_def=100, # P4 = 20
            tuh_ticks=4, # tuh_index = 2
        )
        assert frame[0] == CTRL_SET
        assert frame[1] == 0xAB
        assert frame[2] == 3
        assert frame[3] == 10
        assert frame[4] == 5
        assert frame[5] == 20
        assert frame[6] == 2
        assert p.crc8(frame[:7]) == frame[7]

    def test_set_frame_clamping_ua_ug2(self):
        p = Protocol()
        frame = p.build_set_frame(
            index=0, heat_idx=0, ua_v=400, ug2_v=400, ug1_def=0, tuh_ticks=0
        )
        assert frame[3] == 255  # 400/10 capped to 255
        assert frame[4] == 255

    def test_set_frame_tuh_index_capped(self):
        p = Protocol()
        frame = p.build_set_frame(
            index=0, heat_idx=0, ua_v=0, ug2_v=0, ug1_def=0,
            tuh_ticks=(SET_TUH_INDEX_MAX + 1) * TUH_TICK_SCALE,
        )
        assert frame[6] == SET_TUH_INDEX_MAX

    def test_build_set_frame_module_level(self):
        frame = build_set_frame(0, 0, 0, 0, 0, 0)
        assert len(frame) == FRAME_LEN
        assert verify_crc(frame) is True


# -----------------------------------------------------------------------------
# build_meas_frame
# -----------------------------------------------------------------------------

class TestProtocolBuildMeasFrame:
    def test_length_and_crc(self):
        p = Protocol()
        frame = p.build_meas_frame(0x11)
        assert len(frame) == FRAME_LEN
        assert p.verify_crc(frame) is True

    def test_meas_frame_byte_layout(self):
        p = Protocol()
        frame = p.build_meas_frame(0x11)
        assert frame[0] == CTRL_MEAS
        assert frame[1] == 0x11
        assert frame[2] == frame[3] == frame[4] == frame[5] == frame[6] == 0
        assert p.crc8(frame[:7]) == frame[7]

    def test_build_meas_frame_module_level(self):
        frame = build_meas_frame(0)
        assert len(frame) == FRAME_LEN
        assert verify_crc(frame) is True


# -----------------------------------------------------------------------------
# parse_response
# -----------------------------------------------------------------------------

def _ack_frame(index: int, err_code: int, param_id: int = 0, value: int = 0) -> bytes:
    """Build 8-byte ACK frame (same layout as firmware vttester_send_response)."""
    p = Protocol()
    buf = bytearray([0x02, index & 0xFF, err_code & 0xFF, 0, 0, 0, 0])
    if err_code == ERR_OUT_OF_RANGE:
        buf[3] = param_id & 0xFF
        buf[4] = (value >> 8) & 0xFF
        buf[5] = value & 0xFF
    buf[7] = p.crc8(bytes(buf[:7]))
    return bytes(buf)


def _meas_frame(index: int, ihlcd: int, ialcd_x10: int, ig2lcd: int, slcd: int, alarm_bits: int) -> bytes:
    """Build 8-byte measurement frame. ialcd in 0.01mA -> buf[3] = (ialcd+5)//10, etc."""
    p = Protocol()
    buf = bytearray(8)
    buf[0] = 0x02
    buf[1] = index & 0xFF
    buf[2] = min(ihlcd, 255)
    buf[3] = min((ialcd_x10 + 5) // 10, 255) if ialcd_x10 < 2550 else 255
    buf[4] = min((ig2lcd * 10 + 8) // 16, 255) if ig2lcd < 256 * 16 else 255
    buf[5] = min((slcd + 5) // 10, 255) if slcd < 1000 else 99
    buf[6] = alarm_bits & 0xF0
    buf[7] = p.crc8(bytes(buf[:7]))
    return bytes(buf)


class TestProtocolParseResponse:
    def test_parse_ack_ok(self):
        p = Protocol()
        frame = _ack_frame(index=1, err_code=ERR_OK)
        out = p.parse_response(frame)
        assert out["type"] == "ack"
        assert out["index"] == 1
        assert out["err_code"] == ERR_OK
        assert out["error_param"] == 0
        assert out["error_value"] == 0

    def test_parse_ack_out_of_range(self):
        p = Protocol()
        frame = _ack_frame(index=2, err_code=ERR_OUT_OF_RANGE, param_id=5, value=64)
        out = p.parse_response(frame)
        assert out["type"] == "ack"
        assert out["index"] == 2
        assert out["err_code"] == ERR_OUT_OF_RANGE
        assert out["error_param"] == 5
        assert out["error_value"] == 64

    def test_parse_ack_other_err_codes(self):
        p = Protocol()
        for err in (0x01, 0x03, 0x04, 0x06):
            frame = _ack_frame(index=0, err_code=err)
            out = p.parse_response(frame)
            assert out["type"] == "ack"
            assert out["err_code"] == err

    def test_parse_measurement(self):
        p = Protocol()
        # ihlcd=50, ialcd 100 (0.01mA units) -> buf[3]=10, slcd=500 -> buf[5]=50; ig2lcd=10 -> buf[4]=6 -> decode 8
        frame = _meas_frame(index=3, ihlcd=50, ialcd_x10=100, ig2lcd=10, slcd=500, alarm_bits=0)
        out = p.parse_response(frame)
        assert out["type"] == "measurement"
        assert out["index"] == 3
        assert out["ihlcd"] == 50
        assert out["ialcd"] == 100  # frame[3]*10
        assert out["ig2lcd"] == 8   # (6*16-8)//10 with buf[4]=6 from encoding 10
        assert out["slcd"] == 500
        assert out["alarm_bits"] == 0

    def test_parse_measurement_alarm_bits(self):
        p = Protocol()
        frame = _meas_frame(
            index=0, ihlcd=0, ialcd_x10=0, ig2lcd=0, slcd=0,
            alarm_bits=ALARM_OVERIH | ALARM_OVERTE,
        )
        out = p.parse_response(frame)
        assert out["type"] == "measurement"
        assert out["alarm_bits"] == (ALARM_OVERIH | ALARM_OVERTE)

    def test_parse_error_crc(self):
        p = Protocol()
        frame = bytearray(_ack_frame(0, ERR_OK))
        frame[7] ^= 0x01
        out = p.parse_response(bytes(frame))
        assert out["type"] == "error"
        assert out["err_code"] == ERR_CRC

    def test_parse_error_short_frame(self):
        p = Protocol()
        out = p.parse_response(b"\x02\x00\x00\x00\x00\x00\x00")  # 7 bytes
        assert out["type"] == "error"
        assert out["err_code"] == ERR_CRC

    def test_parse_response_module_level(self):
        frame = _ack_frame(0, ERR_OK)
        out = parse_response(frame)
        assert out["type"] == "ack"
        assert out["err_code"] == ERR_OK


# -----------------------------------------------------------------------------
# Round-trip
# -----------------------------------------------------------------------------

class TestProtocolRoundTrip:
    def test_set_build_then_parse_ack(self):
        p = Protocol()
        set_frame = p.build_set_frame(1, 6, 200, 150, 120, 20)
        assert p.verify_crc(set_frame)
        # Device would respond with ACK; we build ACK and parse it
        ack = _ack_frame(1, ERR_OK)
        out = p.parse_response(ack)
        assert out["type"] == "ack"
        assert out["index"] == 1

    def test_meas_build_then_parse_measurement(self):
        p = Protocol()
        meas_frame = p.build_meas_frame(1)
        assert p.verify_crc(meas_frame)
        resp = _meas_frame(1, ihlcd=100, ialcd_x10=50, ig2lcd=5, slcd=250, alarm_bits=0)
        out = p.parse_response(resp)
        assert out["type"] == "measurement"
        assert out["index"] == 1
        assert out["ihlcd"] == 100
