"""Measured values, condition badge, and sweep error metrics."""

from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtGui import QColor
from PySide6.QtWidgets import (
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QTableWidget,
    QTableWidgetItem,
    QVBoxLayout,
    QWidget,
)

from frontend.theme import MUTED, ORANGE_BG, TEAL, TEAL_BG


class MeasuredValuesPanel(QGroupBox):
    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__("MEASURED VALUES", parent)

        self._badge = QLabel("NOS")
        self._badge.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._badge.setStyleSheet(
            f"background: {TEAL}; color: white; font-weight: 700; "
            "padding: 6px 18px; border-radius: 4px; font-size: 14px;"
        )

        badge_row = QHBoxLayout()
        badge_row.addStretch()
        badge_row.addWidget(self._badge)

        self._value_table = QTableWidget(5, 3)
        self._value_table.setHorizontalHeaderLabels(["Parameter", "Value", "% expected"])
        self._value_table.verticalHeader().setVisible(False)
        self._value_table.setEditTriggers(QTableWidget.NoEditTriggers)
        self._value_table.setSelectionMode(QTableWidget.NoSelection)
        self._value_table.setShowGrid(True)
        self._value_table.setFixedHeight(170)
        self._populate_demo_values()

        sweep_title = QLabel("SWEEP RESULTS")
        sweep_title.setStyleSheet(f"color: {MUTED}; font-weight: 600; font-size: 11px; margin-top: 4px;")

        self._sweep_table = QTableWidget(3, 6)
        self._sweep_table.setHorizontalHeaderLabels(["", "Ia", "gm", "μ", "Ra", ""])
        self._sweep_table.setVerticalHeaderLabels(["RMSE", "MAE", "MAPE (%)"])
        self._sweep_table.verticalHeader().setVisible(True)
        self._sweep_table.horizontalHeader().setVisible(True)
        self._sweep_table.setEditTriggers(QTableWidget.NoEditTriggers)
        self._sweep_table.setSelectionMode(QTableWidget.NoSelection)
        self._sweep_table.setFixedHeight(120)
        self._populate_demo_sweep()

        layout = QVBoxLayout(self)
        layout.addLayout(badge_row)
        layout.addWidget(self._value_table)
        layout.addWidget(sweep_title)
        layout.addWidget(self._sweep_table)

    def set_condition(self, label: str) -> None:
        self._badge.setText(label)

    def _populate_demo_values(self) -> None:
        rows = [
            ("Ia", "2.05 mA", 102.5, TEAL_BG),
            ("Ig2", "—", None, None),
            ("gm", "2.02 S", 101.0, TEAL_BG),
            ("μ", "99.8", 99.8, ORANGE_BG),
            ("Ra", "56.2 kΩ", 100.7, TEAL_BG),
        ]
        for row, (param, value, pct, bg) in enumerate(rows):
            self._value_table.setItem(row, 0, QTableWidgetItem(param))
            self._value_table.setItem(row, 1, QTableWidgetItem(value))
            pct_text = "—" if pct is None else f"{pct:.1f}%"
            pct_item = QTableWidgetItem(pct_text)
            if bg:
                pct_item.setBackground(QColor(bg))
            self._value_table.setItem(row, 2, pct_item)
        self._value_table.resizeColumnsToContents()

    def _populate_demo_sweep(self) -> None:
        data = [
            [0.012, 0.018, 0.4, 0.8],
            [0.008, 0.011, 0.3, 0.5],
            [1.2, 1.8, 0.4, 1.1],
        ]
        colors = [
            [TEAL_BG, TEAL_BG, ORANGE_BG, ORANGE_BG],
            [TEAL_BG, TEAL_BG, TEAL_BG, TEAL_BG],
            [TEAL_BG, ORANGE_BG, TEAL_BG, ORANGE_BG],
        ]
        for row, values in enumerate(data):
            self._sweep_table.setItem(row, 0, QTableWidgetItem(""))
            for col, value in enumerate(values, start=1):
                item = QTableWidgetItem(f"{value:g}")
                item.setBackground(QColor(colors[row][col - 1]))
                self._sweep_table.setItem(row, col, item)
        self._sweep_table.resizeColumnsToContents()
