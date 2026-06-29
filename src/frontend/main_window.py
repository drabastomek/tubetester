"""Main application window — layout mockup matching the VTTester UI design."""

from __future__ import annotations

from PySide6.QtCore import Qt, QTimer
from PySide6.QtWidgets import QHBoxLayout, QLabel, QMainWindow, QVBoxLayout, QWidget

from backend.communication.connection import SerialConnectionService
from backend.database.repository import CatalogRepository
from frontend.theme import APP_STYLESHEET, MUTED
from frontend.widgets.catalog_panel import CatalogPanel
from frontend.widgets.connection_panel import ConnectionPanel
from frontend.widgets.measured_values_panel import MeasuredValuesPanel
from frontend.widgets.progress_panel import ProgressPanel
from frontend.widgets.sweep_chart import SweepChartView
from frontend.widgets.sweep_panel import SweepPanel
from frontend.widgets.tube_parameters_panel import TubeParametersPanel


class MainWindow(QMainWindow):
    def __init__(
        self,
        repo: CatalogRepository,
        serial_service: SerialConnectionService,
    ) -> None:
        super().__init__()
        self._repo = repo
        self._serial = serial_service
        self._mock_step = 0
        self._mock_timer = QTimer(self)
        self._mock_timer.timeout.connect(self._advance_mock_measurement)

        self.setWindowTitle("Vacuum Tube Tester and Catalog")
        self.setMinimumSize(1280, 900)
        self.setStyleSheet(APP_STYLESHEET)

        central = QWidget()
        self.setCentralWidget(central)
        root = QVBoxLayout(central)
        root.setContentsMargins(14, 14, 14, 14)
        root.setSpacing(12)

        title = QLabel("VACUUM TUBE TESTER")
        title.setStyleSheet(
            f"font-size: 22px; font-weight: 700; letter-spacing: 1px; color: {MUTED};"
        )
        root.addWidget(title)

        self._connection = ConnectionPanel(self._serial)
        root.addWidget(self._connection)

        top = QHBoxLayout()
        top.setSpacing(14)

        self._tube_panel = TubeParametersPanel(self._repo)
        self._tube_panel.setFixedWidth(320)
        top.addWidget(self._tube_panel)

        center = QVBoxLayout()
        center.setSpacing(10)
        self._sweep_panel = SweepPanel()
        center.addWidget(self._sweep_panel)
        self._chart = SweepChartView()
        center.addWidget(self._chart, stretch=1)
        top.addLayout(center, stretch=1)

        right = QVBoxLayout()
        right.setSpacing(10)
        self._progress = ProgressPanel()
        self._measured = MeasuredValuesPanel()
        right.addWidget(self._progress)
        right.addWidget(self._measured)
        right.addStretch()
        right_wrap = QWidget()
        right_wrap.setFixedWidth(380)
        right_wrap.setLayout(right)
        top.addWidget(right_wrap)

        root.addLayout(top, stretch=2)

        self._catalog = CatalogPanel(self._repo)
        root.addWidget(self._catalog, stretch=1)

        self._sweep_panel.start_clicked.connect(self._on_start_clicked)
        self._tube_panel.tube_changed.connect(self._on_tube_changed)

        initial = self._tube_panel.current_tube_name()
        if initial:
            self._on_tube_changed(initial)

    def _on_tube_changed(self, tube_name: str) -> None:
        self._catalog.load_for_tube(tube_name)
        rows = self._repo.list_catalog_for_tube_name(tube_name)
        if rows:
            self._measured.set_condition(rows[0].condition)

    def _on_start_clicked(self, start: bool) -> None:
        if not start:
            self._mock_timer.stop()
            self._sweep_panel.set_running(False)
            self._progress.reset()
            return
        if not self._serial.is_connected:
            self._connection.show_message("Connect to the tester before measuring")
            return
        self._mock_step = 0
        self._sweep_panel.set_running(True)
        self._mock_timer.start(120)

    def _advance_mock_measurement(self) -> None:
        self._mock_step += 1
        overall = min(100, self._mock_step * 4)
        g1 = -2.0 + (self._mock_step * 0.1)
        ua = min(250, self._mock_step * 8)
        self._progress.set_overall(overall)
        self._progress.set_step((self._mock_step * 7) % 100, g1, ua)
        if overall >= 100:
            self._mock_timer.stop()
            self._sweep_panel.set_running(False)
