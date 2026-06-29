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


_NA_DESCRIPTIONS = frozenset({"", "n/a", "na"})


def has_usable_port_name(description: str | None) -> bool:
    """True when pyserial reported a human-readable port name (not N/A)."""
    desc = (description or "").strip()
    return bool(desc) and desc.lower() not in _NA_DESCRIPTIONS


def format_port_label(
    *,
    device: str,
    description: str | None = None,
) -> str:
    """Text shown in the port picker — the OS device path (``/dev/cu.*``, ``COMn``, …)."""
    del description  # name is used for filtering only, not display
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


def _is_listable_port(port: SerialPortInfo) -> bool:
    """Ports shown in the UI: named devices; on macOS, ``/dev/cu.*`` only."""
    if not has_usable_port_name(port.description):
        return False
    if sys.platform == "darwin":
        return port.device.startswith("/dev/cu.")
    return True


def list_serial_ports(*, include_all: bool = True) -> list[SerialPortInfo]:
    """
    Return serial ports available on this machine, sorted by label.

    Skips unnamed / ``N/A`` entries. On macOS, only ``/dev/cu.*`` call-out devices
    are returned. The combo shows each port's device path.
    """
    if comports is None:
        raise RuntimeError("pyserial is not installed")

    raw = [_info_from_list_port(p) for p in comports()]
    filtered = [p for p in raw if _is_listable_port(p)]
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
