# Firmware changes and structure

High-level summary of refactoring and the current layout. For main-loop flow see [MAIN_LOOP_WALKTHROUGH.md](MAIN_LOOP_WALKTHROUGH.md).

---

## 1. Summary of changes

### Protocol and main loop

- **Serial communication protocol:** VTTester protocol handling over the serial link (UART) was added and refactored into a dedicated `protocol/` module. The device receives 8-byte frames (SET parameters, MEAS request, etc.), parses them with CRC and limit checks, applies SET when in remote mode (typ==1), and sends ACK or measurement result frames back. See section 3 for parser and frame details.
- **Unified response path:** The main loop sends a single `parsed.err_code` (and for SET, optional error param/value) for every parsed frame; configurable SET limits live in `config/config.h`.
- **Ug1:** Stored as 0..240 in 0.1 V units (240 = -24 V); parser uses same convention as legacy (`ug1def = P4*5`).

### Display vs control split

- **Display:** All LCD output (hardware init, CGRAM, splash, and refreshing the 4×20 screen from `buf[]`) lives in `display/`. The main loop only calls `display_init()` once and `display_refresh()` each sync tick.
- **Control:** All menu, katalog, parameter editing, ADC→value calculations, and EEPROM/setpoint updates live in `control/`. The main loop calls `control_update(ascii)` each sync tick; control fills `buf[]` and updates globals (setpoints, `lamptem`, etc.).

### Utils and tests

- **int2asc:** Moved to `utils` for shared use; outputs 4 decimal digits in `ascii[0..3]`.
- **Host unit tests:** Suite in `test/` builds with `gcc` (no AVR), tests protocol, utils, display row builders, and control. Run with `make test` from the firmware directory.

### EEPROM and config

- **EEPROM macros:** `EEPROM_READ` / `EEPROM_WRITE` are defined in `config/config.h` (backed by avr-libc) so both main and control can use them without duplicating definitions.

---

## 2. Directory and file structure

```
firmware/
├── TTesterLCD32.c          # Main: init, ISRs, main loop (protocol dispatch, display_refresh, control_update, safety)
├── Makefile                 # AVR build; `make test` runs host tests
├── config/
│   ├── config.h             # Hardware/app config: port macros, timing, protocol limits, EEPROM macros, katalog type
│   └── config.c             # Tube catalog (lamprom), EEPROM data (lampeep, poptyp), AZ table, cyr* CGRAM
├── display/
│   ├── display.h            # display_init(), display_refresh(), LCD primitives (cmd2lcd, gotoxy, char2lcd, cstr2lcd, str2lcd)
│   └── display.c            # LCD port access, CGRAM load, splash, refresh from buf[] (rows 0–3 by typ)
├── control/
│   ├── control.h            # control_update(unsigned char *ascii)
│   └── control.c            # Tube number/name, name edit, Ug1/Uh/Ih/Ua/Ia/Ug2/Ig2/S/R/K: compute, fill buf[], encoder/EEPROM/setpoints; TX to PC
├── protocol/
│   ├── vttester_remote.h     # Frame length, cmd/error codes, parsed types, parse_message, send_response, send_measurement
│   └── vttester_remote.c     # CRC-8, parse 8-byte frame (SET/MEAS/NONE), build ACK and MEAS result frames
├── utils/
│   ├── utils.h               # char2rs, cstr2rs, delay, zersrk, liczug1, int2asc
│   └── utils.c               # UART/hardware helpers; under VTTESTER_HOST_TEST stubs for host tests, int2asc shared
├── test/                     # Host unit tests (no AVR)
│   ├── config/config.h       # Minimal config (VTTESTER_SET_* only) for protocol tests
│   ├── test_common.h         # TEST_ASSERT, counters
│   ├── test_control.c        # control_update (number/name, Ug1 from ADC, lamprom load)
│   ├── test_display.c        # display_build_row0..3 (LCD rows)
│   ├── test_protocol.c       # Parse (CRC, invalid cmd, MEAS, SET OK/OOB P1–P5, Ug1), send_response, send_measurement
│   ├── test_utils.c          # int2asc (0, 7, 42, 123, 9999, 300, 240, 255)
│   ├── main.c                # Test runner; exit 0/1
│   ├── Makefile              # Build test_runner, `make run` runs it
│   └── README.md             # How to run and extend tests
└── docs/
    ├── CHANGES_AND_STRUCTURE.md   # This file
    ├── MAIN_LOOP_WALKTHROUGH.md   # Main loop step-by-step
    └── AVR_SIZE_EXPLAINED.md      # Flash/RAM size notes
```

---

## 3. Role of each module

### TTesterLCD32.c (main)

- **Owns:** Global state (buf, adr, typ, setpoints, lamptem, lamprem, ADC/measurement state, protocol RX/TX buffers, etc.).
- **Does:** Port/timer/UART/watchdog init; ISRs (encoder, timer, UART RX/TX, ADC); main loop: on sync tick, run protocol handling (parse, apply SET when typ==1, send ACK/MEAS), call `display_refresh()`, re-enable INT1, safety check, then `control_update(ascii)`.
- **Does not:** Know LCD details (handled by display) or how buf[] is filled (handled by control).

### config/

- **config.h:** Single place for hardware and application constants: port/bit macros, timing (MS1, DMAX, …), ADC/alarm defines, catalog sizes (FLAMP, ELAMP), **VTTester protocol SET limits** (VTTESTER_SET_UA_MAX, VTTESTER_SET_UG1_P4_STEP, …), EEPROM_READ/EEPROM_WRITE, and the `katalog` type.
- **config.c:** ROM tube catalog (`lamprom` in PROGMEM), EEPROM-backed catalog (`lampeep`), `load_lamprom()`, `poptyp`, AZ lookup table, and cyr* CGRAM data for the LCD.

### display/

- **display.c:** Low-level LCD: `cmd2lcd`, `gotoxy`, `char2lcd`, `cstr2lcd`, `str2lcd`; `display_init()` (LCD hw init, CGRAM, splash); `display_refresh()` (draw rows 0–3 from main’s `buf[]`, with row 3 depending on typ: local S/R/K or T=, or power-supply blank / remote “TX/RX: 9600,8,N,1”).
- **display.h:** Declares the above. Display uses main’s globals (buf, adr, typ, start, err, dusk0, takt) via extern; it does not own state.

### control/

- **control.c:** One function `control_update(ascii)`. Uses main’s globals (buf, adr, typ, setpoints, lamptem, lamprem, ADC values, etc.) to: refresh tube number/name (adr==0), handle name editing (typ>=FLAMP), then for each parameter (Ug1, Uh, Ih, Ua, Ia, Ug2, Ig2, S, R, K) compute value from ADC or measurement, write ASCII into buf[], and on encoder confirm write setpoints or EEPROM. Also sends to PC when `txen` is set (10-byte legacy or 63-char dump). Uses `int2asc(..., ascii)` from utils.
- **control.h:** Declares `control_update(unsigned char *ascii)`.

### protocol/

- **vttester_remote.c:** CRC-8 (table in PROGMEM on AVR), `vttester_parse_message()` (validates CRC and cmd byte, decodes SET with config limits, sets `err_code` and SET fields/error_param/error_value), `vttester_send_response()` (ACK with optional OUT_OF_RANGE param/value), `vttester_send_measurement()` (MEAS result frame). Uses `config/config.h` for VTTESTER_SET_* only.
- **vttester_remote.h:** Frame length, command and error codes, `vttester_parsed_t` / `vttester_set_params_t`, and the three API functions.

### utils/

- **utils.c:** UART send (`char2rs`, `cstr2rs`), `delay`, `zersrk`, `liczug1` (Ug1 0..240 → hardware setpoint), `int2asc` (integer → 4 ASCII digits). When built with `-DVTTESTER_HOST_TEST`, AVR code is excluded and only stubs + `int2asc` are compiled for host tests.
- **utils.h:** Declares all of the above.

### test/

- **Purpose:** Host-only tests; no AVR toolchain. Builds protocol (with test config) and utils (with host stubs), runs all test cases, exits 0 if all pass else 1.
- **test/config/config.h:** Defines only VTTESTER_SET_* so protocol compiles without AVR headers.
- **test_protocol.c:** Parse (CRC, invalid cmd, MEAS, SET valid, SET P1–P5 out-of-range, Ug1 range), send_response (OK and OUT_OF_RANGE), send_measurement layout and CRC.
- **test_utils.c:** int2asc for 0, 7, 42, 123, 9999, 300, 240, 255.
- **test_display.c:** display_build_row0..3 (LCD row content).
- **test_control.c:** control_update (tube number/name, Ug1 from ADC, lamprom load).
- **main.c:** Calls `run_protocol_tests()`, `run_utils_tests()`, `run_display_tests()`, `run_control_tests()`, prints result, returns 0/1.

---

## 4. Data flow (simplified)

1. **UART RX** (ISR) → 8-byte `rx_proto_buf` → `rx_proto_ready = 1`.
2. **Main loop (sync):** If `rx_proto_ready`, parse with `vttester_parse_message(rx_proto_buf, &parsed)`. If SET and typ==1 and no error, apply parsed params to setpoints/lamptem; then send ACK with `parsed.err_code` (+ error_param/error_value for SET). If MEAS, send OK and set measurement trigger. If NONE, send `parsed.err_code`.
3. **Display:** `display_refresh()` reads main’s `buf[]` and typ/adr/start/err and draws the 4×20 LCD (no modification of buf).
4. **Control:** `control_update(ascii)` computes values from ADC/state, fills `buf[]`, and on encoder/confirm updates setpoints or EEPROM; also handles TX to PC when `txen` is set.
5. **Measurement:** When a measurement finishes, main (or timer logic) fills result and calls `vttester_send_measurement()`; the 8-byte frame is sent via UART.

So: **protocol** parses incoming frames and builds outgoing ACK/MEAS; **display** only draws from buf; **control** owns the content of buf and setpoints/EEPROM.

---

## 5. Build and test

- **Firmware (AVR):** `make` in `firmware/` → `TTesterLCD32.hex`. `make clean`, `make size` as usual.
- **Host tests:** `make test` in `firmware/` (or `make run` in `firmware/test/`) → compile and run test_runner; “OK: N tests” and exit 0 means all passed.

After any change to protocol or utils, run `make test` to confirm behaviour.
