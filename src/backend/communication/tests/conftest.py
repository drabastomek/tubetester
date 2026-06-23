import os

import pytest


def pytest_configure(config):
    config.addinivalue_line(
        "markers",
        "hardware: requires ATmega32 harness on serial (set VTTESTER_PORT)",
    )


@pytest.fixture
def harness_port():
    port = os.environ.get("VTTESTER_PORT")
    if not port:
        pytest.skip("VTTESTER_PORT not set")
    return port
