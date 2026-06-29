"""Application preferences (MVP — serial auto-connect)."""

from __future__ import annotations

from PySide6.QtWidgets import (
    QCheckBox,
    QDialog,
    QDialogButtonBox,
    QFormLayout,
    QVBoxLayout,
)

from backend.communication.connection import SerialConnectionService
from backend.communication.serial_prefs import SerialPreferences


class PreferencesDialog(QDialog):
    def __init__(
        self,
        serial_service: SerialConnectionService,
        parent=None,
    ) -> None:
        super().__init__(parent)
        self._serial = serial_service
        self.setWindowTitle("Preferences")

        prefs = self._serial.load_preferences()
        self._auto_connect = QCheckBox("Connect to last used port on startup")
        self._auto_connect.setChecked(prefs.auto_connect)

        form = QFormLayout()
        form.addRow("Serial", self._auto_connect)

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(self._save_and_accept)
        buttons.rejected.connect(self.reject)

        layout = QVBoxLayout(self)
        layout.addLayout(form)
        layout.addWidget(buttons)

    def _save_and_accept(self) -> None:
        prefs = self._serial.load_preferences()
        prefs.auto_connect = self._auto_connect.isChecked()
        self._serial.save_preferences(prefs)
        self.accept()
