"""Invalid command byte → 3-byte ERROR (ERR_INVALID_CMD)."""

from backend.communication.harness_client import Step
from backend.communication.protocol import (
    ErrorCode,
    ProtocolError,
    prepare_raw_request,
)

INVALID_CMD_STEPS = [
    Step(
        name="invalid CMD 0x7F",
        tx=lambda: prepare_raw_request((0x7F, ord("0"), 0, 0, 0, 0, 0, 0, 0)),
        expect=ProtocolError,
        expect_fields={"code": ErrorCode.ERR_INVALID_CMD},
    ),
]
