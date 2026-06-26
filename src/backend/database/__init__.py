"""VTTester catalog database (SQLite)."""

from backend.database.connection import connect, default_db_path, init_db
from backend.database.repository import CatalogRepository
from backend.database.types import (
    ConditionPctBand,
    FAULT_TUBE_CONDITIONS,
    LIFE_TUBE_CONDITIONS,
    MeasurementKind,
    SUGGESTED_TUBE_CONDITIONS,
    TubeCondition,
    TubeKind,
    condition_band_matches,
    measured_pct,
    normalize_condition_name,
    resolve_condition_for_measurement,
    socket_needs_banana_adapter,
)

__all__ = [
    "CatalogRepository",
    "ConditionPctBand",
    "FAULT_TUBE_CONDITIONS",
    "LIFE_TUBE_CONDITIONS",
    "MeasurementKind",
    "SUGGESTED_TUBE_CONDITIONS",
    "TubeCondition",
    "TubeKind",
    "condition_band_matches",
    "connect",
    "default_db_path",
    "init_db",
    "measured_pct",
    "normalize_condition_name",
    "resolve_condition_for_measurement",
    "socket_needs_banana_adapter",
]
