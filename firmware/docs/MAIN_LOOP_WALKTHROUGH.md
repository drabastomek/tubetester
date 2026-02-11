# Main loop walkthrough (from line 878)

The main loop runs forever. **It only does real work when `sync == 1`** (set by the timer ISR at a fixed rate, e.g. ~4× per second). So each “tick” is: watchdog kick → if sync, do one full refresh and logic pass → repeat.

**Modes (by `typ`):**
- **Power supply:** `typ == 0` — protocol can set parameters and read measurements dynamically.
- **Remote:** `typ == 1` — same as power supply from FW perspective; 8‑byte protocol SET/MEAS.
- **Local:** `typ > 1` — tube type selected from catalog/EEPROM; encoder/button control; protocol ignored except ESC for LCD dump.

---

## 1. Loop entry (878–883)

```c
while( 1 )
{
   WDR;                           // Kick watchdog
   if ( sync == 1 )
   {
      sync = 0;
```

- **WDR** — Reset watchdog so the MCU doesn’t reset.
- **sync** — Set to 1 by the timer ISR (TIMER2_COMP) every N ms. When 1, we run one “frame” of display + logic and clear it.

If `sync == 0`, the loop does nothing else this iteration (just spins and kicks WDR).

---

## 2. Protocol handling (885–947) — **Power supply & Remote only**

**Only when `typ <= 1` (power supply or remote):**

- **2a. Incoming 8‑byte frame (`rx_proto_ready`)**  
  - Parse with `vttester_parse_message(rx_proto_buf, &parsed)`. The parser fills `parsed.err_code` (OK, OUT_OF_RANGE, CRC, INVALID_CMD, UNKNOWN) and for SET also `parsed.set.error_param` / `error_value` when out-of-range.
  - **SET:**  
    - If **typ == 1 (remote)** and `parsed.set.error_param == 0`: apply parsed params to `lamprem` / `lamptem` and setpoints (`uhset`, `ug1set`, `uaset`, `ug2set`, `tuh`).  
    - Send ACK with `parsed.err_code`, `parsed.set.error_param`, `parsed.set.error_value` (one response; no branching on error type).  
  - **MEAS:** Send OK (err_code), set `remote_meas_index`, `remote_meas_pending`, `remote_meas_trigger` so the timer will start the measurement sequence and later send the result.
  - **NONE (parse error):** Send ACK with `parsed.err_code`, 0, 0.
  - In all cases send the 8‑byte response out UART.

- **2b. Measurement result ready (`remote_meas_ready`)**  
  - Build alarm bits from `err`, call `vttester_send_measurement(...)`, send the 8 bytes, clear `remote_meas_ready` and `remote_meas_pending`.

**When `typ > 1` (local):**

- **2c. Local**  
  - Any received 8‑byte frame is discarded: `if( rx_proto_ready ) rx_proto_ready = 0;`. No parsing, no responses. Only ESC (handled in UART ISR) triggers LCD dump.

---

## 3. LCD update — “Wyswietlenie zawartosci bufora” (948–1032)

**All modes.** This block only **refreshes the LCD** from the current `buf[]` and `adr` (which field is “selected”):

- Row 0: tube number (`buf[0..2]`), name (`buf[4..12]`), Ug1 (`buf[24..27]`).
- Row 1: Uh, Ua, Ug2 (`buf[14..17]`, `buf[29..32]`, `buf[39..42]`).
- Row 2: Error char (H/A/G/T), Ih, Ia, Ig2 (`buf[19..22]`, `buf[33..37]`, `buf[43..47]`).
- Row 3:  
  - **Local (`typ > 1`):** S=, R=, then either K= or T= (countdown) from `buf` and `start`.  
  - **Power supply / Remote (`typ <= 1`):** blank line (typ==0) or “TX/RX: 9600,8,N,1” (typ==1).

So here we’re not computing values, only **displaying** whatever is already in `buf[]`. The rest of the loop is what **fills** `buf[]` and handles encoder/editing.

---

## 4. After LCD — INT1 and safety (1033–1034)

- **GICR = BIT(INT1)** — Re‑enable external interrupt (encoder/button).
- **Emergency off** — If there’s an error and we’re not in a “safe” part of the sequence, force `stop = 1` and set `start` to the safe shutdown point.

---

## 5. “Wyswietlanie Numeru” — Tube number and name (1036–1081)

**When `adr == 0`** (user is on “tube number” field):

- **5a.** Write current `typ` into `buf[0..2]` (tube number on LCD).
- **5b. Name source by mode:**  
  - **typ < FLAMP (catalog):**  
    - typ==1: `lamptem = lamprem` (remote template).  
    - else: load catalog entry `typ` into `lamptem`; if type changed and not during measurement restart, reset `lamprem` and setpoints.  
  - **typ >= FLAMP (EEPROM):** load `lampeep[typ-FLAMP]` into `lamptem`.  
  - Then set `tuh` from name byte 8 and fill `buf[4..12]` with the 9‑char name.
- **5c.** Set `lastyp = typ`, `nowa = 1`.

**When typ==0 or typ==1:** set `buf[11]` from `lamptem.nazwa[7]` (anode label).

So this block **chooses the current tube type and name** and puts them into `buf`; it’s the “which tube” logic.

---

## 6. Name editing — “Edycja Nazwy” (1083–1103)

**Only when `typ >= FLAMP`** (user-defined tube from EEPROM):

- If the selected field is one of the name chars (`adr` 1..9):  
  - Optionally read EEPROM name into `lamptem.nazwa`,  
  - Copy name to `buf[4..12]`,  
  - Handle debounce and write back one character to EEPROM when user confirms.
- Otherwise set `czytaj = 1` for next time.

So in **local** mode, when a user-defined type is selected, this handles editing the 9‑character name and saving to EEPROM.

---

## 7. Per‑parameter blocks (1104–1502) — **The big “convoluted” block**

From here to the end of the `sync` block, the code runs **the same long sequence for every tick**, regardless of mode. For **each** parameter (Ug1, Uh, Ih, Ua, Ia, Ug2, Ig2, S, R, K) it does two things:

**A. Compute current value and what to show**

- Read ADC (or use last measurement if `start == (FUH+2)`).
- Compute physical value (e.g. voltage/current) and decide the value to display (`licz`).
- Convert to ASCII and write into `buf[]` at fixed offsets (e.g. Ug1 → buf[24..27], Uh → buf[14..17], …).

**B. Apply encoder/editing and persistence**

- If the **current field** is this parameter (`adr == 10` for Ug1, `adr == 11` for Uh, …):  
  - When encoder is at “max” (`dusk0 == DMAX`): copy current default into `licz` and set `zapisz = 1`.  
  - When encoder leaves that position (`nodus == DMIN` and `zapisz == 1`):  
    - **Power supply (typ==0):** copy value to setpoints (e.g. `ug1set`, `uhset`, `uaset`, `ug2set`).  
    - **Local + EEPROM (typ >= FLAMP):** write that parameter to EEPROM (`lampeep[typ-FLAMP].ug1def`, etc.).  
    - Then clear `zapisz`.

So:

- **Power supply:** This is where **encoder-adjusted** Ug1/Uh/Ih/Ua/Ug2 (and for ELAMP, S/R/K) are written to setpoints or EEPROM; the same `buf[]` is what gets sent over the protocol when the host reads.
- **Remote:** Setpoints are driven by protocol (step 2); this block still updates `buf[]` and LCD from ADC/measurement and can touch EEPROM only for typ >= FLAMP (user-defined).
- **Local:** This is the **only** place where the user changes setpoints and catalog/EEPROM (via encoder + `adr`). So lines 944–1505 are “local UI”: refresh display from ADC/state and handle encoder edits for every parameter.

The repetition is “for each of the ~10 parameters: compute value → fill buf → if this field is selected, handle encoder and optionally write setpoint/EEPROM”. That’s why it looks long and repetitive.

---

## 8. Sending to PC — “Wyslanie pomiarow do PC” (1503–1513)

**When `txen == 1`** (set by ISRs when there’s something to send):

- Save current `typ` to EEPROM (`poptyp`).
- **Remote (typ == 1):** Send the 10‑byte legacy frame `bufin[0..9]`.
- **Other (typ != 1):** Send ASCII dump: `\r\n` then 63 chars from `buf[]` (space-padded), i.e. the LCD contents. This is what ESC triggers in local/power-supply.

Then clear `txen`.

---

## 9. End of `if (sync == 1)` (1514–1515)

- Closing brace of `if (sync == 1)` and of `while(1)`.
- Next iteration: WDR, then either nothing (sync==0) or another full pass.

---

## Summary by mode

| Step              | Power supply (typ==0)     | Remote (typ==1)            | Local (typ>1)              |
|-------------------|---------------------------|----------------------------|----------------------------|
| Protocol (2)      | Yes: SET/MEAS, responses  | Yes: SET/MEAS, responses   | No: discard 8‑byte frames |
| LCD refresh (3)   | Yes                       | Yes                        | Yes                        |
| Tube number (5)   | Yes (catalog 0)           | Yes (catalog 1 = REMOTE)   | Yes (catalog or EEPROM)    |
| Name edit (6)     | No (typ < FLAMP)          | No                         | Yes if typ >= FLAMP        |
| Params (7)        | Update buf + setpoints    | Update buf (setpoints from protocol) | Update buf + setpoints/EEPROM from encoder |
| TX (8)            | ASCII dump if txen        | 10‑byte frame or protocol  | ASCII dump on ESC          |

So: **protocol is only applied in power supply and remote** (step 2). The rest of the loop is shared display/state update, with mode-specific behavior only in “which tube”, “who drives setpoints”, and “what we send when txen is set”.
