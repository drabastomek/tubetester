"""Tier B — serial integration against flashed harness (VTTESTER_PORT)."""

import pytest

pytest.importorskip("serial")

from backend.communication.harness_client import run_scenario
from backend.communication.mcu_serial import VTTesterSerial
from backend.communication.scenarios import SCENARIOS


@pytest.mark.hardware
@pytest.mark.parametrize("scenario_name", sorted(SCENARIOS))
def test_harness_scenario(harness_port, scenario_name):
    steps = SCENARIOS[scenario_name]
    with VTTesterSerial(harness_port, timeout=0.2) as link:
        link.drain()
        run_scenario(link, steps)
