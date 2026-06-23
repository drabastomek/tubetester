"""Optional: HARNESS_FLOW=auto firmware only (SET → DATA, no ACK).

**Not the production flow** — see MEASUREMENT_FLOW.md (split: SET → ACK → STATUS).

Run only when explicitly requested:

  make -C firmware HARNESS_FLOW=auto HARNESS_MEAS_DELAY_MS=100 flash
  export VTTESTER_RUN_AUTO_MEASURE=1
  pytest -m hardware src/backend/communication/tests/test_auto_measure_serial.py
"""

import os

import pytest

pytest.importorskip("serial")

from backend.communication.harness_client import ScenarioError
from backend.communication.mcu_serial import VTTesterSerial
from backend.communication.protocol import CommandCode, Data, prepare_request
from backend.communication.scenarios.nominal import NOMINAL_SUMS

_REFLASH_AUTO = (
    "MCU sent ACK after SET (split-mode firmware on the wire). Reflash with:\n"
    "  make -C firmware HARNESS_FLOW=auto HARNESS_MEAS_DELAY_MS=100 flash"
)


@pytest.mark.hardware
@pytest.mark.skipif(
    os.environ.get("VTTESTER_RUN_AUTO_MEASURE") != "1",
    reason="optional legacy harness mode; unset or use split firmware (default make flash)",
)
def test_auto_measure(harness_port):
    with VTTesterSerial(harness_port, timeout=0.2) as link:
        link.drain()
        link.send_message(
            prepare_request(
                CommandCode.CMD_SET,
                a1_a2="0",
                ug=100,
                uh=63,
                ua=300,
            )
        )
        response = link.read_response(timeout_s=1.0)

        if response == "RSP_ACK":
            pytest.fail(_REFLASH_AUTO)

        if not isinstance(response, Data):
            raise ScenarioError(
                f"expected Data after SET, got {type(response).__name__}: {response!r}"
            )

        for key, expected in {"a1_a2": "0", **NOMINAL_SUMS}.items():
            actual = getattr(response, key)
            assert actual == expected, f"{key}: expected {expected!r}, got {actual!r}"
