"""Tube selection and measurement parameter display."""

from __future__ import annotations

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (
    QComboBox,
    QFormLayout,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QRadioButton,
    QVBoxLayout,
    QWidget,
)

from backend.database.repository import CatalogRepository, TubeTypeSystem
from frontend.theme import MUTED, TEAL


class TubeParametersPanel(QWidget):
    tube_changed = Signal(str)

    def __init__(self, repo: CatalogRepository, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._repo = repo
        self._systems: list[TubeTypeSystem] = []

        selection_box = QGroupBox("TUBE SELECTION")
        selection_row = QHBoxLayout(selection_box)
        self._tube_combo = QComboBox()
        self._tube_combo.currentIndexChanged.connect(self._on_tube_changed)
        self._edit_btn = QPushButton("EDIT")
        self._edit_btn.setEnabled(False)
        selection_row.addWidget(self._tube_combo, stretch=1)
        selection_row.addWidget(self._edit_btn)

        params_box = QGroupBox("MEASUREMENT PARAMETERS")
        params_layout = QVBoxLayout(params_box)

        setpoints = QFormLayout()
        setpoints.setLabelAlignment(setpoints.labelAlignment())
        self._ua = self._ro_field("250")
        self._ug1 = self._ro_field("-1.5")
        self._ug2 = self._ro_field("—")
        setpoints.addRow("Anode Voltage (Va)", self._with_unit(self._ua, "V"))
        setpoints.addRow("Grid Voltage (Vg1)", self._with_unit(self._ug1, "V"))
        setpoints.addRow("Screen Voltage (Vg2)", self._with_unit(self._ug2, "V"))
        params_layout.addLayout(setpoints)

        expected_title = QLabel("EXPECTED VALUES")
        expected_title.setStyleSheet(f"color: {MUTED}; font-weight: 600; font-size: 11px; margin-top: 8px;")
        params_layout.addWidget(expected_title)

        expected_grid = QGridLayout()
        expected_grid.setHorizontalSpacing(16)
        self._exp_ia = QLabel("—")
        self._exp_ig2 = QLabel("—")
        self._exp_gm = QLabel("—")
        self._exp_mu = QLabel("—")
        self._exp_ra = QLabel("—")
        self._add_expected_row(expected_grid, 0, "Anode Current (Ia)", self._exp_ia, "mA")
        self._add_expected_row(expected_grid, 1, "Screen Current (Ig2)", self._exp_ig2, "mA")
        self._add_expected_row(expected_grid, 2, "Transconductance (gm)", self._exp_gm, "S")
        self._add_expected_row(expected_grid, 3, "Amplification (μ)", self._exp_mu, "")
        self._add_expected_row(expected_grid, 4, "Anode Resistance (Ra)", self._exp_ra, "kΩ")
        params_layout.addLayout(expected_grid)

        system_box = QGroupBox("TUBE SYSTEM")
        system_row = QHBoxLayout(system_box)
        self._sys1 = QRadioButton("SYSTEM 1")
        self._sys2 = QRadioButton("SYSTEM 2")
        self._sys1.setChecked(True)
        self._sys1.toggled.connect(self._on_system_changed)
        system_row.addWidget(self._sys1)
        system_row.addWidget(self._sys2)
        system_row.addStretch()

        socket_row = QHBoxLayout()
        socket_col = QVBoxLayout()
        socket_label = QLabel("SOCKET")
        socket_label.setStyleSheet(f"color: {MUTED}; font-size: 11px; font-weight: 600;")
        self._socket_letter = QLabel("F")
        self._socket_letter.setFixedSize(72, 72)
        self._socket_letter.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._socket_letter.setStyleSheet(
            f"border: 2px solid {MUTED}; border-radius: 36px; font-size: 28px; font-weight: 700;"
        )
        socket_col.addWidget(socket_label, alignment=Qt.AlignmentFlag.AlignHCenter)
        socket_col.addWidget(self._socket_letter, alignment=Qt.AlignmentFlag.AlignHCenter)
        tube_hint = QLabel("12-pin compact socket")
        tube_hint.setStyleSheet(f"color: {TEAL}; font-size: 11px;")
        tube_hint.setOpenExternalLinks(True)
        tube_hint.setText('<a href="#">View datasheet</a>')
        socket_col.addWidget(tube_hint)
        socket_row.addLayout(socket_col)
        socket_row.addStretch()

        root = QVBoxLayout(self)
        root.setContentsMargins(0, 0, 0, 0)
        root.addWidget(selection_box)
        root.addWidget(params_box)
        root.addWidget(system_box)
        root.addLayout(socket_row)
        root.addStretch()

        self._load_tubes()

    def current_tube_name(self) -> str | None:
        return self._tube_combo.currentData()

    def _load_tubes(self) -> None:
        self._tube_combo.blockSignals(True)
        self._tube_combo.clear()
        for tube in self._repo.list_tube_types():
            self._tube_combo.addItem(tube.name, tube.name)
        self._tube_combo.blockSignals(False)
        if self._tube_combo.count():
            self._on_tube_changed()

    def _on_tube_changed(self) -> None:
        name = self.current_tube_name()
        if not name:
            return
        tube = self._repo.get_tube_type_by_name(name)
        if tube is None:
            return
        self._systems = self._repo.list_tube_type_systems(tube.id)
        self._socket_letter.setText(tube.socket)
        self._apply_system(self._current_system())
        self.tube_changed.emit(name)

    def _on_system_changed(self) -> None:
        self._apply_system(self._current_system())

    def _current_system(self) -> TubeTypeSystem | None:
        target = 1 if self._sys1.isChecked() else 2
        for system in self._systems:
            if system.system_number == target:
                return system
        return self._systems[0] if self._systems else None

    def _apply_system(self, system: TubeTypeSystem | None) -> None:
        if system is None:
            return
        self._ua.setText(f"{system.ua_v:g}")
        self._ug1.setText(f"{system.ug1_v:g}")
        self._ug2.setText("—" if system.ug2_v is None else f"{system.ug2_v:g}")
        self._exp_ia.setText(self._fmt(system.ia_expected_ma))
        self._exp_ig2.setText("—" if system.ig2_expected_ma is None else self._fmt(system.ig2_expected_ma))
        self._exp_gm.setText(self._fmt(system.transconductance_s_expected))
        self._exp_mu.setText(self._fmt(system.gain_k_expected))
        ra = system.anode_resistance_r_expected
        self._exp_ra.setText("—" if ra is None else f"{ra:g}")

    @staticmethod
    def _fmt(value: float | None) -> str:
        return "—" if value is None else f"{value:g}"

    @staticmethod
    def _ro_field(text: str) -> QLabel:
        label = QLabel(text)
        label.setStyleSheet("padding: 4px 6px; border: 1px solid #cccccc; border-radius: 3px;")
        return label

    @staticmethod
    def _with_unit(field: QLabel, unit: str) -> QWidget:
        wrap = QWidget()
        row = QHBoxLayout(wrap)
        row.setContentsMargins(0, 0, 0, 0)
        row.addWidget(field)
        if unit:
            row.addWidget(QLabel(unit))
        row.addStretch()
        return wrap

    @staticmethod
    def _add_expected_row(grid: QGridLayout, row: int, label: str, value: QLabel, unit: str) -> None:
        name = QLabel(label)
        name.setStyleSheet(f"color: {MUTED};")
        grid.addWidget(name, row, 0)
        grid.addWidget(value, row, 1)
        if unit:
            grid.addWidget(QLabel(unit), row, 2)
