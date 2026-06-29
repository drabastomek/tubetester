"""Serial port UI logic — bound once from the main toolbar (no duplicate panel)."""

from __future__ import annotations

from PySide6.QtCore import QObject, Qt, QSize, Signal
from PySide6.QtGui import QFontMetrics
from PySide6.QtWidgets import (
    QComboBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QSizePolicy,
    QWidget,
)

from backend.communication.connection import ConnectionState, ConnectionStatus, SerialConnectionService
from backend.communication.ports import SerialPortInfo
from frontend.icons import TOOLBAR_ICON_PX, icon
from frontend.theme import MUTED, TEAL

_ICON_BTN_SIZE = QSize(34, 30)


class SerialConnectionToolbar(QWidget):
    """Port + connect controls as one toolbar block (prevents QToolBar overflow splitting)."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(6)

        self.port_combo = QComboBox()
        self.port_combo.setSizeAdjustPolicy(
            QComboBox.SizeAdjustPolicy.AdjustToMinimumContentsLengthWithIcon
        )
        self.port_combo.setMinimumContentsLength(14)
        self.port_combo.setMinimumWidth(150)
        self.port_combo.setMaximumWidth(200)

        self.refresh_btn = self._icon_button("refresh", "Refresh serial ports")
        self.connect_btn = self._icon_button("connect", "Connect to selected port")

        self.status_label = QLabel("● Disconnected")
        self.status_label.setMinimumWidth(132)
        self.status_label.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Preferred)
        self.status_label.setStyleSheet(f"color: {MUTED}; padding: 0 4px;")
        self._status_full_text = "● Disconnected"

        layout.addWidget(self.port_combo)
        layout.addWidget(self.refresh_btn)
        layout.addWidget(self.connect_btn)
        layout.addWidget(self.status_label)

    @staticmethod
    def _icon_button(icon_name: str, tooltip: str) -> QPushButton:
        btn = QPushButton()
        btn.setIcon(icon(icon_name))
        btn.setIconSize(QSize(TOOLBAR_ICON_PX, TOOLBAR_ICON_PX))
        btn.setToolTip(tooltip)
        btn.setFixedSize(_ICON_BTN_SIZE)
        return btn

    def set_connect_icon(self, name: str, *, tooltip: str) -> None:
        self.connect_btn.setIcon(icon(name))
        self.connect_btn.setIconSize(QSize(TOOLBAR_ICON_PX, TOOLBAR_ICON_PX))
        self.connect_btn.setToolTip(tooltip)

    def set_status_text(self, text: str, *, connected: bool = False) -> None:
        self._status_full_text = text
        color = TEAL if connected else MUTED
        self.status_label.setToolTip(text)
        self.status_label.setStyleSheet(f"color: {color}; padding: 0 4px;")
        self._elide_status_text()

    def resizeEvent(self, event) -> None:  # noqa: ANN001, N802
        super().resizeEvent(event)
        self._elide_status_text()

    def _elide_status_text(self) -> None:
        available = max(self.status_label.width() - 8, 80)
        elided = QFontMetrics(self.status_label.font()).elidedText(
            self._status_full_text,
            Qt.TextElideMode.ElideRight,
            available,
        )
        self.status_label.setText(elided)


class SerialConnectionController(QObject):
    """Owns port list + connect/disconnect; drives toolbar widgets via ``bind``."""

    status_changed = Signal(object)

    def __init__(self, service: SerialConnectionService, parent: QObject | None = None) -> None:
        super().__init__(parent)
        self._service = service
        self._toolbar: SerialConnectionToolbar | None = None
        self._connect_btn_icon: str | None = None
        self._service.add_listener(self._apply_status)

    def bind(self, toolbar: SerialConnectionToolbar) -> None:
        self._toolbar = toolbar
        toolbar.refresh_btn.clicked.connect(self.refresh_ports)
        toolbar.connect_btn.clicked.connect(self.toggle_connection)
        self.refresh_ports()
        self._apply_status(self._service.status)

    def refresh_ports(self) -> None:
        if self._toolbar is None:
            return
        ports = self._service.refresh_ports()
        preferred = self._service.preferred_port(ports)
        current = self._toolbar.port_combo.currentData()
        self._populate_combo(self._toolbar.port_combo, ports, preferred, current)

    def toggle_connection(self) -> None:
        if self._service.is_connected:
            self.disconnect()
        else:
            self.connect_selected()

    def connect_selected(self) -> None:
        if self._toolbar is None:
            return
        device = self._toolbar.port_combo.currentData()
        if not device:
            self._toolbar.set_status_text("Select a port")
            return
        self._service.connect_async(device)

    def disconnect(self) -> None:
        self._service.disconnect()

    def try_auto_connect(self) -> None:
        prefs = self._service.load_preferences()
        if not prefs.auto_connect:
            return
        if self._toolbar is None:
            return
        self.refresh_ports()
        preferred = self._service.preferred_port(self._service.ports)
        if preferred is None:
            return
        index = self._toolbar.port_combo.findData(preferred.device)
        if index >= 0:
            self._toolbar.port_combo.setCurrentIndex(index)
        self.connect_selected()

    @staticmethod
    def _populate_combo(
        combo: QComboBox,
        ports: list[SerialPortInfo],
        preferred: SerialPortInfo | None,
        current: str | None,
    ) -> None:
        combo.blockSignals(True)
        combo.clear()
        for port in ports:
            combo.addItem(port.label, port.device)
        if preferred is not None:
            idx = combo.findData(preferred.device)
            if idx >= 0:
                combo.setCurrentIndex(idx)
        elif current is not None:
            idx = combo.findData(current)
            if idx >= 0:
                combo.setCurrentIndex(idx)
        if combo.count() == 0:
            combo.addItem("No serial ports found", None)
        combo.blockSignals(False)

    def _apply_status(self, status: ConnectionStatus) -> None:
        if self._toolbar is None:
            return

        toolbar = self._toolbar

        if status.state == ConnectionState.CONNECTED:
            want_icon = "disconnect"
            tooltip = "Disconnect"
            enabled = True
        elif status.state == ConnectionState.CONNECTING:
            want_icon = "connect"
            tooltip = "Connecting…"
            enabled = False
        elif status.state == ConnectionState.DISCONNECTING:
            want_icon = "disconnect"
            tooltip = "Disconnecting…"
            enabled = False
        else:
            want_icon = "connect"
            tooltip = "Connect to selected port"
            enabled = True

        if self._connect_btn_icon != want_icon:
            toolbar.set_connect_icon(want_icon, tooltip=tooltip)
            self._connect_btn_icon = want_icon
        else:
            toolbar.connect_btn.setToolTip(tooltip)
        toolbar.connect_btn.setEnabled(enabled)

        busy = status.state in (
            ConnectionState.CONNECTED,
            ConnectionState.CONNECTING,
            ConnectionState.DISCONNECTING,
        )
        toolbar.refresh_btn.setEnabled(not busy)
        toolbar.port_combo.setEnabled(not busy)

        if status.state == ConnectionState.CONNECTED:
            toolbar.set_status_text("● Connected", connected=True)
            toolbar.status_label.setToolTip(f"Connected — {status.device}")
        elif status.state == ConnectionState.CONNECTING:
            toolbar.set_status_text("● Connecting…")
            toolbar.status_label.setToolTip(f"Connecting — {status.device}")
        elif status.state == ConnectionState.DISCONNECTING:
            toolbar.set_status_text("● Disconnecting…")
        else:
            message = status.message or "Disconnected"
            toolbar.set_status_text(f"● {message}")

        self.status_changed.emit(status)
