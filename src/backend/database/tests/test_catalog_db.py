"""Database schema and repository tests (no hardware)."""

import tempfile
from pathlib import Path

import pytest

from backend.database.connection import init_db, connect
from backend.database.repository import CatalogRepository, PointResult, SweepPoint
from backend.database.seed import seed_demo_catalog
from backend.database.types import MeasurementKind, TubeCondition, TubeKind, FAULT_TUBE_CONDITIONS, LIFE_TUBE_CONDITIONS, socket_needs_banana_adapter


@pytest.fixture
def db_path(tmp_path: Path) -> Path:
    path = tmp_path / "test.db"
    init_db(path)
    return path


@pytest.fixture
def repo(db_path: Path) -> CatalogRepository:
    conn = connect(db_path)
    seed_demo_catalog(conn)
    yield CatalogRepository(conn)
    conn.close()


def test_schema_creates_tables(db_path: Path):
    with connect(db_path) as conn:
        tables = {
            r[0]
            for r in conn.execute(
                "SELECT name FROM sqlite_master WHERE type='table'"
            )
        }
    assert "tube_types" in tables
    assert "tube_conditions" in tables
    assert "measurement_point_results" in tables
    assert "measurement_sweep_results" in tables


def test_seed_tube_types(repo: CatalogRepository):
    names = [t.name for t in repo.list_tube_types()]
    assert "ECC83" in names
    assert "12AX7" in names


def test_builtin_conditions(repo: CatalogRepository):
    conditions = repo.list_conditions()
    names = [c.name for c in conditions]
    assert names == list(LIFE_TUBE_CONDITIONS) + list(FAULT_TUBE_CONDITIONS)
    assert all(c.is_builtin for c in conditions)

    nos = next(c for c in conditions if c.name == "NOS")
    assert nos.ia_pct_min == pytest.approx(95.0)
    assert nos.ia_pct_max is None

    usable = next(c for c in conditions if c.name == "USABLE")
    assert usable.ia_pct_min == pytest.approx(70.0)
    assert usable.ia_pct_max == pytest.approx(95.0)

    for fault_name in FAULT_TUBE_CONDITIONS:
        fault = next(c for c in conditions if c.name == fault_name)
        assert fault.ia_pct_min is None
        assert fault.ia_pct_max is None
        assert fault.ig2_pct_min is None
        assert fault.ig2_pct_max is None


def test_suggest_condition_from_ia_pct(repo: CatalogRepository):
    expected = 2.0
    nos = repo.suggest_condition(measured_ia_ma=1.95, expected_ia_ma=expected)
    assert nos is not None
    assert nos.name == "NOS"

    usable = repo.suggest_condition(measured_ia_ma=1.5, expected_ia_ma=expected)
    assert usable is not None
    assert usable.name == "USABLE"

    near_end = repo.suggest_condition(measured_ia_ma=1.2, expected_ia_ma=expected)
    assert near_end is not None
    assert near_end.name == "NEAR_END_LIFE"

    useless = repo.suggest_condition(measured_ia_ma=0.8, expected_ia_ma=expected)
    assert useless is not None
    assert useless.name == "USELESS"


def test_custom_condition_has_no_auto_band(repo: CatalogRepository):
    custom_id = repo.ensure_condition("GAS / leaky")
    custom = repo.get_condition_by_id(custom_id)
    assert custom is not None
    assert custom.ia_pct_min is None
    assert custom.ia_pct_max is None
    assert custom.ig2_pct_min is None
    assert custom.ig2_pct_max is None

    suggested = repo.suggest_condition(measured_ia_ma=2.0, expected_ia_ma=2.0)
    assert suggested is not None
    assert suggested.id != custom_id
    assert suggested.name == "NOS"


def test_catalog_list_for_tube_name(repo: CatalogRepository):
    rows = repo.list_catalog_for_tube_name("12AX7")
    assert len(rows) == 1
    assert rows[0].kind == "POINT"
    assert rows[0].manufacturer == "GE"
    assert rows[0].condition == "NOS"
    assert rows[0].condition_id == repo.get_condition_id(TubeCondition.NOS)


def test_group_detail_two_systems(repo: CatalogRepository):
    rows = repo.list_catalog_for_tube_name("12AX7")
    detail = repo.get_group_measurements(rows[0].group_id)
    assert len(detail) == 2
    assert detail[0]["point"].ia_ma == pytest.approx(2.05)


def test_sweep_json_roundtrip(repo: CatalogRepository):
    ecc = repo.get_tube_type_by_name("ECC83")
    assert ecc is not None
    gid = repo.create_measurement_group(
        ecc.id,
        manufacturer="RFT",
        condition=TubeCondition.NOS,
        kind=MeasurementKind.SWEEP,
        sweep_grid={"ua_min": 0, "ua_max": 250, "ua_step": 10, "ug1_min": -2, "ug1_max": 0, "ug1_step": 0.5},
    )
    repo.add_sweep_measurement(
        gid,
        1,
        [
            SweepPoint(ua=0, ug1=0.0, ug2=None, ia=0.0),
            SweepPoint(ua=250, ug1=-1.5, ug2=None, ia=2.1),
        ],
        metrics={"rmse_ia": 0.01, "mae_ia": 0.008, "mape_ia": 1.2},
    )
    detail = repo.get_group_measurements(gid)
    assert len(detail[0]["points"]) == 2


def test_socket_banana_adapter():
    assert not socket_needs_banana_adapter("F")
    assert socket_needs_banana_adapter("J")
    assert socket_needs_banana_adapter("Z")


def test_custom_condition(repo: CatalogRepository):
    ecc = repo.get_tube_type_by_name("ECC83")
    assert ecc is not None
    useless_id = repo.get_condition_id("USELESS")
    assert useless_id is not None
    gid = repo.create_measurement_group(
        ecc.id,
        manufacturer="POLAM",
        condition_id=useless_id,
        kind=MeasurementKind.POINT,
    )
    rows = repo.list_catalog_for_tube_name("ECC83")
    assert any(
        r.group_id == gid and r.condition_id == useless_id and r.condition == "USELESS"
        for r in rows
    )

    custom_id = repo.ensure_condition("GAS / leaky")
    gid2 = repo.create_measurement_group(
        ecc.id,
        manufacturer="?",
        condition_id=custom_id,
        kind=MeasurementKind.POINT,
    )
    custom = repo.get_condition_by_id(custom_id)
    assert custom is not None
    assert custom.is_builtin is False
    assert custom.name == "GAS / leaky"
    detail = repo.get_group_measurements(gid2)
    assert len(detail) == 0  # no systems added yet
