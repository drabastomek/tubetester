"""Shared colors and stylesheet fragments for the VTTester UI mockup."""

from frontend.fonts import FONT_FAMILY

TEAL = "#2aa8a8"
TEAL_BG = "#d4f0f0"
ORANGE = "#e8923a"
ORANGE_BG = "#fdebd0"
GREY_BORDER = "#cccccc"
GREY_HEADER = "#e8e8e8"
TEXT = "#222222"
MUTED = "#666666"
START_BG = "#1a1a1a"
START_HOVER = "#333333"

APP_STYLESHEET = f"""
QMainWindow, QWidget {{
    background: #ffffff;
    color: {TEXT};
    font-family: "{FONT_FAMILY}";
    font-size: 14px;
}}
QGroupBox {{
    font-weight: 600;
    font-size: 11px;
    letter-spacing: 0.5px;
    border: 1px solid {GREY_BORDER};
    border-radius: 4px;
    margin-top: 10px;
    padding: 18px 10px 10px 10px;
}}
QGroupBox::title {{
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 6px;
    color: {MUTED};
}}
QLineEdit, QSpinBox, QDoubleSpinBox {{
    border: 1px solid {GREY_BORDER};
    border-radius: 3px;
    padding: 4px 6px;
    background: #ffffff;
    min-height: 22px;
}}
QComboBox {{
    border: 1px solid {GREY_BORDER};
    border-radius: 3px;
    padding: 4px 6px;
    background: #ffffff;
    min-height: 22px;
}}
QComboBox:disabled {{
    color: #999999;
    background: #f5f5f5;
}}
QPushButton {{
    border: 1px solid {GREY_BORDER};
    border-radius: 4px;
    padding: 6px 14px;
    background: #f5f5f5;
}}
QPushButton:hover {{
    background: #ececec;
}}
QPushButton:disabled {{
    color: #999999;
    background: #f0f0f0;
}}
QProgressBar {{
    border: 1px solid {GREY_BORDER};
    border-radius: 3px;
    text-align: center;
    height: 18px;
}}
QProgressBar::chunk {{
    background: {TEAL};
    border-radius: 2px;
}}
QTableWidget {{
    border: 1px solid {GREY_BORDER};
    gridline-color: {GREY_BORDER};
    selection-background-color: {TEAL_BG};
    selection-color: {TEXT};
}}
QHeaderView::section {{
    background: {GREY_HEADER};
    border: none;
    border-right: 1px solid {GREY_BORDER};
    border-bottom: 1px solid {GREY_BORDER};
    padding: 6px 8px;
    font-weight: 600;
    font-size: 11px;
}}
QRadioButton {{
    spacing: 6px;
}}
"""
