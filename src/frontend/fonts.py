"""Load bundled Fira Sans fonts (cross-platform, not system-dependent)."""

from __future__ import annotations

from pathlib import Path

from PySide6.QtGui import QFont, QFontDatabase
from PySide6.QtWidgets import QApplication

FONTS_DIR = Path(__file__).resolve().parent / "assets" / "fonts"
FONT_FAMILY = "Fira Sans"
FONT_FILES: tuple[str, ...] = (
    "FiraSans-Regular.ttf",
    "FiraSans-SemiBold.ttf",
    "FiraSans-Bold.ttf",
)
DEFAULT_POINT_SIZE = 10

_loaded = False


def load_bundled_fonts() -> str:
    """Register bundled TTF files with Qt. Safe to call more than once."""
    global _loaded
    if _loaded:
        return FONT_FAMILY

    if not FONTS_DIR.is_dir():
        raise FileNotFoundError(f"Font directory not found: {FONTS_DIR}")

    font_ids: list[int] = []
    for filename in FONT_FILES:
        path = FONTS_DIR / filename
        if not path.is_file():
            raise FileNotFoundError(f"Bundled font missing: {path}")
        font_id = QFontDatabase.addApplicationFont(str(path))
        if font_id < 0:
            raise RuntimeError(f"Qt failed to load font: {path}")
        font_ids.append(font_id)

    families = {
        family
        for font_id in font_ids
        for family in QFontDatabase.applicationFontFamilies(font_id)
    }
    if FONT_FAMILY not in families:
        raise RuntimeError(
            f"Expected bundled family {FONT_FAMILY!r}, Qt registered: {sorted(families)!r}"
        )

    _loaded = True
    return FONT_FAMILY


def apply_app_font(app: QApplication, *, point_size: int = DEFAULT_POINT_SIZE) -> QFont:
    """Set the application-wide default font from bundled Fira Sans."""
    load_bundled_fonts()
    font = QFont(FONT_FAMILY)
    font.setPointSize(point_size)
    font.setStyleStrategy(QFont.StyleStrategy.PreferAntialias)
    app.setFont(font)
    return font
