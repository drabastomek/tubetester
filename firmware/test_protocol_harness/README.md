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
| `HARNESS_FLOW=auto` | split | `auto` → SET schedules DATA after `HARNESS_MEAS_DELAY_MS` |
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

## Desktop smoke test

With the harness flashed and USB-serial connected:

```bash
cd /path/to/tubetester
PYTHONPATH=src python -m backend.communication /dev/cu.YourSerialPort
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
