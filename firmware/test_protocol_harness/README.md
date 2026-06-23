# Protocol test harness firmware (ATmega32A)

UART stand-in for desktop protocol testing. Uses the shared `communication/` module and synthetic ADC sums instead of real measurements.

See [OUTLINE.md](OUTLINE.md) for the full test plan.

## Build

From the repo:

```bash
cd firmware
make
```

Output: `firmware/bin/comms_test_avr.hex`

### Build options (Makefile variables)

| Variable | Default | Effect |
|----------|---------|--------|
| `HARNESS_FLOW=auto` | split | `auto` → SET emits DATA after delay (no ACK) |
| `HARNESS_MEAS_DELAY_MS=50` | 50 | Auto-measure delay (only with `HARNESS_FLOW=auto`) |
| `HARNESS_TX_DELAY_MS=0` | 0 | Milliseconds to wait before each TX reply |
| `COMMS_DEBUG_BOOT=1` | off | Reserved hook (no output yet) |

Examples:

```bash
make HARNESS_FLOW=auto HARNESS_MEAS_DELAY_MS=100
make HARNESS_TX_DELAY_MS=20
```

## Flash

USBasp (default in Makefile):

```bash
make flash
# or slower ISP clock:
make flash-slow
```

MCU: **ATmega32A**, 16 MHz, **9600 8N1** (same as production tester).

## Desktop measurement flow

Production sequence (SET → wait 125 ms → STATUS stability loop) is documented in  
[`src/backend/MEASUREMENT_FLOW.md`](../../src/backend/MEASUREMENT_FLOW.md).

Use **split-mode** harness (`make` default) — desktop owns timing, not the MCU.

## Desktop tests

**Tier A** (no hardware):

```bash
pytest -m "not hardware"
```

**Tier B** (harness flashed, USB-serial connected):

```bash
export VTTESTER_PORT=/dev/cu.usbserial-0001
pytest -m hardware                      # all scenarios
pytest -m hardware --scenario beep      # one scenario

PYTHONPATH=src python -m backend.communication.harness_client $VTTESTER_PORT --scenario beep
```

**Auto-measure** (optional, not production flow — see MEASUREMENT_FLOW.md):

```bash
make -C firmware HARNESS_FLOW=auto HARNESS_MEAS_DELAY_MS=100 flash
export VTTESTER_RUN_AUTO_MEASURE=1
pytest -m hardware src/backend/communication/tests/test_auto_measure_serial.py
```

## Layout

| File | Role |
|------|------|
| `main.c` | Init, main loop, UART ISR |
| `harness_command.c` | SET / RESET / STATUS dispatch and validation |
| `synthetic_measure.c` | Mutable ADC sum model |
| `scenario.c` | Auto-measure timing (split vs delayed DATA) |
| `harness_tick.c` | 1 ms Timer0 tick |
| `harness_config.h` | Compile-time harness options |

Shared (not in this folder): `firmware/communication/`, `firmware/config/`.

## Legacy entry point

`firmware/TTesterLCD32.c` was the original comms-test main; the harness build now uses `test_protocol_harness/main.c` instead.
