"""Arm ALARM-on-SET fault via harness STATUS 'H' command."""

from backend.communication.harness_client import Step
from backend.communication.protocol import (
    CommandCode,
    HARNS_FAULT_ALARM_ON_SET,
    prepare_harness_arm,
    prepare_request,
)

FAULT_ALARM_STEPS = [
    Step(
        name="arm ALARM-on-SET fault",
        tx=lambda: prepare_harness_arm(HARNS_FAULT_ALARM_ON_SET),
        expect="RSP_ACK",
    ),
    Step(
        name="SET triggers RSP_ALARM",
        tx=lambda: prepare_request(
            CommandCode.CMD_SET,
            a1_a2="0",
            ug=100,
            uh=63,
            ua=300,
        ),
        expect="RSP_ALARM",
    ),
]
