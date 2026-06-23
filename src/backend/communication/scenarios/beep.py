"""BEEP command → ACK (harness drives SPK on PORTD7)."""

from backend.communication.harness_client import Step
from backend.communication.protocol import BeepDuration, prepare_beep

BEEP_STEPS = [
    Step(
        name="BEEP default duration",
        tx=lambda: prepare_beep(BeepDuration.DEFAULT),
        expect="RSP_ACK",
        timeout_s=1.0,
    ),
    Step(
        name="BEEP 100 ms",
        tx=lambda: prepare_beep(10),
        expect="RSP_ACK",
        timeout_s=1.0,
    ),
]
