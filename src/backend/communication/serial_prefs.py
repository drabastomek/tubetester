"""Persist last-used serial port selection (optional auto-connect)."""

from __future__ import annotations

import json
from dataclasses import asdict, dataclass
from pathlib import Path

_DEFAULT_PATH = Path(__file__).resolve().parents[3] / "data" / "serial_prefs.json"


@dataclass
class SerialPreferences:
    last_device: str | None = None
    last_dedupe_key: str | None = None
    auto_connect: bool = False


def serial_prefs_path(path: Path | str | None = None) -> Path:
    return Path(path) if path is not None else _DEFAULT_PATH


def load_serial_preferences(path: Path | str | None = None) -> SerialPreferences:
    prefs_path = serial_prefs_path(path)
    if not prefs_path.is_file():
        return SerialPreferences()
    data = json.loads(prefs_path.read_text(encoding="utf-8"))
    return SerialPreferences(
        last_device=data.get("last_device"),
        last_dedupe_key=data.get("last_dedupe_key"),
        auto_connect=bool(data.get("auto_connect", False)),
    )


def save_serial_preferences(
    prefs: SerialPreferences,
    path: Path | str | None = None,
) -> Path:
    prefs_path = serial_prefs_path(path)
    prefs_path.parent.mkdir(parents=True, exist_ok=True)
    prefs_path.write_text(
        json.dumps(asdict(prefs), indent=2) + "\n",
        encoding="utf-8",
    )
    return prefs_path
