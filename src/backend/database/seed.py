"""Seed built-in conditions and ECC83 / 12AX7 demo catalog."""

from __future__ import annotations

import sqlite3

from backend.database.repository import CatalogRepository, PointResult
from backend.database.types import MeasurementKind, TubeCondition, TubeKind

_BUILTIN_CONDITIONS: tuple[
    tuple[TubeCondition, int, float | None, float | None, float | None, float | None],
    ...,
] = (
    # name, sort, ia_min, ia_max, ig2_min, ig2_max  (% of expected)
    (TubeCondition.NOS, 10, 95.0, None, 95.0, None),
    (TubeCondition.USABLE, 20, 70.0, 95.0, 70.0, 95.0),
    (TubeCondition.NEAR_END_LIFE, 30, 50.0, 70.0, 50.0, 70.0),
    (TubeCondition.USELESS, 40, None, 50.0, None, 50.0),
    # Fault labels — null bands; assignment logic lives in the app, not SQL.
    (TubeCondition.SHORTED, 50, None, None, None, None),
    (TubeCondition.OPEN, 60, None, None, None, None),
    (TubeCondition.GASSY, 70, None, None, None, None),
    (TubeCondition.LEAKY, 80, None, None, None, None),
)


def seed_builtin_conditions(conn: sqlite3.Connection) -> None:
    for name, sort_order, ia_min, ia_max, ig2_min, ig2_max in _BUILTIN_CONDITIONS:
        conn.execute(
            """
            INSERT INTO tube_conditions (
                name, sort_order, is_builtin,
                ia_pct_min, ia_pct_max, ig2_pct_min, ig2_pct_max
            ) VALUES (?, ?, 1, ?, ?, ?, ?)
            ON CONFLICT(name) DO UPDATE SET
                sort_order = excluded.sort_order,
                ia_pct_min = excluded.ia_pct_min,
                ia_pct_max = excluded.ia_pct_max,
                ig2_pct_min = excluded.ig2_pct_min,
                ig2_pct_max = excluded.ig2_pct_max
            WHERE tube_conditions.is_builtin = 1
            """,
            (str(name), sort_order, ia_min, ia_max, ig2_min, ig2_max),
        )


def seed_demo_catalog(conn: sqlite3.Connection) -> None:
    repo = CatalogRepository(conn)

    if repo.get_tube_type_by_name("ECC83"):
        return

    ecc83_id = repo.insert_tube_type(
        name="ECC83",
        tube_kind=TubeKind.DUO_TRIODE,
        socket="F",
        datasheet_url="https://tube-data.com/sheets/030/e/ECC83.pdf",
    )
    for sys_no, ia, gm, mu, ra in (
        (1, 2.0, 2.0, 100.0, 55.8),
        (2, 2.0, 2.0, 100.0, 55.8),
    ):
        repo.insert_tube_type_system(
            ecc83_id,
            system_number=sys_no,
            heater_voltage_v=6.3,
            ug1_v=-1.5,
            ua_v=250.0,
            ia_expected_ma=ia,
            transconductance_s_expected=gm,
            gain_k_expected=mu,
            anode_resistance_r_expected=ra,
        )

    ax7_id = repo.insert_tube_type(
        name="12AX7",
        tube_kind=TubeKind.DUO_TRIODE,
        socket="F",
    )
    for sys_no in (1, 2):
        repo.insert_tube_type_system(
            ax7_id,
            system_number=sys_no,
            heater_voltage_v=6.3,
            ug1_v=-1.5,
            ua_v=250.0,
            ia_expected_ma=2.0,
            transconductance_s_expected=2.0,
            gain_k_expected=100.0,
            anode_resistance_r_expected=55.8,
        )

    nos_id = repo.get_condition_id(TubeCondition.NOS)
    assert nos_id is not None

    group_id = repo.create_measurement_group(
        ax7_id,
        manufacturer="GE",
        condition_id=nos_id,
        kind=MeasurementKind.POINT,
        measured_at="2026-04-28 15:12:22",
    )
    for sys_no, ia, gm, mu, ra in (
        (1, 2.05, 2.02, 99.8, 56.2),
        (2, 1.95, 1.98, 100.1, 55.8),
    ):
        repo.add_point_measurement(
            group_id,
            sys_no,
            PointResult(ia_ma=ia, transconductance_s=gm, gain_k=mu, anode_resistance_r=ra),
        )
