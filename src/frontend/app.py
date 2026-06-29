"""Application bootstrap — database, serial service, main window."""

from __future__ import annotations

import sys

from PySide6.QtWidgets import QApplication

from backend.communication.connection import SerialConnectionService
from backend.database.connection import connect, init_db
from backend.database.repository import CatalogRepository
from backend.database.seed import seed_demo_catalog
from frontend.fonts import apply_app_font
from frontend.main_window import MainWindow


def create_app(argv: list[str] | None = None) -> QApplication:
    qt_app = QApplication(argv or sys.argv)
    qt_app.setApplicationName("VTTester")
    qt_app.setOrganizationName("VTTester")
    # Fusion draws combo arrows correctly with app-wide stylesheets (macOS native style does not).
    qt_app.setStyle("Fusion")
    apply_app_font(qt_app, point_size=10)
    return qt_app


def run(argv: list[str] | None = None) -> int:
    init_db()
    conn = connect()
    seed_demo_catalog(conn)
    repo = CatalogRepository(conn)
    serial_service = SerialConnectionService()

    qt_app = create_app(argv)
    window = MainWindow(repo, serial_service)
    window.show()
    window.try_auto_connect()
    code = qt_app.exec()
    conn.close()
    if serial_service.is_connected:
        serial_service.disconnect()
    return code
