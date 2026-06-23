"""Bad CRC on request → firmware discards silently (v0.5 §3)."""

from backend.communication.harness_client import Step
from backend.communication.protocol import (
    CommandCode,
    corrupt_crc,
    prepare_request,
)

BAD_CRC_STEPS = [
    Step(
        name="SET with corrupted CRC",
        tx=lambda: corrupt_crc(
            prepare_request(
                CommandCode.CMD_SET,
                a1_a2="0",
                ug=100,
                uh=63,
                ua=300,
            )
        ),
        expect=None,
        timeout_s=0.25,
    ),
]
