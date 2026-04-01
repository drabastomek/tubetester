# VTTester Protocol Changelog

All notable changes to the VTTester communication protocol (Desktop Application <-> Firmware, REMOTE mode) are documented here.

---
## [0.4.1] – 2026-03-29

### Changed (breaking vs v0.4)
- **Frame length** — Standard frame is **9 bytes** (was 8) for SET, STATUS, RESET, ACK/BUSY/ERROR, and DATA.
- **CRC-8** — Computed over **bytes 0–7**; **byte 8** is the CRC (was bytes 0–6, CRC in byte 7).
- **SET (§6)** — Payload is **P1–P6**: heater (P1), **12-bit Ua/Ug2** across **P2–P4** (Ua low 8, shared high nibbles, Ug2 low 8), |Ug1| (P5), time index (P6). OUT_OF_RANGE uses **R2 = parameter id 1–6** where applicable.
- **DATA (§10)** — **Three** 9-byte frames (was **four** 8-byte): (1) Uh, Ih (uint16 LE each) + two reserved bytes; (2) Ua, Ug2, Ug1 (uint16 LE each); (3) Ia, Ig2 (uint16 LE) + alarm + one reserved byte. **Ug1** moves into frame 2; **alarm** into frame 3, **byte 6** (0-based).
- **INDEX example** — **0x12** for double triode, system 1, parallel (matches §5: heater bit 0, SYSTEM in bits 3–1).

---
## [0.4] - 2026-03-30

### Changed
- **Section 5**   
  INDEX bit 0: 0 = parallel heater, 1 = series heater. SYSTEM moved to bits 3–1 (v0.3 used bits 3–0 for system only).

- **Section 6, P1**  
  We are sending heater voltage or current. Instead of some predefined values, will be sending the actual values for voltage (0.1V multiplier) or current (10mA multiplier). This maps the values found in the constant catalog in the firmware and exceeds the full range of the heater power supply.

- **Section 10.1**   
  Rather than getting averaged values from the firmware -- we will be sending the sum of all the values of 64 measurements. Averaging to be done on the computer side.

---

## [0.3] – 2026-02-02

### Added
- **RESET command (CTRL 0x80)**  
  Power-down selected subcircuits in a defined order (Ua, Ug2, Ug1; optionally Uh) and clear alarm latch. P1: 0x01 = Ua, Ug2, Ug1 only; 0x02 = full power down (those + Uh, e.g. when swapping the tube). Replaces reliance on "SET with all zeros".
- **Explicit measurement flow**  
  SET -> ACK (measurement starts automatically) -> [optionally STATUS to poll] -> four RESULT frames (sent automatically by the meter) -> RESET. Example for double triode and RESET before tube swap.

### Changed
- **MEAS result format (breaking)**  
  - **v0.2:** One 8-byte frame with index-based R1–R5 (Ih, Ia, Ig2, S, extended status).  
  - **v0.3:** Four consecutive 8-byte frames with raw 16-bit little-endian values: Uh, Ih; Ua, Ug2; Ia, Ig2; Ug1 (magnitude) + alarm byte. S, R, K are no longer sent; the PC computes them from raw voltages and currents.  
  **Why:** The meter does not hold Ua/Ug2 to exact setpoints. Reporting actual Uh, Ih, Ua, Ia, Ug2, Ig2, Ug1 gives the desktop app accurate characteristics and full resolution; index-based encoding was limiting and did not match internal resolution.
- **Ug1 in MEAS result**  
  Sent as uint16 magnitude; physical voltage = -(value/100) V (e.g. 2400 = -24.00 V).  
  **Why:** Single scale, no sign handling in the binary frame.
- **READ renamed to STATUS**  
  Same semantics and CTRL code (0xA0); name clarifies that the command returns status/state only.
- **Alarm bits in MEAS**  
  In v0.3, alarm is only in MEAS result frame 4, byte 5, bits 7–4 (OVERTE, OVERIG, OVERIA, OVERIH); bits 3–0 reserved.  
  **Why:** One consistent place and bit layout for alarms in the result.

### Removed
- **Single 8-byte MEAS result**  
  Replaced by the four-frame raw result; v0.2 clients cannot parse v0.3 MEAS payloads.
- **20-byte extended frame**  
  Not used; MEAS uses only the four 8-byte frames.
- **Distinction between “single” and “multi” measurement mode in the flow**  
  One measurement sequence is described.

### Document
- **Standalone v0.3 specification**  
  The v0.3 document is self-contained; it does not refer to v0.2 for definitions. See `VTTester_Protocol_v0.2_to_v0.3_diff.txt` for a concise what-and-why summary.

---

## [0.2] – 2026-02-02

### Added
- **REMOTE-only protocol**  
  Binary protocol for desktop application <-> firmware; no front-panel catalog or EEPROM writes from the PC.
- **Commands: SET, MEAS, READ (status)**  
  SET loads parameters into RAM (not EEPROM). MEAS runs the measurement and returns one 8-byte result frame. READ returns current status and optional last Ua/Ia/Ug1.
- **Legacy ASCII mode (ESC)**  
  Sending 0x1B (ESC) can request an ASCII dump of the LCD line for debugging; coexists with the binary protocol.
- **Index-based parameter encoding (P1–P5)**  
  Heater index, Ua/Ug2/Ug1 with resolution bits, and measurement time index, aligned with the original v0.1 mapping for compatibility.
- **CRC-8 and error handling**  
  CRC-8 (polynomial 0x07) over bytes 0–6; on CRC error the firmware responds with ERROR and R1=CRC_ERROR so the PC can retry.

### Removed
- **STORE command**  
  No remote EEPROM writes from the desktop app; settings are not persisted by the device when controlled remotely.
- **Catalog operations**  
  Desktop app uses a local tube database; the protocol does not read the hardware catalog.

### Why
- **Efficiency and timing:** Binary frames (8 bytes) are much faster than ASCII and give deterministic timing for curve tracing.
- **Safety:** CRC and strict “do not execute on bad CRC” rule protect against wrong voltages.
- **Scope:** Protocol focuses on what the desktop needs: set parameters, run measurement, read status, with a path for future extensions.

---

## [0.1] – Initial proposal

- **Source:** Polish document *Protokół komunikacyjny VTTester.pdf*.
- **Content:** Original parameter encoding (tube type, heater, Ua, Ug2, Ug1, measurement time), catalog and local operation, and the idea of index-based mapping for voltages and currents.
- **Why baseline:** v0.2 and v0.3 keep SET parameter encoding and INDEX (tube type, system) compatible with this mapping where applicable; REMOTE mode and MEAS result format evolved in v0.2 and v0.3.

---

*Document: CHANGELOG.md  
Location: firmware/protocol/  
Last updated: 2026-03-29*
