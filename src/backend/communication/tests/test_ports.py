"""Tier A — serial port listing (mocked comports)."""

from __future__ import annotations

from types import SimpleNamespace

import pytest

from backend.communication.ports import (
    SerialPortInfo,
    find_port_by_dedupe_key,
    find_port_by_device,
    format_port_label,
    list_serial_ports,
)


def _port(device: str, description: str = "", hwid: str | None = None) -> SimpleNamespace:
    return SimpleNamespace(device=device, description=description, hwid=hwid)


class TestFormatPortLabel:
    def test_description_and_device(self):
        assert format_port_label(device="COM3", description="USB Serial") == "USB Serial — COM3"

    def test_device_only_when_description_empty(self):
        assert format_port_label(device="/dev/cu.usbserial-1", description="") == "/dev/cu.usbserial-1"


class TestListSerialPorts:
    def test_sorts_by_label(self, monkeypatch):
        monkeypatch.setattr(
            "backend.communication.ports.comports",
            lambda: [
                _port("COM5", "B device"),
                _port("COM3", "A device"),
            ],
        )
        ports = list_serial_ports()
        assert [p.device for p in ports] == ["COM3", "COM5"]

    def test_prefers_macos_cu_over_tty(self, monkeypatch):
        monkeypatch.setattr("sys.platform", "darwin")
        monkeypatch.setattr(
            "backend.communication.ports.comports",
            lambda: [
                _port("/dev/tty.usbserial-10", "UART"),
                _port("/dev/cu.usbserial-10", "UART"),
            ],
        )
        ports = list_serial_ports()
        assert [p.device for p in ports] == ["/dev/cu.usbserial-10"]

    def test_include_all_false_filters_non_usb(self, monkeypatch):
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
