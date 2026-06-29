"""Main application window — layout mockup matching the VTTester UI design."""

from __future__ import annotations

from PySide6.QtCore import Qt, QTimer, QSize
from PySide6.QtGui import QAction, QKeySequence
from PySide6.QtWidgets import (
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QMessageBox,
    QSizePolicy,
    QToolBar,
    QVBoxLayout,
    QWidget,
)

from backend.communication.connection import SerialConnectionService
from backend.database.repository import CatalogRepository
from frontend.dialogs.about_dialog import AboutDialog
from frontend.dialogs.preferences_dialog import PreferencesDialog
from frontend.icons import icon
from frontend.serial_connection import SerialConnectionController, SerialConnectionToolbar
from frontend.theme import APP_STYLESHEET, MUTED
from frontend.widgets.catalog_panel import CatalogPanel
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
        self._serial_ui = SerialConnectionController(serial_service, self)
        self._mock_step = 0
        self._mock_timer = QTimer(self)
        self._mock_timer.timeout.connect(self._advance_mock_measurement)

        self.setWindowTitle("Vacuum Tube Tester and Catalog")
        self.setMinimumSize(1280, 900)
        self.setStyleSheet(APP_STYLESHEET)

        self._build_central_layout()
        self._build_actions()
        self._build_menu_bar()
        self._build_toolbar()
        self._wire_panel_signals()
        self._update_measure_actions()

        initial = self._tube_panel.current_tube_name()
        if initial:
            self._on_tube_changed(initial)

    def _build_central_layout(self) -> None:
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

    def _build_actions(self) -> None:
        self._action_new_session = QAction(icon("new_session"), "New Session", self)
        self._action_new_session.triggered.connect(self._placeholder_new_session)

        self._action_new_tube = QAction(icon("new_tube"), "New Tube", self)
        self._action_new_tube.triggered.connect(self._placeholder_new_tube)

        self._action_save_measurement = QAction(icon("save_measurement"), "Save Measurement", self)
        self._action_save_measurement.triggered.connect(self._placeholder_save_measurement)

        self._action_export_last = QAction(icon("export_last"), "Export Last Result", self)
        self._action_export_last.triggered.connect(self._placeholder_export_last)

        self._action_edit_tube = QAction(icon("edit_tube"), "Edit Tube Parameters", self)
        self._action_edit_tube.triggered.connect(self._placeholder_edit_tube)

        self._action_help = QAction(icon("help"), "Help", self)
        self._action_help.triggered.connect(self._show_about)

        self._action_preferences = QAction("Preferences…", self)
        self._action_preferences.triggered.connect(self._show_preferences)

        self._action_quit = QAction("Quit", self)
        self._action_quit.setShortcut(QKeySequence.StandardKey.Quit)
        self._action_quit.triggered.connect(self.close)

        self._action_start = QAction("Start Measurement", self)
        self._action_start.setShortcut("F5")
        self._action_start.triggered.connect(self.start_measurement)

        self._action_stop = QAction("Stop Measurement", self)
        self._action_stop.setShortcut("Esc")
        self._action_stop.triggered.connect(self.stop_measurement)

    def _build_menu_bar(self) -> None:
        menu_bar = self.menuBar()

        file_menu = menu_bar.addMenu("&File")
        file_menu.addAction(self._action_new_session)
        file_menu.addAction(self._action_new_tube)
        file_menu.addAction(self._action_save_measurement)
        file_menu.addAction(self._action_export_last)
        file_menu.addSeparator()
        file_menu.addAction(self._action_preferences)
        file_menu.addSeparator()
        file_menu.addAction(self._action_quit)

        edit_menu = menu_bar.addMenu("&Edit")
        edit_menu.addAction(self._action_edit_tube)

        measure_menu = menu_bar.addMenu("&Measure")
        measure_menu.addAction(self._action_start)
        measure_menu.addAction(self._action_stop)

        help_menu = menu_bar.addMenu("&Help")
        help_menu.addAction(self._action_help)

    def _build_toolbar(self) -> None:
        toolbar = QToolBar("Main", self)
        toolbar.setMovable(False)
        toolbar.setToolButtonStyle(Qt.ToolButtonStyle.ToolButtonTextBesideIcon)
        toolbar.setIconSize(QSize(20, 20))
        self.addToolBar(toolbar)

        toolbar.addAction(self._action_new_session)
        toolbar.addAction(self._action_new_tube)
        toolbar.addAction(self._action_save_measurement)
        toolbar.addAction(self._action_export_last)
        toolbar.addSeparator()
        toolbar.addAction(self._action_edit_tube)
        toolbar.addSeparator()

        self._serial_toolbar = SerialConnectionToolbar()
        toolbar.addWidget(self._serial_toolbar)

        toolbar_spacer = QWidget()
        toolbar_spacer.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Preferred)
        toolbar.addWidget(toolbar_spacer)

        toolbar.addSeparator()
        toolbar.addAction(self._action_help)

        self._serial_ui.bind(self._serial_toolbar)

    def _wire_panel_signals(self) -> None:
        self._sweep_panel.start_clicked.connect(self._on_start_clicked)
        self._tube_panel.tube_changed.connect(self._on_tube_changed)

    def _update_measure_actions(self) -> None:
        running = self._sweep_panel.is_running
        self._action_start.setEnabled(not running)
        self._action_stop.setEnabled(running)

    def _on_tube_changed(self, tube_name: str) -> None:
        self._catalog.load_for_tube(tube_name)
        rows = self._repo.list_catalog_for_tube_name(tube_name)
        if rows:
            self._measured.set_condition(rows[0].condition)

    def start_measurement(self) -> None:
        self._on_start_clicked(True)

    def stop_measurement(self) -> None:
        self._on_start_clicked(False)

    def _on_start_clicked(self, start: bool) -> None:
        if not start:
            self._mock_timer.stop()
            self._sweep_panel.set_running(False)
            self._progress.reset()
            self._update_measure_actions()
            return
        if not self._serial.is_connected:
            QMessageBox.warning(self, "Not Connected", "Connect to the tester before measuring.")
            return
        self._mock_step = 0
        self._sweep_panel.set_running(True)
        self._mock_timer.start(120)
        self._update_measure_actions()

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
            self._update_measure_actions()

    def try_auto_connect(self) -> None:
        self._serial_ui.try_auto_connect()

    def _show_preferences(self) -> None:
        PreferencesDialog(self._serial, self).exec()

    def _show_about(self) -> None:
        AboutDialog(self).exec()

    def _placeholder(self, title: str, message: str) -> None:
        QMessageBox.information(self, title, message)

    def _placeholder_new_session(self) -> None:
        self._placeholder("New Session", "Not implemented yet.")

    def _placeholder_new_tube(self) -> None:
        self._placeholder("New Tube", "Not implemented yet.")

    def _placeholder_save_measurement(self) -> None:
        self._placeholder("Save Measurement", "Not implemented yet.")

    def _placeholder_export_last(self) -> None:
        self._placeholder("Export Last Result", "Not implemented yet.")

    def _placeholder_edit_tube(self) -> None:
        tube = self._tube_panel.current_tube_name() or "(none)"
        self._placeholder(
            "Edit Tube Parameters",
            f"Tube type editor for “{tube}” is not implemented yet.",
        )
