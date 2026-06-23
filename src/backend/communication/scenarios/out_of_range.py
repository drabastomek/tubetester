"""Out-of-range SET should return 5-byte ERROR without changing DATA snapshot."""

from backend.communication.harness_client import Step
from backend.communication.protocol import (
    CommandCode,
    Data,
    OutOfRangeError,
    PARAM_ID_UG,
    prepare_request,
)
from backend.communication.scenarios.nominal import NOMINAL_SUMS

OUT_OF_RANGE_STEPS = [
    Step(
        name="SET valid baseline",
        tx=lambda: prepare_request(
            CommandCode.CMD_SET,
            a1_a2="0",
            ug=100,
            uh=63,
            ua=300,
        ),
        expect="RSP_ACK",
    ),
    Step(
        name="SET UG out of range (241 > 240)",
        tx=lambda: prepare_request(
            CommandCode.CMD_SET,
            a1_a2="0",
            ug=241,
            uh=63,
            ua=300,
        ),
        expect=OutOfRangeError,
        expect_fields={"parameter_id": PARAM_ID_UG, "value": 241},
    ),
    Step(
        name="STATUS unchanged after rejected SET",
        tx=lambda: prepare_request(CommandCode.CMD_STATUS),
        expect=Data,
        expect_fields={"a1_a2": "0", **NOMINAL_SUMS},
    ),
]
