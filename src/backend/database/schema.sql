-- VTTester desktop catalog (SQLite)
-- Tube type definitions + user measurement history (POINT / SWEEP).

PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS tube_types (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    name            TEXT    NOT NULL UNIQUE,
    tube_kind       TEXT    NOT NULL,
    socket          TEXT    NOT NULL CHECK (length(socket) = 1),
    datasheet_url   TEXT,
    notes           TEXT,
    created_at      TEXT    NOT NULL DEFAULT (datetime('now')),
    updated_at      TEXT    NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS tube_type_systems (
    id                          INTEGER PRIMARY KEY AUTOINCREMENT,
    tube_type_id                INTEGER NOT NULL REFERENCES tube_types(id) ON DELETE CASCADE,
    system_number               INTEGER NOT NULL CHECK (system_number BETWEEN 1 AND 3),
    heater_voltage_v            REAL,
    heater_current_ma           REAL,
    ug1_v                       REAL    NOT NULL,
    ua_v                        REAL    NOT NULL,
    ug2_v                       REAL,
    ia_expected_ma              REAL,
    ig2_expected_ma             REAL,
    transconductance_s_expected REAL,
    gain_k_expected             REAL,
    anode_resistance_r_expected  REAL,
    UNIQUE (tube_type_id, system_number)
);

CREATE INDEX IF NOT EXISTS idx_tube_type_systems_type
    ON tube_type_systems (tube_type_id);

-- Tube condition labels (NOS, USABLE, …). User-defined rows allowed.
--
-- Assignment bands: measured Ia or Ig2 as % of the tube type's expected value
-- (tube_type_systems.ia_expected_ma / ig2_expected_ma). NULL bound = open on that
-- side; both bounds inclusive. All four NULL = no pct auto-match (manual label, or
-- fault conditions like SHORTED / OPEN where detection is hardcoded in the app).
CREATE TABLE IF NOT EXISTS tube_conditions (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT    NOT NULL UNIQUE,
    sort_order  INTEGER NOT NULL DEFAULT 0,
    is_builtin  INTEGER NOT NULL DEFAULT 0 CHECK (is_builtin IN (0, 1)),
    ia_pct_min  REAL,
    ia_pct_max  REAL,
    ig2_pct_min REAL,
    ig2_pct_max REAL
);

CREATE INDEX IF NOT EXISTS idx_tube_conditions_sort
    ON tube_conditions (sort_order, name COLLATE NOCASE);

CREATE TABLE IF NOT EXISTS measurement_point_results (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    ia_ma               REAL,
    ig2_ma              REAL,
    transconductance_s  REAL,
    gain_k              REAL,
    anode_resistance_r  REAL
);

CREATE TABLE IF NOT EXISTS measurement_sweep_results (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    points_json     TEXT    NOT NULL,
    rmse_ia         REAL,
    mae_ia          REAL,
    mape_ia         REAL,
    rmse_gm         REAL,
    mae_gm          REAL,
    mape_gm         REAL,
    rmse_mu         REAL,
    mae_mu          REAL,
    mape_mu         REAL,
    rmse_ra         REAL,
    mae_ra          REAL,
    mape_ra         REAL
);

-- Measured tubes (TUBE CATALOG) — one row per test *session*.
--
-- measurement_groups = header for a single time you tested a physical tube:
--   when, which type (ECC83), manufacturer, condition_id, POINT vs SWEEP, sweep grid.
-- Dual triode: one group, two rows in `measurements` (system 1 and 2).
-- Detail numbers live in measurement_point_results / measurement_sweep_results.
CREATE TABLE IF NOT EXISTS measurement_groups (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    tube_type_id    INTEGER NOT NULL REFERENCES tube_types(id),
    measured_at     TEXT    NOT NULL DEFAULT (datetime('now')),
    manufacturer    TEXT,
    condition_id    INTEGER NOT NULL REFERENCES tube_conditions(id),
    kind            TEXT    NOT NULL CHECK (kind IN ('POINT', 'SWEEP')),
    sweep_ua_min    REAL,
    sweep_ua_max    REAL,
    sweep_ua_step   REAL,
    sweep_ug1_min   REAL,
    sweep_ug1_max   REAL,
    sweep_ug1_step  REAL,
    sweep_ug2_min   REAL,
    sweep_ug2_max   REAL,
    sweep_ug2_step  REAL,
    notes           TEXT
);

CREATE TABLE IF NOT EXISTS measurements (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    group_id            INTEGER NOT NULL REFERENCES measurement_groups(id) ON DELETE CASCADE,
    system_number       INTEGER NOT NULL CHECK (system_number BETWEEN 1 AND 3),
    point_result_id     INTEGER REFERENCES measurement_point_results(id),
    sweep_result_id     INTEGER REFERENCES measurement_sweep_results(id),
    UNIQUE (group_id, system_number),
    CHECK (
        (point_result_id IS NOT NULL AND sweep_result_id IS NULL)
        OR (point_result_id IS NULL AND sweep_result_id IS NOT NULL)
    )
);

CREATE INDEX IF NOT EXISTS idx_measurement_groups_type_date
    ON measurement_groups (tube_type_id, measured_at DESC);

CREATE INDEX IF NOT EXISTS idx_measurement_groups_condition
    ON measurement_groups (condition_id);

CREATE INDEX IF NOT EXISTS idx_measurements_group
    ON measurements (group_id);
