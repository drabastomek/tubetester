"""Arm hardware-error-on-STATUS fault."""

from backend.communication.harness_client import Step
from backend.communication.protocol import (
    CommandCode,
    ErrorCode,
    HARNS_FAULT_HWERR_ON_STATUS,
    ProtocolError,
    prepare_harness_arm,
    prepare_request,
)

FAULT_HARDWARE_STEPS = [
    Step(
        name="arm HW-error-on-STATUS fault",
        tx=lambda: prepare_harness_arm(HARNS_FAULT_HWERR_ON_STATUS),
        expect="RSP_ACK",
    ),
    Step(
        name="STATUS triggers ERR_HARDWARE",
        tx=lambda: prepare_request(CommandCode.CMD_STATUS),
        expect=ProtocolError,
        expect_fields={"code": ErrorCode.ERR_HARDWARE},
    ),
]
