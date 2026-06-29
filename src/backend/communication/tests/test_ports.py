"""Tier A — serial port listing (mocked comports)."""

from __future__ import annotations

from types import SimpleNamespace

import pytest

from backend.communication.ports import (
    SerialPortInfo,
    find_port_by_dedupe_key,
    find_port_by_device,
    format_port_label,
    has_usable_port_name,
    list_serial_ports,
)


def _port(device: str, description: str = "", hwid: str | None = None) -> SimpleNamespace:
    return SimpleNamespace(device=device, description=description, hwid=hwid)


class TestFormatPortLabel:
    def test_shows_device_path_not_description(self):
        assert format_port_label(device="COM3", description="USB Serial") == "COM3"

    def test_device_when_description_empty(self):
        assert format_port_label(device="/dev/cu.usbserial-1", description="") == "/dev/cu.usbserial-1"

    def test_device_when_description_na(self):
        assert format_port_label(device="/dev/cu.usbmodem1", description="n/a") == "/dev/cu.usbmodem1"


class TestHasUsablePortName:
    def test_rejects_na_and_empty(self):
        assert not has_usable_port_name("")
        assert not has_usable_port_name("N/A")
        assert not has_usable_port_name("n/a")

    def test_accepts_real_name(self):
        assert has_usable_port_name("USB Serial")


class TestListSerialPorts:
    def test_sorts_by_label(self, monkeypatch):
        monkeypatch.setattr("sys.platform", "linux")
        monkeypatch.setattr(
            "backend.communication.ports.comports",
            lambda: [
                _port("COM5", "B device"),
                _port("COM3", "A device"),
            ],
        )
        ports = list_serial_ports()
        assert [p.device for p in ports] == ["COM3", "COM5"]
        assert [p.label for p in ports] == ["COM3", "COM5"]

    def test_macos_lists_named_cu_only(self, monkeypatch):
        monkeypatch.setattr("sys.platform", "darwin")
        monkeypatch.setattr(
            "backend.communication.ports.comports",
            lambda: [
                _port("/dev/tty.usbserial-10", "UART Bridge"),
                _port("/dev/cu.usbserial-10", "UART Bridge"),
                _port("/dev/cu.usbmodem99", "n/a"),
                _port("/dev/cu.Bluetooth-Incoming-Port", ""),
            ],
        )
        ports = list_serial_ports()
        assert [p.device for p in ports] == ["/dev/cu.usbserial-10"]
        assert ports[0].label == "/dev/cu.usbserial-10"

    def test_include_all_false_filters_non_usb(self, monkeypatch):
        monkeypatch.setattr("sys.platform", "linux")
        monkeypatch.setattr(
            "backend.communication.ports.comports",
            lambda: [
                _port("COM3", "USB Serial"),
                _port("/dev/pty0", "pseudo terminal"),
            ],
        )
        ports = list_serial_ports(include_all=False)
        assert [p.device for p in ports] == ["COM3"]

    def test_find_helpers(self):
        ports = [
            SerialPortInfo("COM3", "USB — COM3", "USB", hwid="USB VID:PID=1A86:7523"),
            SerialPortInfo("COM5", "USB — COM5", "USB", hwid="USB VID:PID=10C4:EA60"),
        ]
        assert find_port_by_device(ports, "COM5") == ports[1]
        assert find_port_by_dedupe_key(ports, "USB VID:PID=1A86:7523") == ports[0]
        assert find_port_by_device(ports, "COM9") is None

    def test_missing_pyserial_raises(self, monkeypatch):
        monkeypatch.setattr("backend.communication.ports.comports", None)
        with pytest.raises(RuntimeError, match="pyserial"):
            list_serial_ports()
