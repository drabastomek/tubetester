"""Tier A — serial connection service (mocked transport)."""

from __future__ import annotations

from unittest.mock import MagicMock

import pytest

from backend.communication.connection import (
    ConnectionState,
    SerialConnectionService,
    SerialNotConnectedError,
    verify_tester_handshake,
)
from backend.communication.ports import SerialPortInfo
from backend.communication.protocol import Data
from backend.communication.serial_prefs import SerialPreferences


def _data_response() -> Data:
    return Data(
        a1_a2="0",
        ug_sum=0,
        uh_sum=0,
        ih_sum=0,
        ua_sum=0,
        ia_sum=0,
        ue_sum=0,
        ie_sum=0,
        tm_sum=0,
        err=0,
        raw=b"\x00" * 20,
    )


class FakeLink:
  instances: list["FakeLink"] = []

  def __init__(self, port: str, **kwargs: object) -> None:
      self.port = port
      self.kwargs = kwargs
      self.closed = False
      self.responses: list[object] = [_data_response()]
      FakeLink.instances.append(self)

  def close(self) -> None:
      self.closed = True

  def drain(self) -> None:
      return None

  def send_message(self, frame: bytes) -> None:
      return None

  def read_response(self, timeout_s: float | None = None) -> object | None:
      if self.responses:
          return self.responses.pop(0)
      return None


@pytest.fixture(autouse=True)
def _reset_fake_links():
    FakeLink.instances.clear()
    yield
    FakeLink.instances.clear()


@pytest.fixture
def service(tmp_path) -> SerialConnectionService:
    return SerialConnectionService(
        serial_factory=lambda port, **kw: FakeLink(port, **kw),  # type: ignore[return-value]
        prefs_path=str(tmp_path / "serial_prefs.json"),
    )


def test_connect_handshake_and_disconnect(service: SerialConnectionService):
    changes: list[str] = []
    service.add_listener(lambda s: changes.append(s.state))

    status = service.connect("COM3")
    assert status.state == ConnectionState.CONNECTED
    assert status.device == "COM3"
    assert service.is_connected
    assert ConnectionState.CONNECTING in changes
    assert changes[-1] == ConnectionState.CONNECTED

    with service.session() as link:
        assert link.port == "COM3"

    disconnected = service.disconnect()
    assert disconnected.state == ConnectionState.DISCONNECTED
    assert not service.is_connected


def test_connect_fails_without_data_response(service: SerialConnectionService):
    def factory(port: str, **kw: object) -> FakeLink:
        link = FakeLink(port, **kw)
        link.responses = [None]
        return link

    svc = SerialConnectionService(
        serial_factory=factory,  # type: ignore[arg-type]
        prefs_path=service._prefs_path,
    )
    status = svc.connect("COM3")
    assert status.state == ConnectionState.DISCONNECTED
    assert status.message is not None
    assert "No VTTester response" in status.message
    assert FakeLink.instances[0].closed


def test_require_link_when_disconnected(service: SerialConnectionService):
    with pytest.raises(SerialNotConnectedError):
        service.require_link()


def test_verify_tester_handshake_accepts_data():
    link = MagicMock()
    link.read_response.return_value = _data_response()
    assert verify_tester_handshake(link, timeout_s=0.1) is True
    link.send_message.assert_called_once()


def test_remember_last_device(service: SerialConnectionService, monkeypatch):
    port_info = SerialPortInfo(
        "COM3",
        "USB — COM3",
        "USB",
        hwid="USB VID:PID=1A86:7523",
    )
    monkeypatch.setattr(
        "backend.communication.connection.list_serial_ports",
        lambda **_: [port_info],
    )
    service.connect("COM3")
    prefs = service.load_preferences()
    assert prefs.last_device == "COM3"
    assert prefs.last_dedupe_key == "USB VID:PID=1A86:7523"


def test_preferred_port_by_dedupe_key(service: SerialConnectionService):
    service.save_preferences(
        SerialPreferences(
            last_device="COM3",
            last_dedupe_key="USB VID:PID=1A86:7523",
        )
    )
    ports = [
        SerialPortInfo(
            "COM7",
            "USB — COM7",
            "USB",
            hwid="USB VID:PID=1A86:7523",
        )
    ]
    found = service.preferred_port(ports)
    assert found is not None
    assert found.device == "COM7"


def test_connect_async_invokes_callback(service: SerialConnectionService):
    seen: list[ConnectionState] = []
    thread = service.connect_async(
        "COM3",
        callback=lambda s: seen.append(s.state),
    )
    thread.join(timeout=2.0)
    assert seen[-1] == ConnectionState.CONNECTED
