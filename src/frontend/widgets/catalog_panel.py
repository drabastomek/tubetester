"""TUBE CATALOG table backed by the catalog database."""

from __future__ import annotations

from PySide6.QtWidgets import QGroupBox, QTableWidget, QTableWidgetItem, QVBoxLayout, QWidget

from backend.database.repository import CatalogRepository


class CatalogPanel(QGroupBox):
    def __init__(self, repo: CatalogRepository, parent: QWidget | None = None) -> None:
        super().__init__("TUBE CATALOG", parent)
        self._repo = repo

        self._table = QTableWidget(0, 5)
        self._table.setHorizontalHeaderLabels(
            ["ID", "MEASUREMENT DATE", "TUBE MANUFACTURER", "CONDITION", "POINT / SWEEP"]
        )
        self._table.verticalHeader().setVisible(False)
        self._table.setAlternatingRowColors(True)
        self._table.setEditTriggers(QTableWidget.NoEditTriggers)
        self._table.setSelectionBehavior(QTableWidget.SelectRows)
        self._table.setSelectionMode(QTableWidget.SingleSelection)

        layout = QVBoxLayout(self)
        layout.addWidget(self._table)

    def load_for_tube(self, tube_name: str) -> None:
        rows = self._repo.list_catalog_for_tube_name(tube_name)
        self._table.setRowCount(len(rows))
        for row_idx, row in enumerate(rows):
            values = [
                str(row.group_id),
                row.measured_at,
                row.manufacturer or "—",
                row.condition,
                row.kind,
            ]
            for col, value in enumerate(values):
                self._table.setItem(row_idx, col, QTableWidgetItem(value))
        self._table.resizeColumnsToContents()
