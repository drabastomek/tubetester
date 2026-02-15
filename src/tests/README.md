# Backend tests (pytest)

Unit tests for `backend.protocol` and `backend.api`. No serial I/O (API tests use a mocked `SerialDriver`).

## Run tests

From **repo root** (recommended; `pytest.ini` sets `pythonpath = src`):

```bash
pytest src/tests -v
```

From **src**:

```bash
cd src && PYTHONPATH=. pytest tests/ -v
```

## Dependencies

```bash
pip install -r requirements-dev.txt   # from src/ (adds pytest to requirements.txt)
# or: pip install pytest
```

## Layout

- **test_protocol.py** — `Protocol`: CRC-8, `verify_crc`, `build_set_frame`, `build_meas_frame`, `parse_response` (ack, measurement, error).
- **test_api.py** — `VTTesterClient` with mocked driver: `open`/`close`/`handshake`, `set_params`, `start_measurement`, callbacks, `read_status`/`reset_params` stubs, timeout handling, `_alarm_info`.
