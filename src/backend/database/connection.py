"""SQLite connection and schema initialization."""

from __future__ import annotations

import sqlite3
from pathlib import Path

_SCHEMA_PATH = Path(__file__).with_name("schema.sql")
_DEFAULT_DB_PATH = Path(__file__).resolve().parents[3] / "data" / "vttester.db"


def default_db_path() -> Path:
    return _DEFAULT_DB_PATH


def connect(db_path: Path | str | None = None) -> sqlite3.Connection:
    path = Path(db_path) if db_path is not None else _DEFAULT_DB_PATH
    path.parent.mkdir(parents=True, exist_ok=True)
    conn = sqlite3.connect(path)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA foreign_keys = ON")
    return conn


def init_db(db_path: Path | str | None = None) -> Path:
    path = Path(db_path) if db_path is not None else _DEFAULT_DB_PATH
    schema = _SCHEMA_PATH.read_text(encoding="utf-8")
    with connect(path) as conn:
        conn.executescript(schema)
        from backend.database.seed import seed_builtin_conditions

        seed_builtin_conditions(conn)
        conn.commit()
    return path
