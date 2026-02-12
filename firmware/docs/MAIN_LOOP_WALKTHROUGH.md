# Main loop walkthrough

The main loop in `TTesterLCD32.c` (from ~line 802) runs forever. **Real work happens in two places:** (1) when **`sync == 1`** (set by the timer ISR at a fixed rate, e.g. ~4× per second) — protocol handling and LCD refresh; (2) **every iteration** — INT1 re-enable, safety check, and **`control_update(ascii)`**, which fills `buf[]` and handles encoder/EEPROM/setpoints.

So each “tick” when `sync == 1`: watchdog kick → protocol (parse, SET/MEAS, send ACK or measurement) → `display_refresh()` → then always: re-enable INT1, safety, `control_update(ascii)`.

**Modes (by `typ`):**
- **Power supply:** `typ == 0` — protocol can set parameters and read measurements.
- **Remote:** `typ == 1` — 8‑byte protocol SET/MEAS; setpoints and `lamptem` come from parsed SET.
- **Local:** `typ > 1` — tube from catalog/EEPROM; encoder/button control; protocol frames discarded (ESC still triggers LCD dump).

---

## 1. Loop entry (802–806)

```c
while( 1 )
{
   WDR;                           // Kick watchdog
   if ( sync == 1 )
   {
      sync = 0;
```

- **WDR** — Reset watchdog so the MCU doesn’t reset.
- **sync** — Set to 1 by the timer ISR (TIMER2_COMP) every N ms. When 1, we run protocol and display refresh, then clear it.

If `sync == 0`, the block below is skipped; the loop still runs GICR, safety, and `control_update(ascii)` at the end.

---

## 2. Protocol handling (809–876) — **Power supply & Remote only (`typ <= 1`)**

**When `typ <= 1`:**

- **2a. Incoming 8‑byte frame (`rx_proto_ready`)**  
  - Parse with `vttester_parse_message(rx_proto_buf, &parsed)`. The parser (in `protocol/vttester_remote.c`) fills `parsed.err_code` and for SET: `parsed.set.error_param` / `error_value` when out-of-range.
  - **SET:** If **typ == 1** and `parsed.set.error_param == 0`: apply parsed params to `lamprem`/`lamptem` and setpoints (`uhset`, `ug1set`, `uaset`, `ug2set`, `tuh`). Then send ACK with `vttester_send_response(...)` and send the 8 bytes out UART.
  - **MEAS:** Send OK via `vttester_send_response(...)`, set `remote_meas_index`, `remote_meas_pending`, `remote_meas_trigger` so the timer/ADC sequence will later set `remote_meas_ready` and we send the result.
  - **NONE (parse error):** Send ACK with `parsed.err_code`, 0, 0.
  - In all cases the 8‑byte response is sent out UART.

- **2b. Measurement result ready (`remote_meas_ready`)**  
  - Build alarm bits from `err`, call `vttester_send_measurement(...)`, send the 8 bytes, clear `remote_meas_ready` and `remote_meas_pending`.

**When `typ > 1` (local):**

- **2c.** Any received 8‑byte frame is discarded: `if( rx_proto_ready ) rx_proto_ready = 0;`. No parsing, no responses. ESC (handled in UART ISR) still triggers LCD dump via `txen`.

---

## 3. LCD update — `display_refresh()` (879)

**Only when `sync == 1`.** This call is the only place the LCD is refreshed; the implementation lives in **`display/display.c`**.

- **display_refresh()** reads main’s global `buf[]`, `adr`, `typ`, `start`, `err`, `dusk0`, `takt` and draws the 4×20 screen:
  - Row 0: tube number (`buf[0..2]`), name (`buf[4..12]`), Ug1 (`buf[24..27]`).
  - Row 1: Uh, Ua, Ug2.
  - Row 2: Error char (H/A/G/T), Ih, Ia, Ig2.
  - Row 3: For **local (`typ > 1`):** S=, R=, then K= or T= countdown; for **power supply / remote:** blank (typ==0) or “TX/RX: 9600,8,N,1” (typ==1).

So the display module **only displays** what is already in `buf[]`. It does not compute values or handle encoder; that is done in **control**.

---

## 4. After the `sync` block — INT1 and safety (881–882)

These run **every iteration** (not only when sync was 1):

- **GICR = BIT(INT1)** — Re‑enable external interrupt (encoder/button).
- **Emergency off** — If `err != 0` and we’re in an unsafe part of the sequence, force `stop = 1` and set `start` to the safe shutdown point.

---

## 5. Control — `control_update(ascii)` (885)

**Every iteration.** The implementation is in **`control/control.c`**; one function does all of the following:

- **Tube number and name (adr == 0):** Write `typ` into `buf[0..2]`. Load name from catalog or EEPROM into `lamptem`, set `tuh`, fill `buf[4..12]`. For typ 0/1 set `buf[11]` from anode.
- **Name editing (typ >= FLAMP):** When `adr` is one of the name positions, copy name to `buf[4..12]`, handle debounce, write back one character to EEPROM on confirm.
- **Per-parameter (Ug1, Uh, Ih, Ua, Ia, Ug2, Ig2, S, R, K):**  
  - Compute current value from ADC (or from last measurement if `start == (FUH+2)`).  
  - Convert to ASCII and write into `buf[]` at the fixed offsets (e.g. Ug1 → buf[24..27], Uh → buf[14..17], …).  
  - If this parameter is the selected field (`adr`), handle encoder: when `dusk0 == DMAX` copy default into display value and set `zapisz = 1`; when leaving (`nodus == DMIN` and `zapisz == 1`) write to setpoints (typ==0) or EEPROM (typ >= FLAMP) and clear `zapisz`.
- **TX to PC (`txen`):** Save `typ` to EEPROM; send 10‑byte legacy frame (typ==1) or 63‑char ASCII dump (typ != 1).

So: **protocol** (in main) parses frames and sends ACK/MEAS; **display** only draws from `buf[]`; **control** owns the content of `buf[]`, setpoints, and EEPROM updates.

---

## 6. End of loop (886–888)

- Closing brace of `while(1)`.
- Next iteration: WDR, then either skip the `sync` block (if sync==0) or run protocol + display_refresh; then always GICR, safety, control_update.

---

## Summary by mode

| Step              | Power supply (typ==0)     | Remote (typ==1)            | Local (typ>1)              |
|-------------------|---------------------------|----------------------------|-----------------------------|
| Protocol (2)      | Yes: SET/MEAS, responses  | Yes: SET/MEAS, responses   | No: discard 8‑byte frames   |
| LCD refresh (3)   | Yes (when sync)          | Yes (when sync)           | Yes (when sync)             |
| INT1 + safety (4) | Every iteration           | Every iteration            | Every iteration             |
| control_update (5)| Every iteration           | Every iteration            | Every iteration             |

So: **protocol is only active when typ <= 1**. Display runs only on sync ticks; control runs every iteration and fills `buf[]` and handles encoder/EEPROM/setpoints/TX.
