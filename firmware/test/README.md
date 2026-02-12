# Firmware unit tests (host)

Host-based tests (no AVR). Run after any protocol or utils change.

## Run

From firmware directory:
```bash
make test
```

Or from this directory:
```bash
make run
```

## What is tested

- **Protocol** (`test_protocol.c`): `vttester_parse_message` (CRC error, invalid cmd, MEAS, SET valid, SET P1–P5 out-of-range, Ug1 0/240), `vttester_send_response` (OK and OUT_OF_RANGE layout), `vttester_send_measurement` (frame layout and CRC).
- **Utils** (`test_utils.c`): `int2asc` (0, one digit, 42, 123, 9999, 300, 240, 255).

## Build

Tests use a minimal `config/config.h` in this directory (no AVR). Protocol is compiled with that config. Utils is compiled with `-DVTTESTER_HOST_TEST` so hardware-dependent functions are stubbed and only `int2asc` is exercised.

## Adding tests

1. Add cases in `test_protocol.c` or `test_utils.c` using `TEST_ASSERT`, `TEST_ASSERT_EQ`, or `TEST_ASSERT_MEM_EQ` from `test_common.h`.
2. Call the new test from `run_protocol_tests()` or `run_utils_tests()`.
