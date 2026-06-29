"""Cross-platform serial port enumeration for the VTTester desktop app."""

from __future__ import annotations

import sys
from dataclasses import dataclass
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Iterable, Sequence

try:
    from serial.tools.list_ports import comports
except ImportError:  # pragma: no cover - pyserial optional at import in some envs
    comports = None  # type: ignore[assignment,misc]


@dataclass(frozen=True)
class SerialPortInfo:
    """One selectable serial device."""

    device: str
    label: str
    description: str
    hwid: str | None = None

    @property
    def dedupe_key(self) -> str:
        """Stable key for re-selecting the same USB adapter after replug."""
        return self.hwid or self.device


def format_port_label(
    *,
    device: str,
    description: str | None = None,
) -> str:
    desc = (description or "").strip()
    if desc and desc.lower() not in device.lower():
        return f"{desc} — {device}"
    return device


def _info_from_list_port(port: object) -> SerialPortInfo:
    device = str(getattr(port, "device", ""))
    description = str(getattr(port, "description", "") or "").strip()
    hwid = getattr(port, "hwid", None)
    hwid_str = str(hwid).strip() if hwid else None
    return SerialPortInfo(
        device=device,
        label=format_port_label(device=device, description=description),
        description=description,
        hwid=hwid_str or None,
    )


def _prefer_macos_callout_ports(ports: Sequence[SerialPortInfo]) -> list[SerialPortInfo]:
    """Drop ``/dev/tty.*`` when the matching ``/dev/cu.*`` port is present."""
    if sys.platform != "darwin":
        return list(ports)
    devices = {p.device for p in ports}
    kept: list[SerialPortInfo] = []
    for port in ports:
        if port.device.startswith("/dev/tty."):
            cu = "/dev/cu." + port.device[len("/dev/tty.") :]
            if cu in devices:
                continue
        kept.append(port)
    return kept


def list_serial_ports(*, include_all: bool = True) -> list[SerialPortInfo]:
    """
    Return serial ports available on this machine, sorted by label.

    On macOS, hides paired ``/dev/tty.*`` entries when ``/dev/cu.*`` exists.
    """
    if comports is None:
        raise RuntimeError("pyserial is not installed")

    raw = [_info_from_list_port(p) for p in comports()]
    filtered = _prefer_macos_callout_ports(raw)
    if not include_all:
        filtered = [p for p in filtered if _looks_like_usb_uart(p)]
    filtered.sort(key=lambda p: p.label.lower())
    return filtered


def find_port_by_device(
    ports: Iterable[SerialPortInfo],
    device: str,
) -> SerialPortInfo | None:
    for port in ports:
        if port.device == device:
            return port
    return None


def find_port_by_dedupe_key(
    ports: Iterable[SerialPortInfo],
    dedupe_key: str,
) -> SerialPortInfo | None:
    for port in ports:
        if port.dedupe_key == dedupe_key:
            return port
    return None


def _looks_like_usb_uart(port: SerialPortInfo) -> bool:
    haystack = " ".join(
        part for part in (port.description, port.hwid or "", port.device) if part
    ).lower()
    needles = (
        "usb",
        "uart",
        "serial",
        "ch340",
        "cp210",
        "ftdi",
        "pl2303",
        "cdc acm",
        "usbmodem",
        "usbserial",
        "slab",
    )
    return any(n in haystack for n in needles)
