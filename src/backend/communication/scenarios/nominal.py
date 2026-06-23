"""Nominal SET → STATUS → RESET flow against harness nominal ADC preset."""

from backend.communication.harness_client import Step
from backend.communication.protocol import (
    CommandCode,
    Data,
    ResetKind,
    prepare_request,
)

# Matches synthetic_measure_load_nominal() in firmware (float × 64, uint16 truncate).
NOMINAL_SUMS = {
    "ug_sum": int(20.1 * 64),
    "uh_sum": int(126.1 * 64),
    "ih_sum": int(30.5 * 64),
    "ua_sum": int(256.4 * 64),
    "ia_sum": int(15.1 * 64),
    "ue_sum": 0,
    "ie_sum": 0,
    "tm_sum": int(12.0 * 64),
    "err": 0,
}

NOMINAL_STEPS = [
    Step(
        name="SET valid",
        tx=lambda: prepare_request(
            CommandCode.CMD_SET,
            a1_a2="0",
            ug=100,
            uh=63,
            ih=0,
            ua=300,
            ue=0,
        ),
        expect="RSP_ACK",
    ),
    Step(
        name="STATUS nominal DATA",
        tx=lambda: prepare_request(CommandCode.CMD_STATUS),
        expect=Data,
        expect_fields={"a1_a2": "0", **NOMINAL_SUMS},
    ),
    Step(
        name="RESET full",
        tx=lambda: prepare_request(
            CommandCode.CMD_RESET, reset_kind=ResetKind.RESET_FULL
        ),
        expect="RSP_ACK",
    ),
    Step(
        name="STATUS after RESET",
        tx=lambda: prepare_request(CommandCode.CMD_STATUS),
        expect=Data,
        expect_fields={"a1_a2": "0", **NOMINAL_SUMS},
    ),
]
