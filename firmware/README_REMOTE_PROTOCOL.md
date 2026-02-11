# VTTester remote protocol (binary) – firmware

## What was added

- **vttester_remote.h / vttester_remote.c**  
  Binary protocol layer: 8-byte frames, CRC-8, RX state machine (feed bytes from UART, poll for SET/MEAS events), build and queue SET ACK, MEAS ACK, MEAS RESULT. No float; safe to call `vttester_feed_byte()` from UART RX ISR.

- **vttester_setup.h / vttester_setup.c**  
  Decode SET frame into firmware parameters: heater (P1 → uhdef/ihdef), Ua (P2), Ug2 (P3), Ug1 (P4), warmup time (P5 → tuh). Validates ranges and returns OUT_OF_RANGE with param id and value for the ACK.

- **TTesterLCD32.c**  
  Integration: `vttester_init()` after LCD init; UART RX ISR feeds bytes with `vttester_feed_byte(UDR)`; main loop polls `vttester_poll()`, applies SET to `lamptem`/`tuh`/`ug1ref`, sends MEAS ACK and sets `remote_meas_trigger`; timer2_comp starts the measurement sequencer when `remote_meas_trigger` is set and, at capture phase, calls `vttester_send_meas_result()` and clears `remote_meas_pending`; main loop sends any queued TX frame (8 bytes) when `vttester_has_pending_tx()`.

Flow: **PC → SET** → FW applies params, sends **ACK**; **PC → MEAS** → FW sends **ACK (meas started)**, starts sequencer; when measurement finishes, FW sends **RESULT** (R1–R5 = IH, IA, IG2, S, alarm).

## Build

Add the new sources to your AVR project/makefile:

- `vttester_remote.c`
- `vttester_setup.c`

Include path must see `vttester_remote.h` and `vttester_setup.h` (same directory as the .c is enough if you compile from `firmware/`).

Example (gcc-avr):

```make
SRCS = TTesterLCD32.c vttester_remote.c vttester_setup.c
```

No extra libraries. Integer-only, no stdint requirement (uses `unsigned char`, `unsigned int`).

## Tech debt / design notes

- **Single TX queue**  
  Only one frame is queued at a time (SET ACK, MEAS ACK, or MEAS RESULT). The flow is sequential (SET→ACK, MEAS→ACK→RESULT), so this is enough. If you later add overlapping commands, consider a small ring of outgoing frames.

- **SET applies to current `lamptem`**  
  SET does not change `typ` or tube name; it only overwrites uhdef, ihdef, ug1def, uadef, ug2def and `tuh`. So the “current tube” on the LCD stays; only the measurement parameters are updated. Remote and local (encoder) share the same `lamptem`.

- **MEAS reuses the same sequencer as the button**  
  `remote_meas_trigger` causes the same `start = (tuh + ...)` and `zersrk()` as the encoder start. So one code path for both local and remote measurement.

- **Alarm bits**  
  Firmware `err` (OVERIH, OVERIA, OVERIG, OVERTE) is mapped to protocol R5 in the capture phase before calling `vttester_send_meas_result()`.

- **ESC and binary**  
  UART RX still sets `txen = 1` on ESC for the legacy ASCII dump. Binary frames are parsed in parallel; no conflict.

- **Optional cleanup in main**  
  The protocol block in the main loop could be moved to a function, e.g. `vttester_main_poll()`, to keep `while(1)` shorter. Same for the capture-phase block in timer2_comp.
