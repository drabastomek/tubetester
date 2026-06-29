"""Demo sweep chart (Qt Charts)."""

from __future__ import annotations

from PySide6.QtCharts import QChart, QChartView, QLineSeries, QValueAxis
from PySide6.QtCore import Qt
from PySide6.QtGui import QColor, QPainter
from PySide6.QtWidgets import QSizePolicy, QWidget


class SweepChartView(QWidget):
    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setMinimumHeight(260)

        chart = QChart()
        chart.setBackgroundVisible(False)
        chart.legend().setVisible(True)
        chart.legend().setAlignment(Qt.AlignBottom)

        series_specs = [
            ("Ia", QColor("#2aa8a8"), [(0, 18), (50, 28), (100, 36), (150, 42), (200, 48), (250, 52)]),
            ("gm", QColor("#4a90d9"), [(0, 16), (50, 24), (100, 33), (150, 39), (200, 44), (250, 49)]),
            ("μ", QColor("#e8923a"), [(0, 20), (50, 30), (100, 38), (150, 45), (200, 50), (250, 55)]),
        ]
        for name, color, points in series_specs:
            series = QLineSeries()
            series.setName(name)
            series.setColor(color)
            for x, y in points:
                series.append(x, y)
            chart.addSeries(series)

        axis_x = QValueAxis()
        axis_x.setTitleText("Ua (V)")
        axis_x.setRange(0, 250)
        axis_x.setTickCount(6)
        axis_y = QValueAxis()
        axis_y.setTitleText("Normalized")
        axis_y.setRange(15, 60)
        axis_y.setTickCount(10)
        chart.addAxis(axis_x, Qt.AlignBottom)
        chart.addAxis(axis_y, Qt.AlignLeft)
        for series in chart.series():
            series.attachAxis(axis_x)
            series.attachAxis(axis_y)

        view = QChartView(chart)
        view.setRenderHint(QPainter.Antialiasing)
        view.setRubberBand(QChartView.RectangleRubberBand)

        from PySide6.QtWidgets import QVBoxLayout

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(view)
