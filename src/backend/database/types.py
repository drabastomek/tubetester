"""Shared enums and helpers for the catalog database."""

from __future__ import annotations

from dataclasses import dataclass
from enum import StrEnum
from typing import Protocol


class TubeKind(StrEnum):
    DIODE = "diode"
    DUO_DIODE = "duo_diode"
    TRIODE = "triode"
    DUO_TRIODE = "duo_triode"
    TETRODE = "tetrode"
    PENTODE = "pentode"
    DUO_PENTODE = "duo_pentode"
    HEPTODE = "heptode"
    OTHER = "other"


class TubeCondition(StrEnum):
    """Built-in condition codes seeded into ``tube_conditions``."""

    NOS = "NOS"
    USABLE = "USABLE"
    NEAR_END_LIFE = "NEAR_END_LIFE"
    USELESS = "USELESS"
    SHORTED = "SHORTED"
    OPEN = "OPEN"
    GASSY = "GASSY"
    LEAKY = "LEAKY"


# Life bands (% of expected Ia/Ig2) — auto-assignable via ``suggest_condition``.
LIFE_TUBE_CONDITIONS: tuple[str, ...] = (
    TubeCondition.NOS,
    TubeCondition.USABLE,
    TubeCondition.NEAR_END_LIFE,
    TubeCondition.USELESS,
)

# Fault labels — no pct bands; detection is hardcoded in the measurement model / UI.
FAULT_TUBE_CONDITIONS: tuple[str, ...] = (
    TubeCondition.SHORTED,
    TubeCondition.OPEN,
    TubeCondition.GASSY,
    TubeCondition.LEAKY,
)

# All built-in codes in default UI order (see ``seed_builtin_conditions``).
SUGGESTED_TUBE_CONDITIONS: tuple[str, ...] = LIFE_TUBE_CONDITIONS + FAULT_TUBE_CONDITIONS


def normalize_condition_name(value: str) -> str:
    """Strip and validate a condition label before lookup or insert."""
    text = value.strip()
    if not text:
        raise ValueError("condition name must be non-empty")
    return text


@dataclass(frozen=True)
class ConditionPctBand:
    """Measured current as % of expected — inclusive bounds, NULL = open."""

    ia_pct_min: float | None = None
    ia_pct_max: float | None = None
    ig2_pct_min: float | None = None
    ig2_pct_max: float | None = None

    def has_ia_band(self) -> bool:
        return self.ia_pct_min is not None or self.ia_pct_max is not None

    def has_ig2_band(self) -> bool:
        return self.ig2_pct_min is not None or self.ig2_pct_max is not None

    def has_any_band(self) -> bool:
        return self.has_ia_band() or self.has_ig2_band()


class _ConditionBandSource(Protocol):
    id: int
    sort_order: int
    ia_pct_min: float | None
    ia_pct_max: float | None
    ig2_pct_min: float | None
    ig2_pct_max: float | None


def measured_pct(measured: float | None, expected: float | None) -> float | None:
    """Return measured as % of expected, or None if not computable."""
    if measured is None or expected is None or expected == 0:
        return None
    return 100.0 * measured / expected


def pct_within_band(
    pct: float | None,
    min_pct: float | None,
    max_pct: float | None,
) -> bool:
    """True when pct is inside [min_pct, max_pct] (NULL bound = open)."""
    if pct is None:
        return False
    if min_pct is not None and pct < min_pct:
        return False
    if max_pct is not None and pct > max_pct:
        return False
    return True


def condition_band_matches(
    band: ConditionPctBand,
    *,
    ia_pct: float | None,
    ig2_pct: float | None,
) -> bool:
    """True when any configured metric with data falls within its band."""
    if not band.has_any_band():
        return False
    matches: list[bool] = []
    if band.has_ia_band() and ia_pct is not None:
        matches.append(pct_within_band(ia_pct, band.ia_pct_min, band.ia_pct_max))
    if band.has_ig2_band() and ig2_pct is not None:
        matches.append(pct_within_band(ig2_pct, band.ig2_pct_min, band.ig2_pct_max))
    if not matches:
        return False
    return any(matches)


def condition_row_to_band(row: _ConditionBandSource) -> ConditionPctBand:
    return ConditionPctBand(
        ia_pct_min=row.ia_pct_min,
        ia_pct_max=row.ia_pct_max,
        ig2_pct_min=row.ig2_pct_min,
        ig2_pct_max=row.ig2_pct_max,
    )


def resolve_condition_for_measurement(
    conditions: list[_ConditionBandSource],
    *,
    measured_ia_ma: float | None,
    expected_ia_ma: float | None,
    measured_ig2_ma: float | None = None,
    expected_ig2_ma: float | None = None,
) -> int | None:
    """Pick the first matching condition id (sort_order, then name)."""
    ia_pct = measured_pct(measured_ia_ma, expected_ia_ma)
    ig2_pct = measured_pct(measured_ig2_ma, expected_ig2_ma)
    for row in sorted(conditions, key=lambda c: (c.sort_order, c.id)):
        band = condition_row_to_band(row)
        if condition_band_matches(band, ia_pct=ia_pct, ig2_pct=ig2_pct):
            return row.id
    return None


class MeasurementKind(StrEnum):
    POINT = "POINT"
    SWEEP = "SWEEP"


def socket_needs_banana_adapter(socket: str) -> bool:
    """Sockets J–Z use banana-plug adapters (per hardware note)."""
    if len(socket) != 1:
        raise ValueError("socket must be a single letter A–Z")
    return socket.upper() >= "J"
