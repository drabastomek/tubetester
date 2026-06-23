"""Root pytest hooks (must live beside pytest.ini for --scenario)."""

import pytest


def pytest_addoption(parser):
    parser.addoption(
        "--scenario",
        action="store",
        default=None,
        help="Run one harness scenario by name (default: all hardware scenarios)",
    )


def pytest_generate_tests(metafunc):
    if "scenario_name" not in metafunc.fixturenames:
        return

    from backend.communication.scenarios import SCENARIOS

    names = sorted(SCENARIOS)
    chosen = metafunc.config.getoption("--scenario")
    if chosen:
        if chosen not in SCENARIOS:
            raise pytest.UsageError(
                f"unknown scenario {chosen!r}; choose from: {', '.join(names)}"
            )
        names = [chosen]
    metafunc.parametrize("scenario_name", names)
