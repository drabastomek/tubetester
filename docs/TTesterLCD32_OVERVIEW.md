# TTesterLCD32.c – Firmware overview

Short map of what happens where so you can follow the source. Target: **ATmega32**, 16 MHz. Language: C with Polish comments.

---

## 1. What the firmware does

- **Standalone tube tester**: user picks a tube type (from ROM catalog or EEPROM), starts a measurement with the encoder/button, firmware sequences voltages (heater Uh/Ih, grid Ug1, anode Ua, screen Ug2), samples currents/voltages via ADC, computes S, R, K, shows everything on LCD.
- **Remote / PC**: UART 9600 8N1. Sending **ESC** (0x1B) makes the firmware send an **ASCII dump** of the current screen buffer (one line of tube number, name, voltages, currents, S/R/K). There is **no binary protocol** in this file; that would be added elsewhere (e.g. parse SET/MEAS, set `lamptem`/start, send binary frames).

---

## 2. Hardware / I/O (top of file)

- **Port A**: ADC inputs only (REZ, IH, UH, UA, IA, UG2, IG2, UG1 – see `ADRMUX` defines).
- **Port B**: RNG/SEL/CKG/UH (bits 0–3 output); bits 4–7 inputs (D7–D5, K1). Macros: `SET200`/`RST200` (Ia range 200 mA vs 40 mA), `SETSEL`/`RSTSEL` (anode 1 vs 2), `CLKUG1SET`/`CLKUG1RST` (Ug1 DAC step).
- **Port C**: LCD data (high nibble) + ENABLE, RS; bits 2–3 = RS, ENABLE.
- **Port D**: SPK (buzzer), encoder (DIR, K0), UG2/UA enables, **TXD/RXD** (UART).
- **Timers**: T0 = PWM heater (Uh, 8-bit); T1 = PWM Ua and Ug2 (phase/freq, `ICR1` top); T2 = CTC **1 ms** tick (`OCR2 = MS1`).
- **UART**: 9600 (RATE=103), RX/TX interrupts enabled.

---

## 3. Important global state

- **Tube selection / settings**
  - `typ` – current tube index (0 = PWRSUP, 1..FLAMP-1 = ROM catalog, FLAMP..FLAMP+ELAMP-1 = EEPROM catalog).
  - `lamptem` – copy of current tube’s parameters (name, uhdef, ihdef, ug1def, uadef, iadef, ug2def, ig2def, sdef, rdef, kdef); used for setpoints and EEPROM read/write.
  - `adr` – “cursor” for which field is being edited (0=tube number, 1–9=name, 10=Ug1, 11=Uh, 12=Ih, 13=Ua, 14=Ia, 15=Ug2, 16=Ig2, 17–19=S,R,K).

- **Measurement sequencer**
  - `start` – down-counter driven every **MRUG** ticks in `timer2_comp`. When non-zero, the sequencer is in a specific phase (heater ramp, Ug1/Ua/Ug2 steps, sampling, beeps, shutdown). When it hits the “capture” phase, it stores `uhlcd, ihlcd, ualcd, ialcd, ug2lcd, ig2lcd, slcd, rlcd, klcd` and sets **`txen = 1`** (so the main loop will send the ASCII dump on next sync).
  - `stop` – request to stop after current measurement (e.g. encoder).

- **UI / timing**
  - `sync` – set to 1 every **MRUG** ticks in `timer2_comp`; main loop runs the “big” block only when `sync == 1`, then sets `sync = 0`. So the main loop runs at a fixed rate (e.g. ~4 Hz if MRUG=250 and 1 ms tick).
  - `dusk0`, `nodus` – button debounce / encoder state (DUSK0 = K0 pressed; RIGHT = encoder direction).
  - `buf[63]` – ASCII buffer for the current screen line (tube number, name, Uh, Ih, Ug1, Ua, Ia, Ug2, Ig2, S, R, K). Filled in main from `lamptem` and ADC-derived values; sent out UART when `txen` is set.

- **UART**
  - `txen` – “send measurement to PC”: set either by **ESC** in `usart_rxc`, or when measurement finishes in `timer2_comp` (line ~916). Main loop (around 1638) sends `buf` as ASCII and clears `txen`.
  - `busy` – TX in progress (cleared in `usart_txc`).

- **ADC**
  - `kanal` – current ADC channel (0..13); round-robin in `adc()`.
  - Accumulators like `sihadc`, `suaadc`, … and after `LPROB` samples they’re copied to `mihadc`, `muaadc`, … and used for regulation and for computing `uh`, `ih`, `ua`, `ia`, `ug2`, `ig2`, `ug1`, `s`, `r`, `k`.

---

## 4. Program flow (high level)

1. **main()** (~999)
   - Init ports, timers, ADC, UART, watchdog, INT1 (encoder), Timer2 compare.
   - Init `vref`, `ug1set/ug1zer/ug1ref`, EEPROM read of last `typ`, enable interrupts.
   - LCD init, splash screen, then **`while(1)`**:
     - **WDR**
     - If **`sync == 1`**: set `sync = 0`, then:
       - Refresh **LCD** from `buf` (tube number, name, Uh, Ua, Ug2, Ih, Ia, Ig2, S, R, K, errors).
       - Depending on `adr`, update **`buf`** from `lamptem` and from ADC-derived values (`uh`, `ih`, `ua`, `ia`, `ug2`, `ig2`, `ug1`, `s`, `r`, `k`), with `int2asc` and fixed positions (e.g. buf[13]–buf[16] = Uh, buf[18]–buf[20] = Ih, …).
       - If **`txen`**: send `buf` out UART as ASCII (with `\r\n` and spaces for empty), then `txen = 0`.
     - Re-enable INT1, optional emergency stop logic.

2. **timer2_comp** (1 ms)
   - Decrement `zwloka` (delay).
   - **Button/encoder**: `DUSK0` / `RIGHT` drive `dusk0`, `nodus`; they start/restart the measurement (`start = ...`) when conditions are met, or drive the encoder state machine in **ext_int1** (change `typ`, `adr`, edit `*cwart` / `*wart`).
   - Every **MRUG** ticks: set **`sync = 1`** (so main loop runs one “frame”), then run the **measurement sequencer**:
     - `start` is a big constant that encodes the whole sequence (heater time, Ug1/Ua/Ug2 phases, sampling points, beeps, shutdown). Each tick, one of many `if ( start == ( ... ) )` blocks runs:
       - Set setpoints: `uhset`/`ihset`, `ug1set`, `uaset`, `ug2set`.
       - At sampling points: read `ug1`, `ia`, `ua` and compute `s` (transconductance), `r` (anode resistance), `k = R*S`.
       - At the **capture** phase (`start == (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)`): save `uhlcd`, `ihlcd`, … and set **`txen = 1`**.
       - Ramp down (ug2set=0, uaset=0, ug1set=ug1zer), beeps, then `start` goes to (FUH+2) or 0.
     - Then `start` is decremented (or set to (FUH+1) when `stop` and at end), so the sequence advances one step per “sync” period.

3. **ext_int1** (encoder/button)
   - If measurement is running and encoder turned: can force early stop (`start = ...`, `stop = 0`) or at end of measurement change `typ` (next/prev tube).
   - If not measuring (`start == 0` or at rest): sets **edit context** (`cwart`, `cwartmin/max`, `wart`, `wartmin/max`) from `adr` (tube number, name, Ug1, Uh, Ih, Ua, Ia, Ug2, Ig2, S, R, K), then increments/decrements `*cwart` or `*wart` on encoder rotation, or changes `adr` on click (`nodus == DMIN`). EEPROM writes for catalog entries when editing.

4. **adc()** (free-running, triggered by timer)
   - Round-robin over channels: REZ (temp), IH, UH, UG1, UA, IA, UG2, IG2, UG1 (and toggles CLKUG1 for Ug1 DAC). Accumulates into `sihadc`, `suaadc`, etc. After **LPROB** (63) samples, copies to `mihadc`, `muaadc`, … and:
     - Regulates **heater**: PWM Uh (T0) from `uhset`/`ihset` vs `muhadc`/`mihadc`.
     - Regulates **Ua, Ug2**: `PWMUA`, `PWMUG2` vs setpoints; overcurrent (Ia, Ig2, Ih) sets `err` (OVERIA, OVERIG, OVERIH) and clears setpoints.
     - Auto-range **Ia**: `range` 0/1 and SET200/RST200.
     - Temperature alarm: `mrezadc` vs HITEMP/LOTEMP → OVERTE.

5. **UART**
   - **usart_rxc**: if received byte == **ESC**, set **`txen = 1`** (request ASCII dump).
   - **usart_txc**: set `busy = 0`.
   - Actual send is in **main**: when `txen` and `sync`, it sends `buf` with `char2rs()` / `cstr2rs()`.

---

## 5. Where the “measurement complete” send happens

- In **timer2_comp**, at the exact phase where the sequencer has just captured all values:
  - `start == (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)` →  
    `uhlcd = uh; ihlcd = ih; ... ; slcd = s; rlcd = r; klcd = k; **txen = 1**;`
- On the **next** main-loop iteration (when `sync == 1`), `buf` is already filled from those same values (and from `lamptem` for names/numbers). The block at the end of the sync block (**if (txen)**) sends that buffer over UART and clears `txen`.

So: **no binary protocol here**. To add VTTester binary protocol (SET/MEAS/READ), you’d add a UART RX state machine that collects 8-byte frames, checks CRC, and:
- On **SET**: decode P1–P5 into `lamptem` (and maybe set a “remote” flag), then send 8-byte ACK.
- On **MEAS**: send immediate 8-byte MEAS ACK, then (re)use the existing sequencer to run a measurement; when it hits the same phase (or a dedicated “remote result” path), build the 8-byte MEAS result from `uhlcd`, `ihlcd`, … and send it instead of (or in addition to) setting `txen` for the ASCII dump.

---

## 6. Key constants (for the sequencer)

- **TMAR, TUA, TUG2, TUG, TUH, FUA, FUG2, FUG, FUH, BIP** – timing steps for the sequence (see comments in timer2_comp: “czekaj tuh TUG TUA TUG2 …”).
- **FUH+2** – “idle” state after measurement (or 1 = last step before idle).
- The long `start == ( ... )` expressions are one phase each; decrementing `start` walks through them in reverse order.

---

## 7. File layout (approximate line numbers)

| Section              | Lines (approx) | Content |
|----------------------|----------------|--------|
| Defines, types       | 1–150          | BIT, ports, ADC mux, timing, `katalog` |
| Globals              | 106–150        | `buf`, `typ`, `start`, `txen`, `sync`, ADC vars, etc. |
| Helpers               | 341–380        | `char2rs`, `cstr2rs`, `delay`, `zersrk`, `liczug1` |
| ext_int1             | 392–574        | Encoder/button: start/stop, edit typ/adr, change values |
| adc                  | 574–823        | Round-robin ADC, averaging, PWM regulation, overcurrent |
| usart_txc / usart_rxc| 825–833        | `busy = 0`; ESC → `txen = 1` |
| timer2_comp          | 835–943        | 1 ms + MRUG tick; sync=1; measurement sequencer; **txen=1** at capture |
| LCD helpers          | 946–998        | cmd2lcd, gotoxy, char2lcd, str2lcd, int2asc |
| main                 | 999–1650       | Init, then while(1): sync → refresh LCD, fill buf, **if(txen) send buf** |

This should be enough to jump to the right place when you ask “where is X?” (e.g. where SET would be applied, where MEAS result would be sent, or where the sequencer sets voltages).
