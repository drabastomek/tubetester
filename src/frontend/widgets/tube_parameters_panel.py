"""Tube selection and measurement parameter display."""

from __future__ import annotations

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (
    QComboBox,
    QFrame,
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
from frontend.theme import MUTED, TEAL, TEAL_BG, ORANGE, ORANGE_BG

# Tester front-panel socket positions (photo layout): large A/D/I; B/C, E/F, G/H stacked.
_SOCKET_GRID: dict[str, tuple[int, int, bool]] = {
    "A": (1, 0, True),
    "B": (0, 1, False),
    "C": (2, 1, False),
    "D": (1, 2, True),
    "E": (0, 3, False),
    "F": (2, 3, False),
    "G": (0, 4, False),
    "H": (2, 4, False),
    "I": (1, 5, True),
}
_SOCKET_LARGE_PX = 52
_SOCKET_SMALL_PX = 36


class SocketDiagramWidget(QWidget):
    """Nine on-panel sockets (A–I) in physical layout; highlights the active letter."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        grid = QGridLayout(self)
        grid.setContentsMargins(0, 0, 0, 0)
        grid.setHorizontalSpacing(10)
        grid.setVerticalSpacing(8)

        self._labels: dict[str, QLabel] = {}
        for letter, (row, col, large) in _SOCKET_GRID.items():
            label = QLabel(letter)
            label.setAlignment(Qt.AlignmentFlag.AlignCenter)
            label.setFixedSize(
                _SOCKET_LARGE_PX if large else _SOCKET_SMALL_PX,
                _SOCKET_LARGE_PX if large else _SOCKET_SMALL_PX,
            )
            self._labels[letter] = label
            grid.addWidget(label, row, col, alignment=Qt.AlignmentFlag.AlignCenter)

        self.set_active_socket("A")

    def set_active_socket(self, letter: str) -> None:
        active = letter.upper()
        for socket_letter, label in self._labels.items():
            is_active = socket_letter == active
            large = _SOCKET_GRID[socket_letter][2]
            size = _SOCKET_LARGE_PX if large else _SOCKET_SMALL_PX
            radius = size // 2
            font_px = 20 if large else 14
            if is_active:
                label.setStyleSheet(
                    f"border: 3px solid {ORANGE}; border-radius: {radius}px;"
                    f" background: {ORANGE_BG}; font-size: {font_px}px;"
                    f" font-weight: 700; color: {ORANGE};"
                )
            else:
                label.setStyleSheet(
                    f"border: 2px solid {MUTED}; border-radius: {radius}px;"
                    f" background: #ffffff; font-size: {font_px}px;"
                    f" font-weight: 600; color: {MUTED};"
                )


class QLabelParams(QLabel):
    def __init__(self, text: str, parent: QWidget | None = None) -> None:
        super().__init__(text, parent)

    def setText(self, text: str) -> None:
        super().setText(f"<b>{text}</b>")


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
        # TODO: add database handler here so we extract the parameters for the selected tube
        selection_row.addWidget(self._tube_combo)

        params_box = QGroupBox("MEASUREMENT PARAMETERS")
        params_layout = QVBoxLayout(params_box)

        setpoints_grid = QGridLayout()
        setpoints_grid.setHorizontalSpacing(16)

        self._ua = QLabelParams("250")
        self._ug1 = QLabelParams("-1.5")
        self._ug2 = QLabelParams("—")

        self._add_expected_row(setpoints_grid, 0, "Anode Voltage", "(Va)", self._ua, "V")
        self._add_expected_row(setpoints_grid, 1, "Grid Voltage", "(Vg1)", self._ug1, "V")
        self._add_expected_row(setpoints_grid, 2, "Screen Voltage", "(Vg2)", self._ug2, "V")

        h_line = QFrame()
        h_line.setFrameShape(QFrame.Shape.HLine)  # Set to horizontal line
        h_line.setFrameShadow(QFrame.Shadow.Sunken) # Optional: adds depth

        setpoints_grid.addWidget(h_line, 3, 0, 1, 4)

        expected_title = QLabel("EXPECTED VALUES")
        expected_title.setStyleSheet(f"color: {MUTED}; font-weight: 600; font-size: 11px; margin-top: 8px; text-align: center;")
        
        setpoints_grid.addWidget(expected_title, 4, 0, 1, 4)
        params_layout.addLayout(setpoints_grid)

        params_layout.addWidget(expected_title)

        expected_grid = QGridLayout()
        expected_grid.setHorizontalSpacing(16)
        self._exp_ia = QLabelParams("3")
        self._exp_ig2 = QLabelParams("—")
        self._exp_gm = QLabelParams("—")
        self._exp_mu = QLabelParams("—")
        self._exp_ra = QLabelParams("—")
        self._add_expected_row(expected_grid, 0, "Anode Current", "(Ia)", self._exp_ia, "mA")
        self._add_expected_row(expected_grid, 1, "Screen Current", "(Ig2)", self._exp_ig2, "mA")
        self._add_expected_row(expected_grid, 2, "Transconductance", "(gm)", self._exp_gm, "S")
        self._add_expected_row(expected_grid, 3, "Amplification", "(μ)", self._exp_mu, "")
        self._add_expected_row(expected_grid, 4, "Anode Resistance", "(Ra)", self._exp_ra, "kΩ")
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
        self._socket_diagram = SocketDiagramWidget()
        socket_col.addWidget(socket_label, alignment=Qt.AlignmentFlag.AlignHCenter)
        socket_col.addWidget(self._socket_diagram, alignment=Qt.AlignmentFlag.AlignHCenter)
        self._tube_hint = QLabel("12-pin compact socket")
        self._tube_hint.setStyleSheet(f"color: {TEAL}; font-size: 11px;")
        self._tube_hint.setOpenExternalLinks(True)
        self._tube_hint.setText('<a href="#">View datasheet</a>')
        socket_col.addWidget(self._tube_hint, alignment=Qt.AlignmentFlag.AlignHCenter)
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

    def tube_combo(self) -> QComboBox:
        return self._tube_combo

    def select_tube(self, name: str) -> None:
        index = self._tube_combo.findData(name)
        if index >= 0:
            self._tube_combo.setCurrentIndex(index)

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

        if len(self._systems) > 1:
            self._sys2.setCheckable(True)
        else:
            self._sys2.setCheckable(False)
            self._sys1.setChecked(True)

        self._socket_diagram.set_active_socket(tube.socket)
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
    def _add_expected_row(grid: QGridLayout, row: int, label: str, abbrev: str, value: QLabel, unit: str) -> None:
        name = QLabel(label)
        name.setStyleSheet(f"color: {MUTED};")
        abbrev = QLabel(abbrev)
        abbrev.setStyleSheet(f"color: {MUTED};")
        grid.addWidget(name, row, 0)
        grid.addWidget(abbrev, row, 1)
        grid.addWidget(value, row, 2)
        if unit:
            grid.addWidget(QLabel(unit), row, 3)
