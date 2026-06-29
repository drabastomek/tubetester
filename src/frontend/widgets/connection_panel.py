"""Serial port selection and connect/disconnect controls."""

from __future__ import annotations

from PySide6.QtCore import QObject, Signal
from PySide6.QtWidgets import (
    QComboBox,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QWidget,
)

from backend.communication.connection import ConnectionState, ConnectionStatus, SerialConnectionService
from backend.communication.ports import SerialPortInfo
from frontend.theme import MUTED, TEAL


class _ConnectionSignals(QObject):
    status_changed = Signal(object)


class ConnectionPanel(QGroupBox):
    def __init__(
        self,
        service: SerialConnectionService,
        parent: QWidget | None = None,
    ) -> None:
        super().__init__("CONNECTION", parent)
        self._service = service
        self._signals = _ConnectionSignals(self)
        self._signals.status_changed.connect(self._apply_status)
        self._service.add_listener(self._signals.status_changed.emit)

        self._status_dot = QLabel("●")
        self._status_dot.setStyleSheet(f"color: {MUTED}; font-size: 16px;")
        self._status_label = QLabel("Disconnected")
        self._status_label.setStyleSheet(f"color: {MUTED};")

        self._port_combo = QComboBox()
        self._port_combo.setSizeAdjustPolicy(
            QComboBox.SizeAdjustPolicy.AdjustToMinimumContentsLengthWithIcon
        )
        self._port_combo.setMinimumContentsLength(28)

        self._refresh_btn = QPushButton("Refresh")
        self._refresh_btn.clicked.connect(self.refresh_ports)

        self._connect_btn = QPushButton("Connect")
        self._connect_btn.clicked.connect(self._toggle_connection)

        row = QHBoxLayout(self)
        row.addWidget(self._status_dot)
        row.addWidget(self._status_label)
        row.addSpacing(12)
        row.addWidget(QLabel("Port"))
        row.addWidget(self._port_combo)
        row.addStretch()
        row.addWidget(self._refresh_btn)
        row.addWidget(self._connect_btn)

        self.refresh_ports()
        self._apply_status(self._service.status)

    def refresh_ports(self) -> None:
        ports = self._service.refresh_ports()
        preferred = self._service.preferred_port(ports)
        current = self._port_combo.currentData()

        self._port_combo.blockSignals(True)
        self._port_combo.clear()
        for port in ports:
            self._port_combo.addItem(port.label, port.device)
        if preferred is not None:
            idx = self._port_combo.findData(preferred.device)
            if idx >= 0:
                self._port_combo.setCurrentIndex(idx)
        elif current is not None:
            idx = self._port_combo.findData(current)
            if idx >= 0:
                self._port_combo.setCurrentIndex(idx)
        self._port_combo.blockSignals(False)

        if self._port_combo.count() == 0:
            self._port_combo.addItem("No serial ports found", None)

    def _toggle_connection(self) -> None:
        if self._service.is_connected:
            self._service.disconnect()
            return
        device = self._port_combo.currentData()
        if not device:
            self._status_label.setText("Select a port")
            return
        self._service.connect_async(device)

    def _apply_status(self, status: ConnectionStatus) -> None:
        if status.state == ConnectionState.CONNECTED:
            self._status_dot.setStyleSheet(f"color: {TEAL}; font-size: 16px;")
            self._status_label.setText(f"Connected — {status.device}")
            self._status_label.setStyleSheet(f"color: {TEAL};")
            self._connect_btn.setText("Disconnect")
            self._connect_btn.setEnabled(True)
            self._port_combo.setEnabled(False)
            self._refresh_btn.setEnabled(False)
            return

        if status.state == ConnectionState.CONNECTING:
            self._status_dot.setStyleSheet("color: #e8923a; font-size: 16px;")
            self._status_label.setText(f"Connecting to {status.device}…")
            self._connect_btn.setText("Connecting…")
            self._connect_btn.setEnabled(False)
            self._port_combo.setEnabled(False)
            self._refresh_btn.setEnabled(False)
            return

        if status.state == ConnectionState.DISCONNECTING:
            self._connect_btn.setText("Disconnecting…")
            self._connect_btn.setEnabled(False)
            self._port_combo.setEnabled(False)
            self._refresh_btn.setEnabled(False)
            return

        self._status_dot.setStyleSheet(f"color: {MUTED}; font-size: 16px;")
        message = status.message or "Disconnected"
        self._status_label.setText(message)
        self._status_label.setStyleSheet(f"color: {MUTED};")
        self._connect_btn.setText("Connect")
        self._connect_btn.setEnabled(True)
        self._port_combo.setEnabled(True)
        self._refresh_btn.setEnabled(True)

    def show_message(self, text: str) -> None:
        self._status_label.setText(text)
