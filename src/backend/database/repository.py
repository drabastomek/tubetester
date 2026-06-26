"""Catalog CRUD — tube types and measurement history."""

from __future__ import annotations

import json
import sqlite3
from dataclasses import dataclass
from typing import Any

from backend.database.types import (
    MeasurementKind,
    TubeKind,
    normalize_condition_name,
    resolve_condition_for_measurement,
)


@dataclass(frozen=True)
class TubeType:
    id: int
    name: str
    tube_kind: str
    socket: str
    datasheet_url: str | None
    notes: str | None


@dataclass(frozen=True)
class TubeTypeSystem:
    id: int
    tube_type_id: int
    system_number: int
    heater_voltage_v: float | None
    heater_current_ma: float | None
    ug1_v: float
    ua_v: float
    ug2_v: float | None
    ia_expected_ma: float | None
    ig2_expected_ma: float | None
    transconductance_s_expected: float | None
    gain_k_expected: float | None
    anode_resistance_r_expected: float | None


@dataclass(frozen=True)
class TubeConditionRow:
    id: int
    name: str
    sort_order: int
    is_builtin: bool
    ia_pct_min: float | None
    ia_pct_max: float | None
    ig2_pct_min: float | None
    ig2_pct_max: float | None


@dataclass(frozen=True)
class CatalogListRow:
    """One row in the TUBE CATALOG table (measurement_groups)."""

    group_id: int
    measured_at: str
    tube_name: str
    manufacturer: str | None
    condition_id: int
    condition: str
    kind: str


@dataclass(frozen=True)
class PointResult:
    ia_ma: float | None = None
    ig2_ma: float | None = None
    transconductance_s: float | None = None
    gain_k: float | None = None
    anode_resistance_r: float | None = None


@dataclass(frozen=True)
class SweepPoint:
    ua: float
    ug1: float
    ug2: float | None
    ia: float | None
    ig2: float | None = None
    gm: float | None = None
    mu: float | None = None
    ra: float | None = None


def _row_to_tube_condition(row: sqlite3.Row) -> TubeConditionRow:
    return TubeConditionRow(
        id=row["id"],
        name=row["name"],
        sort_order=row["sort_order"],
        is_builtin=bool(row["is_builtin"]),
        ia_pct_min=row["ia_pct_min"],
        ia_pct_max=row["ia_pct_max"],
        ig2_pct_min=row["ig2_pct_min"],
        ig2_pct_max=row["ig2_pct_max"],
    )


def _row_to_tube_type(row: sqlite3.Row) -> TubeType:
    return TubeType(
        id=row["id"],
        name=row["name"],
        tube_kind=row["tube_kind"],
        socket=row["socket"],
        datasheet_url=row["datasheet_url"],
        notes=row["notes"],
    )


def _row_to_tube_type_system(row: sqlite3.Row) -> TubeTypeSystem:
    return TubeTypeSystem(
        id=row["id"],
        tube_type_id=row["tube_type_id"],
        system_number=row["system_number"],
        heater_voltage_v=row["heater_voltage_v"],
        heater_current_ma=row["heater_current_ma"],
        ug1_v=row["ug1_v"],
        ua_v=row["ua_v"],
        ug2_v=row["ug2_v"],
        ia_expected_ma=row["ia_expected_ma"],
        ig2_expected_ma=row["ig2_expected_ma"],
        transconductance_s_expected=row["transconductance_s_expected"],
        gain_k_expected=row["gain_k_expected"],
        anode_resistance_r_expected=row["anode_resistance_r_expected"],
    )


class CatalogRepository:
    def __init__(self, conn: sqlite3.Connection) -> None:
        self._conn = conn

    # --- tube type catalog -------------------------------------------------

    def list_tube_types(self) -> list[TubeType]:
        cur = self._conn.execute(
            "SELECT * FROM tube_types ORDER BY name COLLATE NOCASE"
        )
        return [_row_to_tube_type(r) for r in cur.fetchall()]

    def get_tube_type_by_name(self, name: str) -> TubeType | None:
        cur = self._conn.execute(
            "SELECT * FROM tube_types WHERE name = ?", (name,)
        )
        row = cur.fetchone()
        return _row_to_tube_type(row) if row else None

    def insert_tube_type(
        self,
        *,
        name: str,
        tube_kind: TubeKind | str,
        socket: str,
        datasheet_url: str | None = None,
        notes: str | None = None,
    ) -> int:
        cur = self._conn.execute(
            """
            INSERT INTO tube_types (name, tube_kind, socket, datasheet_url, notes)
            VALUES (?, ?, ?, ?, ?)
            """,
            (name, str(tube_kind), socket.upper(), datasheet_url, notes),
        )
        self._conn.commit()
        return int(cur.lastrowid)

    def insert_tube_type_system(
        self,
        tube_type_id: int,
        *,
        system_number: int,
        ug1_v: float,
        ua_v: float,
        ug2_v: float | None = None,
        heater_voltage_v: float | None = None,
        heater_current_ma: float | None = None,
        ia_expected_ma: float | None = None,
        ig2_expected_ma: float | None = None,
        transconductance_s_expected: float | None = None,
        gain_k_expected: float | None = None,
        anode_resistance_r_expected: float | None = None,
    ) -> int:
        cur = self._conn.execute(
            """
            INSERT INTO tube_type_systems (
                tube_type_id, system_number,
                heater_voltage_v, heater_current_ma,
                ug1_v, ua_v, ug2_v,
                ia_expected_ma, ig2_expected_ma,
                transconductance_s_expected, gain_k_expected,
                anode_resistance_r_expected
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                tube_type_id,
                system_number,
                heater_voltage_v,
                heater_current_ma,
                ug1_v,
                ua_v,
                ug2_v,
                ia_expected_ma,
                ig2_expected_ma,
                transconductance_s_expected,
                gain_k_expected,
                anode_resistance_r_expected,
            ),
        )
        self._conn.commit()
        return int(cur.lastrowid)

    def list_tube_type_systems(self, tube_type_id: int) -> list[TubeTypeSystem]:
        cur = self._conn.execute(
            """
            SELECT * FROM tube_type_systems
            WHERE tube_type_id = ?
            ORDER BY system_number
            """,
            (tube_type_id,),
        )
        return [_row_to_tube_type_system(r) for r in cur.fetchall()]

    # --- tube conditions ---------------------------------------------------

    def list_conditions(self) -> list[TubeConditionRow]:
        cur = self._conn.execute(
            """
            SELECT * FROM tube_conditions
            ORDER BY sort_order, name COLLATE NOCASE
            """
        )
        return [_row_to_tube_condition(r) for r in cur.fetchall()]

    def get_condition_by_id(self, condition_id: int) -> TubeConditionRow | None:
        cur = self._conn.execute(
            "SELECT * FROM tube_conditions WHERE id = ?",
            (condition_id,),
        )
        row = cur.fetchone()
        return _row_to_tube_condition(row) if row else None

    def get_condition_id(self, name: str) -> int | None:
        label = normalize_condition_name(name)
        cur = self._conn.execute(
            "SELECT id FROM tube_conditions WHERE name = ?",
            (label,),
        )
        row = cur.fetchone()
        return int(row["id"]) if row else None

    def ensure_condition(self, name: str) -> int:
        """Return existing condition id, or insert a user-defined row."""
        label = normalize_condition_name(name)
        existing = self.get_condition_id(label)
        if existing is not None:
            return existing
        cur = self._conn.execute(
            """
            INSERT INTO tube_conditions (name, sort_order, is_builtin)
            VALUES (
                ?,
                COALESCE((SELECT MAX(sort_order) FROM tube_conditions), 0) + 10,
                0
            )
            """,
            (label,),
        )
        self._conn.commit()
        return int(cur.lastrowid)

    def update_condition_thresholds(
        self,
        condition_id: int,
        *,
        ia_pct_min: float | None,
        ia_pct_max: float | None,
        ig2_pct_min: float | None,
        ig2_pct_max: float | None,
    ) -> None:
        if self.get_condition_by_id(condition_id) is None:
            raise ValueError(f"unknown condition_id: {condition_id}")
        self._conn.execute(
            """
            UPDATE tube_conditions
            SET ia_pct_min = ?, ia_pct_max = ?,
                ig2_pct_min = ?, ig2_pct_max = ?
            WHERE id = ?
            """,
            (ia_pct_min, ia_pct_max, ig2_pct_min, ig2_pct_max, condition_id),
        )
        self._conn.commit()

    def suggest_condition(
        self,
        *,
        measured_ia_ma: float | None,
        expected_ia_ma: float | None,
        measured_ig2_ma: float | None = None,
        expected_ig2_ma: float | None = None,
    ) -> TubeConditionRow | None:
        """First matching condition by sort_order, or None."""
        conditions = self.list_conditions()
        condition_id = resolve_condition_for_measurement(
            conditions,
            measured_ia_ma=measured_ia_ma,
            expected_ia_ma=expected_ia_ma,
            measured_ig2_ma=measured_ig2_ma,
            expected_ig2_ma=expected_ig2_ma,
        )
        if condition_id is None:
            return None
        return self.get_condition_by_id(condition_id)

    def _resolve_condition_id(
        self,
        *,
        condition_id: int | None,
        condition: str | None,
    ) -> int:
        if condition_id is not None:
            if self.get_condition_by_id(condition_id) is None:
                raise ValueError(f"unknown condition_id: {condition_id}")
            return condition_id
        if condition is not None:
            return self.ensure_condition(condition)
        raise ValueError("condition_id or condition is required")

    # --- measurement catalog -----------------------------------------------

    def list_catalog_for_tube_name(self, tube_name: str) -> list[CatalogListRow]:
        cur = self._conn.execute(
            """
            SELECT g.id AS group_id,
                   g.measured_at,
                   t.name AS tube_name,
                   g.manufacturer,
                   g.condition_id,
                   c.name AS condition,
                   g.kind
            FROM measurement_groups g
            JOIN tube_types t ON t.id = g.tube_type_id
            JOIN tube_conditions c ON c.id = g.condition_id
            WHERE t.name = ?
            ORDER BY g.measured_at DESC
            """,
            (tube_name,),
        )
        return [
            CatalogListRow(
                group_id=r["group_id"],
                measured_at=r["measured_at"],
                tube_name=r["tube_name"],
                manufacturer=r["manufacturer"],
                condition_id=r["condition_id"],
                condition=r["condition"],
                kind=r["kind"],
            )
            for r in cur.fetchall()
        ]

    def create_measurement_group(
        self,
        tube_type_id: int,
        *,
        manufacturer: str | None,
        kind: MeasurementKind | str,
        condition_id: int | None = None,
        condition: str | None = None,
        measured_at: str | None = None,
        sweep_grid: dict[str, float] | None = None,
    ) -> int:
        resolved_condition_id = self._resolve_condition_id(
            condition_id=condition_id,
            condition=condition,
        )
        sweep = sweep_grid or {}
        cur = self._conn.execute(
            """
            INSERT INTO measurement_groups (
                tube_type_id, measured_at, manufacturer, condition_id, kind,
                sweep_ua_min, sweep_ua_max, sweep_ua_step,
                sweep_ug1_min, sweep_ug1_max, sweep_ug1_step,
                sweep_ug2_min, sweep_ug2_max, sweep_ug2_step
            ) VALUES (?, COALESCE(?, datetime('now')), ?, ?, ?,
                      ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                tube_type_id,
                measured_at,
                manufacturer,
                resolved_condition_id,
                str(kind),
                sweep.get("ua_min"),
                sweep.get("ua_max"),
                sweep.get("ua_step"),
                sweep.get("ug1_min"),
                sweep.get("ug1_max"),
                sweep.get("ug1_step"),
                sweep.get("ug2_min"),
                sweep.get("ug2_max"),
                sweep.get("ug2_step"),
            ),
        )
        self._conn.commit()
        return int(cur.lastrowid)

    def add_point_measurement(
        self,
        group_id: int,
        system_number: int,
        result: PointResult,
    ) -> int:
        cur = self._conn.execute(
            """
            INSERT INTO measurement_point_results (
                ia_ma, ig2_ma, transconductance_s, gain_k, anode_resistance_r
            ) VALUES (?, ?, ?, ?, ?)
            """,
            (
                result.ia_ma,
                result.ig2_ma,
                result.transconductance_s,
                result.gain_k,
                result.anode_resistance_r,
            ),
        )
        point_id = int(cur.lastrowid)
        self._conn.execute(
            """
            INSERT INTO measurements (group_id, system_number, point_result_id)
            VALUES (?, ?, ?)
            """,
            (group_id, system_number, point_id),
        )
        self._conn.commit()
        return point_id

    def add_sweep_measurement(
        self,
        group_id: int,
        system_number: int,
        points: list[SweepPoint],
        metrics: dict[str, float] | None = None,
    ) -> int:
        payload = [
            {
                "Ua": p.ua,
                "Ug1": p.ug1,
                "Ug2": p.ug2,
                "Ia": p.ia,
                "Ig2": p.ig2,
                "gm": p.gm,
                "mu": p.mu,
                "Ra": p.ra,
            }
            for p in points
        ]
        m = metrics or {}
        cur = self._conn.execute(
            """
            INSERT INTO measurement_sweep_results (
                points_json,
                rmse_ia, mae_ia, mape_ia,
                rmse_gm, mae_gm, mape_gm,
                rmse_mu, mae_mu, mape_mu,
                rmse_ra, mae_ra, mape_ra
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                json.dumps(payload),
                m.get("rmse_ia"),
                m.get("mae_ia"),
                m.get("mape_ia"),
                m.get("rmse_gm"),
                m.get("mae_gm"),
                m.get("mape_gm"),
                m.get("rmse_mu"),
                m.get("mae_mu"),
                m.get("mape_mu"),
                m.get("rmse_ra"),
                m.get("mae_ra"),
                m.get("mape_ra"),
            ),
        )
        sweep_id = int(cur.lastrowid)
        self._conn.execute(
            """
            INSERT INTO measurements (group_id, system_number, sweep_result_id)
            VALUES (?, ?, ?)
            """,
            (group_id, system_number, sweep_id),
        )
        self._conn.commit()
        return sweep_id

    def get_group_measurements(self, group_id: int) -> list[dict[str, Any]]:
        """All systems for one catalog entry (detail dialog / PDF)."""
        cur = self._conn.execute(
            """
            SELECT m.system_number, g.kind,
                   p.ia_ma, p.ig2_ma, p.transconductance_s, p.gain_k, p.anode_resistance_r,
                   s.points_json,
                   s.rmse_ia, s.mae_ia, s.mape_ia
            FROM measurements m
            JOIN measurement_groups g ON g.id = m.group_id
            LEFT JOIN measurement_point_results p ON p.id = m.point_result_id
            LEFT JOIN measurement_sweep_results s ON s.id = m.sweep_result_id
            WHERE m.group_id = ?
            ORDER BY m.system_number
            """,
            (group_id,),
        )
        rows = []
        for r in cur.fetchall():
            item: dict[str, Any] = {
                "system_number": r["system_number"],
                "kind": r["kind"],
            }
            if r["points_json"]:
                item["points"] = json.loads(r["points_json"])
                item["rmse_ia"] = r["rmse_ia"]
                item["mae_ia"] = r["mae_ia"]
                item["mape_ia"] = r["mape_ia"]
            else:
                item["point"] = PointResult(
                    ia_ma=r["ia_ma"],
                    ig2_ma=r["ig2_ma"],
                    transconductance_s=r["transconductance_s"],
                    gain_k=r["gain_k"],
                    anode_resistance_r=r["anode_resistance_r"],
                )
            rows.append(item)
        return rows
