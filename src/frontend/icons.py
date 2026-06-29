"""Bundled toolbar icons (Material Symbols / Material Icons, Apache-2.0)."""

from __future__ import annotations

from functools import lru_cache
from pathlib import Path

from PySide6.QtCore import Qt, QSize, QRectF
from PySide6.QtGui import QIcon, QPainter, QPixmap
from PySide6.QtSvg import QSvgRenderer

ICONS_DIR = Path(__file__).resolve().parent / "assets" / "icons"
TOOLBAR_ICON_PX = 20
# Extra inset for glyphs that fill the viewBox (keeps optical size consistent).
ICON_INSET: dict[str, float] = {
    "connect": 0.12,
    "disconnect": 0.12,
}
_DEFAULT_INSET = 0.10

# Basename → bundled SVG filename (without path).
ICON_FILES: dict[str, str] = {
    "new_session": "new-session.svg",
    "new_tube": "new-tube.svg",
    "save_measurement": "save-measurement.svg",
    "export_last": "export-last.svg",
    "edit_tube": "edit-tube.svg",
    "duplicate_tube": "duplicate-tube.svg",
    "refresh": "refresh.svg",
    "connect": "connect.svg",
    "disconnect": "disconnect.svg",
    "help": "help.svg",
}


@lru_cache(maxsize=len(ICON_FILES) * 4)
def icon(name: str, *, size: int = TOOLBAR_ICON_PX) -> QIcon:
    """Return a cached ``QIcon`` rendered at a fixed pixel size."""
    filename = ICON_FILES.get(name)
    if filename is None:
        raise KeyError(f"unknown icon: {name!r}")
    path = ICONS_DIR / filename
    if not path.is_file():
        raise FileNotFoundError(f"icon file missing: {path}")

    renderer = QSvgRenderer(str(path))
    if not renderer.isValid():
        raise ValueError(f"invalid SVG icon: {path}")

    inset_ratio = ICON_INSET.get(name, _DEFAULT_INSET)
    inset = size * inset_ratio
    target = QRectF(inset, inset, size - 2 * inset, size - 2 * inset)

    pixmap = QPixmap(QSize(size, size))
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    renderer.render(painter, target)
    painter.end()
    return QIcon(pixmap)
