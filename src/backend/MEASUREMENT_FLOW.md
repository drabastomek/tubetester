# Desktop measurement flow — requirements (draft)

Application-level sequence for remote tube measurement over the v0.5 wire protocol.  
Agreed with firmware/hardware; timing uses **125 ms** between STATUS polls (was 100 ms in early notes).

**Protocol reference:** `firmware/protocol/VTTester_Protocol_v0.5.txt`  
**Desktop codec / serial:** `src/backend/communication/`  
**Harness:** split-mode firmware (`SET → ACK`, `STATUS → DATA`) — desktop owns all waits and loops.

---

## 1. Timing and protocol primitives

| Constant | Value | Notes |
|----------|-------|--------|
| `STATUS_POLL_INTERVAL_MS` | **125** | Wait after SET before first STATUS; minimum spacing between STATUS polls in stability loop |
| `HEATER_PREHEAT_S` | **60** (default) | User-configurable; no UART traffic required during preheat |

| Command | Wire | Typical response |
|---------|------|------------------|
| **SET** | 10-byte request, CMD=0x00 | **ACK** (2 B) — parameters accepted, measurement armed |
| **STATUS** | 10-byte request, CMD=0x02 | **DATA** (20 B) — raw ADC sums + ERR byte |
| **RESET** | 10-byte request, CMD=0x01 | **ACK** — power-down / restore defaults per reset kind |
| **BEEP** | 10-byte request, CMD=0x03 | **ACK** — buzzer tone (DUR byte = 10 ms units; 0 → 50 ms) |

The desktop app drives timing. Firmware does **not** auto-send DATA after SET in the production flow; the app waits **125 ms**, then polls STATUS.

Scaling from DATA frame sums to volts/amperes is agreed between PC and firmware builds (see protocol §5).

---

## 2. Stability loop (used after every SET)

After each SET that changes a measurement point, the app waits **`STATUS_POLL_INTERVAL_MS`**, then:

1. Send **STATUS** → receive **DATA (1)**.
2. Wait **`STATUS_POLL_INTERVAL_MS`** again.
3. Send **STATUS** → receive **DATA (2)**.
4. For each monitored channel, if `abs(value(1) - value(2)) > lambda` for that channel → go to step 1 (re-poll).
5. If all monitored channels are within lambda → treat readings as **stable**; store the snapshot (use DATA (2) or average — TBD).

**Default channels to compare (TBD exact lambdas):**

| Channel | Typical use |
|---------|-------------|
| Ua | Anode voltage settled |
| Ia | Anode current settled |
| Ue / Ie | Screen / auxiliary (pentode) |
| Ug | Grid (when stepping Ug1) |

Lambdas (`lambda_ua`, `lambda_ia`, `lambda_ue`, …) are desktop configuration, not on the wire.

---

## 3. Full measurement sequence

### Step 0 — Heater (once per tube)

1. **SET** with **Uh** and/or **Ih** (series heater: Uh=0, IH set).
2. Wait **heater preheat** (default 60 s; user may set longer).
3. No STATUS required during preheat unless the app wants live heater readback.

### Step 1 — Select system and bias point

For each measurement point **(system, Ug1, Ua, Ug2)**:

1. **SET** — apply, in order as needed:
   - **A1_A2** — anode / half-tube / system selector
   - **Ug1** (UG byte)
   - **Ua** (UA uint16 LE)
   - **Ug2** (UE uint16 LE) — **pentode only**; omit or zero for triode
2. Wait **125 ms**.
3. Run **stability loop** (§2) → store **Ia_ref**, **Ie_ref** (and Ua/Ue if needed).

### Step 2 — Grid −0.4 V

1. **SET** — Ug1 **− 0.4 V** (same Ua, Ug2, system).
2. Stability loop → store **Ia1**, **Ie1**.

### Step 3 — Grid +0.4 V

1. **SET** — Ug1 **+ 0.4 V**.
2. Stability loop → store **Ia2**, **Ie2**; app may compute **Sa**, **Se** here.

### Step 4 — Anode −10 V

1. **SET** — Ug1 back to **original** value; **Ua − 10 V**.
2. Stability loop → store **Ia_low**, **Ie_low**.

### Step 5 — Anode +10 V

1. **SET** — **Ua + 10 V** (Ug1 still at original for this sub-step).
2. Stability loop → store **Ia_high**, **Ie_high**; app computes **Ra**, **Ku**, **μ** (or u2) from stored points.

### Step 6 — End of point / system

1. **RESET** (or **SET** to safe values): **Ua = 0**, **Ug2 = 0**, **Ug1 = −24 V** (wire values per catalog encoding).

### Double triode

After step 6, **return to §3 step 1** with the **second system** (A1_A2 / system selector).

### Tube change

After finishing all systems:

10. **RESET** heater: **Uh = 0**, **Ih = 0** (RESET kind or explicit SET — match firmware convention).

---

## 4. Curve tracing (simplified)

To measure a characteristic curve, repeat **§3 step 1** only (SET point → stability loop) for **each (Ua, Ug1)** pair in the trace grid — skip grid-step substeps 2–5 unless the app needs full parameter extraction at every point.

---

## 5. Pseudocode

```text
SET(Uh, Ih)
wait HEATER_PREHEAT_S

for each system in tube.systems:
    for each (Ug1, Ua, Ug2) in measurement_plan:
        SET(system, Ug1, Ua, Ug2)
        stable(DATA) -> store ia_ref, ie_ref

        SET(Ug1 - 0.4V)
        stable(DATA) -> store ia1, ie1

        SET(Ug1 + 0.4V)
        stable(DATA) -> store ia2, ie2   # Sa, Se

        SET(Ug1_orig, Ua - 10V)
        stable(DATA) -> store ia_low, ie_low

        SET(Ua + 10V)
        stable(DATA) -> store ia_high, ie_high   # Ra, Ku, mu

        RESET(Ua=0, Ug2=0, Ug1=-24V)

    if double_triode:
        continue next system

RESET(Uh=0, Ih=0)   # tube change
```

```python
def wait_stable(link, lambdas) -> Data:
    """Poll STATUS every STATUS_POLL_INTERVAL_MS until two consecutive samples match."""
    while True:
        sleep_ms(STATUS_POLL_INTERVAL_MS)
        d1 = link.get_status()
        sleep_ms(STATUS_POLL_INTERVAL_MS)
        d2 = link.get_status()
        if all(abs(getattr(d1, ch) - getattr(d2, ch)) <= lambdas[ch] for ch in lambdas):
            return d2
```

---

## 6. Desktop app responsibilities (this document)

- [ ] Heater preheat timer before first SET at a bias point  
- [ ] **125 ms** delay after every SET before first STATUS  
- [ ] Stability loop with configurable lambdas  
- [ ] State machine for steps 1–6 per point; system iteration for double triode  
- [ ] Pentode: include Ug2 in SET; triode: Ug2 = 0  
- [ ] Engineering-unit conversion from DATA sums  
- [ ] Ra, Ku, Sa, Se, μ from stored Ia/Ie/Ua/Ue snapshots  
- [ ] RESET / safe idle between points and on tube change  
- [ ] Error handling: OUT_OF_RANGE, ERR byte alarms, TIMEOUT on missing DATA  

**Out of scope for this doc:** tube catalog UI, EEPROM, local vs remote mode, plotting.

---

## 7. Test harness alignment

| Production flow | Harness (default `HARNESS_FLOW=split`) |
|-----------------|------------------------------------------|
| SET → ACK | Yes |
| wait 125 ms (desktop) | Simulated in app / pytest |
| STATUS → DATA | Yes |
| Stability loop | Desktop logic; future pytest scenario |
| RESET → ACK | Yes |

The optional **`HARNESS_FLOW=auto`** build (SET → delayed DATA, no ACK) is **not** the agreed production sequence; use **split** firmware for harness and desktop bring-up.

---

*Document: `src/backend/MEASUREMENT_FLOW.md`*  
*Last updated: 2026-06-23*
