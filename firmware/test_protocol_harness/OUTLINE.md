# VTTester protocol test harness — conceptual outline

Desktop-driven end-to-end testing of the v0.5 wire protocol against firmware (or a firmware stand-in), without needing the full tube-tester measurement stack.

---

## 1. Goal

Exercise the **whole** PC ↔ firmware protocol from the desktop side:

- All command types (SET, RESET, STATUS)
- All response types (ACK, DATA, ERROR, OUT_OF_RANGE, ALARM, USER_BREAK)
- CRC validation and failure paths
- Parameter validation (in-range / out-of-range)
- Timing: delays, timeouts, “busy” or late responses
- Multi-step flows (SET → measure → DATA, reset sequences, error recovery)

The harness should be **deterministic** and **scriptable** so regressions are caught before flashing production firmware or running the GUI.

---

## 2. What already exists

| Layer | Location | Role today |
|-------|----------|------------|
| Protocol spec | `firmware/protocol/VTTester_Protocol_v0.5.txt`, `CHANGELOG.md` | Normative wire format |
| Shared comms module | `firmware/communication/` | CRC-8, RX framing (10 B), TX helpers (ACK / ERROR / OOR / ALARM / DATA) |
| Minimal MCU main | `firmware/TTesterLCD32.c` | UART ISR, `handle_rx_msg()` for SET / RESET / STATUS, synthetic ADC via `sample_adc()` |
| Harness build | `firmware/Makefile` → `comms_test_avr` | ATmega32A @ 9600 8N1, `communication/` + `config/` only |
| Desktop codec | `src/backend/communication/protocol.py` | `prepare_request()`, `parse_response()`, CRC table (same as firmware) |
| Desktop serial client | `src/backend/communication/mcu_serial.py` | `VTTesterSerial`: send SET/RESET/STATUS, read one response |
| Manual smoke test | `python -m backend.communication <port>` | SET → STATUS → RESET once |

This is a solid **spine**: shared CRC/framing, a flashable stub MCU, and a Python client. It is **not yet** a full protocol test system.

---

## 3. Gaps to close (spec ↔ code ↔ tests)

Before calling coverage “whole protocol”, align and then test these items:

### 3.1 Wire format alignment

| Topic | v0.5 spec | Current firmware / desktop |
|-------|-----------|----------------------------|
| Request | 10 B, CRC @ byte 9 | Matches (`FRAME_RX_BYTES = 10`) |
| DATA response | **20 B** (18 payload + CRC) incl. **ERR** @ byte 18 | **19 B** (`FRAME_TX_DATA = 19`), **no ERR byte** |
| DATA field order | Ug, Uh, Ih, Ua, Ia, Ue, Ie, Tm | Harness STATUS sends Uh, Ih, Ug, Ua, Ia, Ue, Ie, Tm |
| CRC error on RX | Discard frame, **no** reply | Firmware **sends** `VTT_ERR_CRC` ERROR frame |
| SET semantics | Request template + measurement | Harness: SET → ACK only; DATA only on explicit STATUS |
| OUT_OF_RANGE reply | Spec silent; implementation uses 5 B ERROR | Desktop `read_response()` does not fully parse 5 B OOR yet |

Decide explicitly: harness tracks **spec** or **current `communication.c` API** during bring-up, then converge both toward v0.5.

### 3.2 Behaviors not exercised today

- `send_alarm()`, `send_user_break()`
- RESET sub-kinds (`RESET_HEATER` vs `RESET_FULL`) effect on later STATUS
- Invalid CMD (`VTT_ERR_INVALID_CMD`)
- Every `PARAM_ID_*` out-of-range path
- Inter-byte / inter-frame **delays**
- **Timeout** when firmware sends nothing or sends garbage
- Back-to-back commands without `drain()`
- Partial frames / UART noise (desktop resilience)

### 3.3 SET vs measurement flow

Production firmware will likely: accept SET → run acquisition → emit DATA (possibly with delay). The stub today splits that into SET (ACK) + STATUS (DATA). The harness should support **both modes**:

1. **Split mode** (current) — good for unit-testing command parsing and STATUS encoding.
2. **Auto-measure mode** — SET schedules DATA after `MEAS_DELAY_MS`; good for desktop timeout/retry tests.

---

## 4. Proposed architecture

```
┌─────────────────────────────────────────────────────────────────┐
│  Desktop test runner (Python, pytest or CLI scenarios)        │
│  src/backend/communication/  (+ future tests/ or harness CLI) │
│  - build frames, assert responses, timeouts, golden vectors     │
└────────────────────────────┬────────────────────────────────────┘
                             │ USB-UART 9600 8N1
                             │ (or SimAVR UART pipe)
┌────────────────────────────▼────────────────────────────────────┐
│  Firmware protocol harness (ATmega32A)                          │
│  firmware/test_protocol_harness/  (evolves from TTesterLCD32)   │
│                                                                 │
│  ┌──────────────┐   ┌─────────────────┐   ┌──────────────────┐  │
│  │ comm layer   │   │ scenario /      │   │ synthetic        │  │
│  │ (shared      │◄──│ fault injection │◄──│ measurement      │  │
│  │ communication│   │ (delays, alarms)│   │ model (ADC sums) │  │
│  └──────────────┘   └─────────────────┘   └──────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

**Principle:** keep `firmware/communication/` as the single framing/CRC API; put **harness-only** policy (fake ADC, delays, fault injection) in `test_protocol_harness/`, not in production `TTesterLCD*.c`.

---

## 5. Firmware harness — recommended structure

```
firmware/test_protocol_harness/
  OUTLINE.md              ← this document
  README.md               ← how to build, flash, run desktop tests (when implemented)
  main.c                  ← UART ISR + main loop (from TTesterLCD32.c)
  harness_config.h        ← limits, default delays, build flags
  synthetic_measure.c/h   ← sample_adc(), settable sums, ERR bit builder
  scenario.c/h            ← optional: table-driven responses / fault modes
  scenarios/              ← optional: named scenario descriptors (C tables or headers)
```

**Build:** either extend `firmware/Makefile` with `TARGET=harness_avr` and alternate `main.c`, or a small `test_protocol_harness/Makefile` that includes the same `communication/` + `config/` objects.

### 5.1 Synthetic measurement model

Replace fixed `sample_adc()` with a **mutable model**:

- Settable uint16 sums per channel (Ug, Uh, Ih, Ua, Ia, Ue, Ie, Tm)
- Optional: derive sums from last accepted SET (e.g. UA setpoint × 64) for closed-loop checks
- `ERR` byte builder: RNG200, OVERTM, OVERIE, OVERIA, OVERIH
- Presets: `nominal`, `all_alarms`, `zero`, `saturated`

Desktop tests then assert **exact** raw sums and ERR bits, not only “got DATA”.

### 5.2 Command handler (harness policy)

| CMD | Harness behavior (test modes) |
|-----|-------------------------------|
| **SET** | Validate → ACK; optionally start non-blocking timer → auto DATA |
| **RESET** | Apply reset model to setpoints / alarms → ACK |
| **STATUS** | Immediate DATA from synthetic model |
| **invalid** | `VTT_ERR_INVALID_CMD` |
| **bad CRC** | Per spec: silent discard **or** legacy ERROR — selectable compile flag for migration tests |

### 5.3 Fault and timing injection

Controlled via **compile-time flags** and/or a **harness-only SET extension** (e.g. magic bytes in reserved fields, or a dedicated CMD only in `HARNESS_BUILD`):

| Injection | Purpose |
|-----------|---------|
| `TX_DELAY_MS` before any reply | Desktop read timeout tuning |
| `TX_DELAY_MS` between bytes | Framing stress |
| Drop / corrupt TX CRC | Desktop CRC failure handling |
| Send ALARM instead of ACK on SET | Alarm path |
| Send USER_BREAK mid-sequence | Abort path |
| Ignore next N commands | Recovery / drain tests |
| Stuck BUSY (no reply until STATUS) | Polling pattern (if reintroduced) |

Keep injection **off** in default `nominal` image; one flash (or UART “arm fault N”) per fault test.

### 5.4 Non-blocking delays on AVR

Use a tick counter (Timer0 or loop-based `millis()` helper) in the main loop:

```text
on SET:  state = WAIT_MEAS; deadline = now + MEAS_DELAY_MS
loop:    if WAIT_MEAS && now >= deadline → send_data(); state = IDLE
         comm_rx_msg() → handle as today
```

ISRs stay thin (RX byte only); delays run in foreground — same pattern as production.

---

## 6. Desktop test runner — recommended structure

```
src/backend/communication/
  protocol.py           ← existing codec
  mcu_serial.py           ← existing link
  harness_client.py       ← optional: drain, send_raw, read_with_timeout, hex dump
  scenarios/
    nominal_set_status.py
    crc_bad_request.py
    out_of_range_ug.py
    ...
  tests/
    test_protocol_vectors.py   ← CRC + prepare_request golden bytes (no hardware)
    test_harness_serial.py     ← integration, marked @pytest.mark.hardware
```

### 6.1 Two test tiers

| Tier | Needs hardware | What it proves |
|------|----------------|----------------|
| **A — codec / vectors** | No | CRC, frame sizes, parse errors, golden request bytes |
| **B — serial integration** | Yes (harness MCU or SimAVR) | Full handshake, delays, faults, state after RESET |

Mark tier B `pytest -m hardware` so CI runs tier A only.

### 6.2 Scenario shape (conceptual)

Each scenario is a list of steps:

```python
@dataclass
class Step:
    tx: bytes | Callable[[], bytes]   # request frame
    expect: type | str | None        # "ACK", Data, ERROR, timeout
    expect_fields: dict | None       # e.g. {"ug_sum": 1286, "err": 0x01}
    timeout_s: float = 0.5
    delay_before_tx_s: float = 0.0
```

Runner: for each step, optional sleep → send → read with timeout → assert.

### 6.3 Coverage matrix (minimum)

**Requests**

- [ ] SET valid (each field at min, max, mid)
- [ ] SET each out-of-range param (UG, UH, IH, UA, UE) → 5 B OOR ERROR
- [ ] SET bad CRC → no action or ERROR (per chosen policy)
- [ ] RESET heater-only vs full
- [ ] STATUS after SET / after RESET
- [ ] Invalid CMD byte
- [ ] Truncated / extra UART bytes (desktop only)

**Responses**

- [ ] ACK (2 B) CRC ok
- [ ] ERROR (3 B) generic
- [ ] ERROR (5 B) OUT_OF_RANGE + param id + value
- [ ] DATA (19/20 B per aligned spec) all channels + ERR
- [ ] ALARM (3 B)
- [ ] USER_BREAK (2 B)

**Flows**

- [ ] SET → ACK → STATUS → DATA → RESET → ACK → STATUS reflects reset
- [ ] SET → (delay) → auto DATA (auto-measure mode)
- [ ] SET out-of-range → no state change on subsequent STATUS
- [ ] Rapid SET SET STATUS (queue / drop policy)

**Timing**

- [ ] Response within `timeout_s`
- [ ] No response when harness configured silent (timeout path)
- [ ] Delayed DATA still parses correctly

---

## 7. Execution environments

### 7.1 Hardware loop (primary)

1. Build and flash `comms_test_avr` / `harness_avr` to ATmega32A.
2. Connect USB-UART (9600 8N1, same as `config.h` `RATE 103`).
3. Run `PYTHONPATH=src pytest src/backend/communication/tests -m hardware --port /dev/cu.…`.

### 7.2 SimAVR (optional)

Your `simavr` workspace can run the same `.elf` with a UART backend:

- Deterministic runs without hardware
- Easier to automate in CI later
- Same desktop scenarios; only the transport string changes (`/tmp/simavr-uart` or similar)

Worth a thin wrapper script once harness is stable on real hardware.

### 7.3 Manual smoke (keep)

`python -m backend.communication <port>` remains the quick human check before scripted suites.

---

## 8. Suggested implementation phases

| Phase | Deliverable | Outcome |
|-------|-------------|---------|
| **0 — Align** | Fix DATA size/order/ERR to match v0.5; sync `protocol.py` | Single source of truth for vectors |
| **1 — Extract harness** | Move `TTesterLCD32.c` logic → `test_protocol_harness/main.c`; Makefile target | Clear separation from production firmware |
| **2 — Synthetic model** | Settable ADC sums + ERR presets | Predictable DATA assertions |
| **3 — Desktop tier A** | pytest vectors, no serial | CRC/regression in CI |
| **4 — Scenario runner** | `harness_client.py` + 3–5 golden flows | Repeatable hardware smoke |
| **5 — Fault injection** | Delays, bad CRC TX, ALARM path | Error and timing coverage |
| **6 — Auto-measure mode** | SET triggers delayed DATA | Production-like flow |

---

## 9. Open decisions (pick before heavy coding)

1. **CRC mismatch:** spec says discard silently; firmware today replies ERROR — migrate firmware to spec, or test both during transition?
2. **DATA frame length:** 19 vs 20 bytes — treat spec as target; update `FRAME_TX_DATA`, `send_data()`, and `Data.from_bytes()` together.
3. **Field order in DATA:** spec order vs current harness order — breaking change for any captured traces.
4. **OUT_OF_RANGE on desktop:** extend `read_response()` to parse 5 B ERROR and return structured `OutOfRangeError(param_id, value)`.
5. **Harness-only commands:** magic SET bytes vs `#ifdef HARNESS` CMD values for fault injection — avoid polluting production opcode space.
6. **ACK on SET vs SET→DATA only:** v0.5 informative flow is request → one DATA frame; ACK/STATUS may be legacy from bring-up — document **normative** desktop expectation.

---

## 10. Success criteria

The harness effort is “done enough” when:

- A single command runs **tier A + tier B** suites against a flashed harness board.
- Every row in §6.3 is either **passing** or **explicitly skipped** with a tracked reason.
- Desktop GUI can point at the same `VTTesterSerial` / protocol types without a parallel wire format.
- Failures print **hex dumps** of TX/RX and the scenario step name — not only “CRC mismatch”.

---

*Document: `firmware/test_protocol_harness/OUTLINE.md`*  
*Protocol reference: `firmware/protocol/VTTester_Protocol_v0.5.txt`*  
*Related: `firmware/communication/`, `firmware/TTesterLCD32.c`, `src/backend/communication/`*
