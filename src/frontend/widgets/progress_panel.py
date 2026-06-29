"""Overall and step progress bars."""

from __future__ import annotations

from PySide6.QtWidgets import QGroupBox, QLabel, QProgressBar, QVBoxLayout, QWidget

from frontend.theme import MUTED


class ProgressPanel(QGroupBox):
    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__("PROGRESS", parent)
        self._overall = QProgressBar()
        self._overall.setRange(0, 100)
        self._overall.setValue(26)
        self._overall.setFormat("OVERALL %p%")

        self._step_label = QLabel("G1 = -2.5V          Ua = 180V")
        self._step_label.setStyleSheet(f"color: {MUTED}; font-size: 12px;")

        self._step = QProgressBar()
        self._step.setRange(0, 100)
        self._step.setValue(42)
        self._step.setTextVisible(False)

        layout = QVBoxLayout(self)
        layout.addWidget(self._overall)
        layout.addWidget(self._step_label)
        layout.addWidget(self._step)

    def set_overall(self, value: int) -> None:
        self._overall.setValue(max(0, min(100, value)))

    def set_step(self, value: int, g1: float, ua: float) -> None:
        self._step.setValue(max(0, min(100, value)))
        self._step_label.setText(f"G1 = {g1:g}V          Ua = {ua:g}V")

    def reset(self) -> None:
        self.set_overall(0)
        self.set_step(0, -2.0, 0)
