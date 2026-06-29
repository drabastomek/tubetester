"""Sweep grid controls and START button (mock measurement trigger)."""

from __future__ import annotations

from PySide6.QtCore import Signal, Qt
from PySide6.QtWidgets import (
    QCheckBox,
    QDoubleSpinBox,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from frontend.theme import START_BG, START_HOVER


class SweepPanel(QWidget):
    start_clicked = Signal(bool)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._running = False

        sweep_box = QGroupBox("MEASURE SWEEP")
        grid = QGridLayout(sweep_box)
        grid.addWidget(QLabel(""), 0, 0)
        grid.addWidget(self._header("MIN"), 0, 1)
        grid.addWidget(self._header("MAX"), 0, 2)
        grid.addWidget(self._header("STEP"), 0, 3)

        self._sweep_enabled = QCheckBox("Enable sweep mode")
        grid.addWidget(self._sweep_enabled, 1, 0, 1, 4)

        self._fields = {
            "Va": self._row(grid, 2, "Va (V)", 0, 250, 10),
            "Vg1": self._row(grid, 3, "Vg1 (V)", -2, 0, 0.5),
            "Vg2": self._row(grid, 4, "Vg2 (V)", 0, 250, 10),
        }
        self._fields["Vg2"][0].setEnabled(False)
        for spin in self._fields["Vg2"][1:]:
            spin.setEnabled(False)

        self._start_btn = QPushButton("  ▶   START")
        self._start_btn.setFixedHeight(72)
        self._start_btn.setStyleSheet(
            f"""
            QPushButton {{
                background: {START_BG};
                color: white;
                font-size: 22px;
                font-weight: 700;
                border: none;
                border-radius: 6px;
                padding: 12px 28px;
            }}
            QPushButton:hover {{
                background: {START_HOVER};
            }}
            QPushButton:disabled {{
                background: #777777;
            }}
            """
        )
        self._start_btn.clicked.connect(self._on_start)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(sweep_box)
        layout.addStretch()
        layout.addWidget(self._start_btn, alignment=Qt.AlignmentFlag.AlignHCenter)

    def is_sweep_enabled(self) -> bool:
        return self._sweep_enabled.isChecked()

    def set_running(self, running: bool) -> None:
        self._running = running
        self._start_btn.setEnabled(not running)
        self._start_btn.setText("  ■   STOP" if running else "  ▶   START")

    def _on_start(self) -> None:
        self.start_clicked.emit(not self._running)

    @staticmethod
    def _header(text: str) -> QLabel:
        label = QLabel(text)
        label.setStyleSheet("font-weight: 600; color: #666666;")
        return label

    @staticmethod
    def _row(
        grid: QGridLayout,
        row: int,
        label: str,
        min_v: float,
        max_v: float,
        step: float,
    ) -> tuple[QLabel, QDoubleSpinBox, QDoubleSpinBox, QDoubleSpinBox]:
        grid.addWidget(QLabel(label), row, 0)
        minimum = QDoubleSpinBox()
        maximum = QDoubleSpinBox()
        step_spin = QDoubleSpinBox()
        for spin, value in ((minimum, min_v), (maximum, max_v), (step_spin, step)):
            spin.setDecimals(2)
            spin.setRange(-9999, 9999)
            spin.setValue(value)
            spin.setFixedWidth(88)
        grid.addWidget(minimum, row, 1)
        grid.addWidget(maximum, row, 2)
        grid.addWidget(step_spin, row, 3)
        return (QLabel(label), minimum, maximum, step_spin)
