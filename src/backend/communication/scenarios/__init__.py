from backend.communication.scenarios.bad_crc import BAD_CRC_STEPS
from backend.communication.scenarios.beep import BEEP_STEPS
from backend.communication.scenarios.fault_alarm import FAULT_ALARM_STEPS
from backend.communication.scenarios.fault_hardware import FAULT_HARDWARE_STEPS
from backend.communication.scenarios.fault_silent import FAULT_SILENT_STEPS
from backend.communication.scenarios.invalid_cmd import INVALID_CMD_STEPS
from backend.communication.scenarios.nominal import NOMINAL_STEPS
from backend.communication.scenarios.out_of_range import OUT_OF_RANGE_STEPS

SCENARIOS = {
    "nominal": NOMINAL_STEPS,
    "out_of_range": OUT_OF_RANGE_STEPS,
    "invalid_cmd": INVALID_CMD_STEPS,
    "bad_crc": BAD_CRC_STEPS,
    "beep": BEEP_STEPS,
    "fault_alarm": FAULT_ALARM_STEPS,
    "fault_silent": FAULT_SILENT_STEPS,
    "fault_hardware": FAULT_HARDWARE_STEPS,
}

__all__ = ["SCENARIOS"]
