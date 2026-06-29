"""About box."""

from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QDialog, QLabel, QPushButton, QVBoxLayout

from frontend.theme import MUTED


class AboutDialog(QDialog):
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setWindowTitle("About VTTester")

        title = QLabel("VTTester")
        title.setStyleSheet("font-size: 18px; font-weight: 700;")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)

        # TODO: make the protocol version dynamic, automatically fetched from the firmware

        body = QLabel(
            "Vacuum tube tester and measurement catalog.\n"
            "Desktop protocol v0.5 · PySide6 UI mockup"
        )
        body.setAlignment(Qt.AlignmentFlag.AlignCenter)
        body.setStyleSheet(f"color: {MUTED};")

        close_btn = QPushButton("Close")
        close_btn.clicked.connect(self.accept)

        layout = QVBoxLayout(self)
        layout.addWidget(title)
        layout.addWidget(body)
        layout.addWidget(close_btn, alignment=Qt.AlignmentFlag.AlignCenter)
