"""Silent fault consumed once, then normal STATUS DATA."""

from backend.communication.harness_client import Step
from backend.communication.protocol import (
    CommandCode,
    Data,
    HARNS_FAULT_SILENT_ONCE,
    prepare_harness_arm,
    prepare_request,
)
from backend.communication.scenarios.nominal import NOMINAL_SUMS

FAULT_SILENT_STEPS = [
    Step(
        name="arm silent-once fault",
        tx=lambda: prepare_harness_arm(HARNS_FAULT_SILENT_ONCE),
        expect="RSP_ACK",
    ),
    Step(
        name="STATUS silently discarded",
        tx=lambda: prepare_request(CommandCode.CMD_STATUS),
        expect=None,
        timeout_s=0.25,
    ),
    Step(
        name="STATUS after fault consumed",
        tx=lambda: prepare_request(CommandCode.CMD_STATUS),
        expect=Data,
        expect_fields={"a1_a2": "0", **NOMINAL_SUMS},
    ),
]
