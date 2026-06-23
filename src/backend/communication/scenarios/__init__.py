from backend.communication.scenarios.nominal import NOMINAL_STEPS
from backend.communication.scenarios.out_of_range import OUT_OF_RANGE_STEPS

SCENARIOS = {
    "nominal": NOMINAL_STEPS,
    "out_of_range": OUT_OF_RANGE_STEPS,
}

__all__ = ["SCENARIOS"]
