"""Tier A — codec and frame vectors (no hardware)."""

import pytest

from backend.communication.protocol import (
    CommandCode,
    Data,
    ErrorCode,
    FrameSize,
    HARNS_FAULT_ALARM_ON_SET,
    OutOfRangeError,
    ProtocolError,
    ResetKind,
    ResponseCode,
    corrupt_crc,
    crc8_run,
    parse_response,
    prepare_beep,
    prepare_harness_arm,
    prepare_raw_request,
    prepare_request,
)


def _with_crc(payload: list[int]) -> bytes:
    body = bytes(payload)
    return body + bytes([crc8_run(body)])


class TestCrc8:
    def test_empty(self):
        assert crc8_run(b"") == 0

    def test_known_single_byte(self):
        # crc = table[0 ^ 0x00] = 0
        assert crc8_run(bytes([0x00])) == 0

    def test_firmware_table_sample(self):
        data = bytes(range(9))
        assert crc8_run(data) == 131


class TestPrepareRequest:
    def test_set_is_10_bytes_with_valid_crc(self):
        frame = prepare_request(
            CommandCode.CMD_SET,
            a1_a2="0",
            ug=100,
            uh=63,
            ua=300,
        )
        assert len(frame) == FrameSize.FRAME_RX_BYTES
        assert frame[0] == CommandCode.CMD_SET
        assert frame[-1] == crc8_run(frame[:-1])

    def test_reset_layout(self):
        frame = prepare_request(
            CommandCode.CMD_RESET, reset_kind=ResetKind.RESET_FULL
        )
        assert frame[0] == CommandCode.CMD_RESET
        assert frame[1] == ResetKind.RESET_FULL
        assert frame[2:9] == bytes(7)

    def test_status_layout(self):
        frame = prepare_request(CommandCode.CMD_STATUS, a1_a2="1")
        assert frame[0] == CommandCode.CMD_STATUS
        assert frame[1] == ord("1")

    def test_beep_layout(self):
        frame = prepare_beep(10)
        assert len(frame) == FrameSize.FRAME_RX_BYTES
        assert frame[0] == CommandCode.CMD_BEEP
        assert frame[1] == 10
        assert frame[2:9] == bytes(7)
        assert frame[-1] == crc8_run(frame[:-1])

    def test_beep_default(self):
        frame = prepare_beep()
        assert frame[1] == 0


class TestParseResponse:
    def test_ack(self):
        frame = _with_crc([ResponseCode.RSP_ACK])
        assert parse_response(frame) == "RSP_ACK"

    def test_user_break(self):
        frame = _with_crc([ResponseCode.RSP_USER_BREAK])
        assert parse_response(frame) == "RSP_USER_BREAK"

    def test_alarm(self):
        frame = _with_crc([ResponseCode.RSP_ALARM, 0x01])
        assert parse_response(frame) == "RSP_ALARM"

    def test_generic_error(self):
        frame = _with_crc([ResponseCode.RSP_ERROR, ErrorCode.ERR_INVALID_CMD])
        result = parse_response(frame)
        assert isinstance(result, ProtocolError)
        assert result.code == ErrorCode.ERR_INVALID_CMD

    def test_out_of_range_error(self):
        frame = _with_crc(
            [ResponseCode.RSP_ERROR, ErrorCode.ERR_OUT_OF_RANGE, 0x03, 241]
        )
        result = parse_response(frame)
        assert isinstance(result, OutOfRangeError)
        assert result.parameter_id == 0x03
        assert result.value == 241

    def test_data_v05_layout(self):
        payload = [
            ResponseCode.RSP_DATA,
            ord("1"),
            0x02,
            0x01,  # ug
            0x04,
            0x03,  # uh
            0x06,
            0x05,  # ih
            0x08,
            0x07,  # ua
            0x0A,
            0x09,  # ia
            0,
            0,  # ue
            0,
            0,  # ie
            0x0C,
            0x0B,  # tm
            0x05,  # err @ byte 18
        ]
        frame = _with_crc(payload)
        assert len(frame) == FrameSize.FRAME_TX_DATA

        data = parse_response(frame)
        assert isinstance(data, Data)
        assert data.a1_a2 == "1"
        assert data.ug_sum == 0x0102
        assert data.uh_sum == 0x0304
        assert data.ih_sum == 0x0506
        assert data.ua_sum == 0x0708
        assert data.ia_sum == 0x090A
        assert data.tm_sum == 0x0B0C
        assert data.err == 0x05

    def test_crc_mismatch_raises(self):
        frame = _with_crc([ResponseCode.RSP_ACK])
        bad = frame[:-1] + bytes([(frame[-1] + 1) & 0xFF])
        with pytest.raises(ValueError, match="CRC"):
            parse_response(bad)

    def test_wrong_data_length_raises(self):
        frame = _with_crc([ResponseCode.RSP_DATA] + [0] * 17)
        with pytest.raises(ValueError, match="20 bytes"):
            parse_response(frame)


class TestHarnessHelpers:
    def test_prepare_harness_arm(self):
        frame = prepare_harness_arm(HARNS_FAULT_ALARM_ON_SET)
        assert len(frame) == FrameSize.FRAME_RX_BYTES
        assert frame[0] == CommandCode.CMD_STATUS
        assert frame[1] == ord("H")
        assert frame[4] == HARNS_FAULT_ALARM_ON_SET

    def test_prepare_raw_request(self):
        frame = prepare_raw_request((0x7F,) + (0,) * 8)
        assert len(frame) == FrameSize.FRAME_RX_BYTES
        assert frame[-1] == crc8_run(frame[:-1])

    def test_corrupt_crc_changes_last_byte(self):
        good = prepare_request(CommandCode.CMD_STATUS)
        bad = corrupt_crc(good)
        assert bad[:-1] == good[:-1]
        assert bad[-1] != good[-1]
