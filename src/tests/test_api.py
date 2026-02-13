"""
Unit tests for backend.api: VTTesterClient with mocked SerialDriver (no real serial I/O).
"""
import pytest
from unittest.mock import MagicMock, patch

from backend.protocol import Protocol, ERR_OK, ERR_OUT_OF_RANGE, ALARM_OVERIH
from backend import api
from backend.api import VTTesterClient, _alarm_info


def _ack_frame(index: int, err_code: int, param_id: int = 0, value: int = 0) -> bytes:
    p = Protocol()
    buf = bytearray([0x02, index & 0xFF, err_code & 0xFF, 0, 0, 0, 0])
    if err_code == ERR_OUT_OF_RANGE:
        buf[3] = param_id & 0xFF
        buf[4] = (value >> 8) & 0xFF
        buf[5] = value & 0xFF
    buf[7] = p.crc8(bytes(buf[:7]))
    return bytes(buf)


def _meas_frame(index: int, ihlcd: int, ialcd_x10: int, ig2lcd: int, slcd: int, alarm_bits: int) -> bytes:
    p = Protocol()
    buf = bytearray(8)
    buf[0], buf[1] = 0x02, index & 0xFF
    buf[2] = min(ihlcd, 255)
    buf[3] = min((ialcd_x10 + 5) // 10, 255) if ialcd_x10 < 2550 else 255
    buf[4] = min((ig2lcd * 10 + 8) // 16, 255) if ig2lcd < 256 * 16 else 255
    buf[5] = min((slcd + 5) // 10, 255) if slcd < 1000 else 99
    buf[6] = alarm_bits & 0xF0
    buf[7] = p.crc8(bytes(buf[:7]))
    return bytes(buf)


# -----------------------------------------------------------------------------
# Fixtures: client with mocked driver
# -----------------------------------------------------------------------------

@pytest.fixture
def mock_driver():
    driver = MagicMock()
    driver.is_open = True
    return driver


@pytest.fixture
def client(mock_driver):
    with patch.object(api, "SerialDriver", return_value=mock_driver):
        c = VTTesterClient(port="/dev/ttyUSB0", timeout_s=1.0)
        c._driver = mock_driver
        return c


# -----------------------------------------------------------------------------
# open / close / is_open / handshake
# -----------------------------------------------------------------------------

class TestVTTesterClientLifecycle:
    def test_open_calls_driver_open(self, client, mock_driver):
        client.open()
        mock_driver.open.assert_called_once_with("/dev/ttyUSB0")

    def test_open_with_arg_uses_arg(self, client, mock_driver):
        client.open("/dev/ttyACM0")
        mock_driver.open.assert_called_once_with("/dev/ttyACM0")

    def test_open_without_port_raises(self):
        with patch.object(api, "SerialDriver", return_value=MagicMock()):
            c = VTTesterClient()
            with pytest.raises(ValueError, match="Port not specified"):
                c.open()

    def test_close_calls_driver_close(self, client, mock_driver):
        client.close()
        mock_driver.close.assert_called_once()

    def test_is_open_delegates(self, client, mock_driver):
        mock_driver.is_open = True
        assert client.is_open is True
        mock_driver.is_open = False
        assert client.is_open is False

    def test_handshake_delegates(self, client, mock_driver):
        mock_driver.handshake.return_value = True
        assert client.handshake() is True
        mock_driver.handshake.assert_called_once()
        call_kw = mock_driver.handshake.call_args[1]
        assert call_kw["timeout_s"] == 1.0


# -----------------------------------------------------------------------------
# set_params
# -----------------------------------------------------------------------------

class TestVTTesterClientSetParams:
    def test_set_params_sends_frame_and_returns_ack(self, client, mock_driver):
        mock_driver.recv_frame.return_value = _ack_frame(index=1, err_code=ERR_OK)
        out = client.set_params(heat_idx=6, ua_v=200, ug2_v=150, ug1_def=120, tuh_ticks=20)
        mock_driver.send_frame.assert_called_once()
        frame = mock_driver.send_frame.call_args[0][0]
        assert len(frame) == 8
        assert frame[0] == 0x20  # SET
        assert frame[2] == 6
        mock_driver.recv_frame.assert_called_once()
        assert out["type"] == "ack"
        assert out["err_code"] == ERR_OK

    def test_set_params_calls_on_status_update_on_ack(self, client, mock_driver):
        mock_driver.recv_frame.return_value = _ack_frame(index=1, err_code=ERR_OK)
        callback = MagicMock()
        client.on_status_update = callback
        client.set_params(heat_idx=0, ua_v=0, ug2_v=0, ug1_def=240, tuh_ticks=0)
        callback.assert_called_once()
        assert callback.call_args[0][0]["type"] == "ack"
        assert callback.call_args[0][0]["err_code"] == ERR_OK

    def test_set_params_out_of_range_returns_ack_with_error_param(self, client, mock_driver):
        mock_driver.recv_frame.return_value = _ack_frame(
            index=1, err_code=ERR_OUT_OF_RANGE, param_id=5, value=64
        )
        out = client.set_params(heat_idx=0, ua_v=0, ug2_v=0, ug1_def=0, tuh_ticks=0)
        assert out["type"] == "ack"
        assert out["err_code"] == ERR_OUT_OF_RANGE
        assert out["error_param"] == 5
        assert out["error_value"] == 64

    def test_set_params_no_callback_when_none(self, client, mock_driver):
        mock_driver.recv_frame.return_value = _ack_frame(1, ERR_OK)
        client.on_status_update = None
        out = client.set_params()
        assert out["type"] == "ack"


# -----------------------------------------------------------------------------
# start_measurement
# -----------------------------------------------------------------------------

class TestVTTesterClientStartMeasurement:
    def test_start_measurement_returns_measurement_dict(self, client, mock_driver):
        mock_driver.recv_frame.return_value = _meas_frame(
            index=1, ihlcd=100, ialcd_x10=50, ig2lcd=5, slcd=250, alarm_bits=0
        )
        out = client.start_measurement()
        assert out["type"] == "measurement"
        assert out["index"] == 1
        assert out["ihlcd"] == 100
        assert "ialcd" in out
        assert "slcd" in out
        assert out["alarm_bits"] == 0

    def test_start_measurement_calls_on_measurement_result(self, client, mock_driver):
        mock_driver.recv_frame.return_value = _meas_frame(
            index=1, ihlcd=50, ialcd_x10=0, ig2lcd=0, slcd=0, alarm_bits=0
        )
        callback = MagicMock()
        client.on_measurement_result = callback
        client.start_measurement()
        callback.assert_called_once()
        arg = callback.call_args[0][0]
        assert arg["type"] == "measurement"
        assert arg["ihlcd"] == 50

    def test_start_measurement_with_alarm_calls_on_alarm(self, client, mock_driver):
        mock_driver.recv_frame.return_value = _meas_frame(
            index=1, ihlcd=0, ialcd_x10=0, ig2lcd=0, slcd=0, alarm_bits=ALARM_OVERIH
        )
        on_meas = MagicMock()
        on_alarm = MagicMock()
        client.on_measurement_result = on_meas
        client.on_alarm = on_alarm
        client.start_measurement()
        on_meas.assert_called_once()
        on_alarm.assert_called_once()
        alarm_info = on_alarm.call_args[0][0]
        assert alarm_info["alarm_bits"] == ALARM_OVERIH
        assert alarm_info["over_ih"] is True
        assert "measurement" in alarm_info

    def test_start_measurement_no_alarm_does_not_call_on_alarm(self, client, mock_driver):
        mock_driver.recv_frame.return_value = _meas_frame(
            index=1, ihlcd=0, ialcd_x10=0, ig2lcd=0, slcd=0, alarm_bits=0
        )
        client.on_alarm = MagicMock()
        client.start_measurement()
        client.on_alarm.assert_not_called()


# -----------------------------------------------------------------------------
# _request timeout
# -----------------------------------------------------------------------------

class TestVTTesterClientRequest:
    def test_request_timeout_returns_error_dict(self, client, mock_driver):
        mock_driver.recv_frame.return_value = None
        out = client._request(b"\x20\x00\x00\x00\x00\x00\x00\x00")  # any 8 bytes
        assert out["type"] == "error"
        assert out.get("timeout") is True
        assert out["err_code"] == api.ERR_CRC


# -----------------------------------------------------------------------------
# read_status / reset_params (placeholders)
# -----------------------------------------------------------------------------

class TestVTTesterClientPlaceholders:
    def test_read_status_returns_stub(self, client):
        out = client.read_status()
        assert out["type"] == "status"
        assert out.get("stub") is True
        assert "message" in out

    def test_reset_params_returns_stub(self, client):
        out = client.reset_params()
        assert out["type"] == "reset_params"
        assert out.get("stub") is True
        assert "message" in out


# -----------------------------------------------------------------------------
# _alarm_info helper
# -----------------------------------------------------------------------------

class TestAlarmInfo:
    def test_alarm_info_over_ih(self):
        meas = {"alarm_bits": ALARM_OVERIH, "ihlcd": 0}
        info = _alarm_info(meas)
        assert info["alarm_bits"] == ALARM_OVERIH
        assert info["over_ih"] is True
        assert info["over_ia"] is False
        assert info["measurement"] is meas

    def test_alarm_info_multiple_bits(self):
        meas = {"alarm_bits": ALARM_OVERIH | 0x20}
        info = _alarm_info(meas)
        assert info["over_ih"] is True
        assert info["over_ia"] is True
