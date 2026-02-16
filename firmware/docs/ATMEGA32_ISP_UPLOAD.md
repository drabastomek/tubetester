# Uploading firmware to ATmega32

Assumes you have a **working ISP programmer** (e.g. USBasp). Target: **ATmega32** (this project’s main MCU). No fuse write is required for normal programming.

---

## Hardware

- **Power the board** from its own supply (7–12 V into the regulator input). Use a common **GND** between programmer and board. Do not power the target from the programmer’s 5 V line if the board has its own regulator.
- **ISP:** Connect the programmer’s 6‑pin cable to the board’s 6‑pin ISP header by **signal name** (MOSI→MOSI, MISO→MISO, SCK→SCK, RESET→RESET, GND→GND). Pin 1 on the connector is often marked; match your programmer’s pinout to the PCB silk or schematic.

**PCB 6‑pin ISP connector** (view from top; pin 1 = black on this board):

```
MISO (pin 7)    1 [ ] o 2     5V
SCK  (pin 8)    3  o  o 4     MOSI (pin 6)
RES  (pin 9)    5  o  o 6     GND
```

ATmega32 40‑pin DIP: RESET = pin 9, MOSI = PB5 = pin 6, MISO = PB6 = pin 7, SCK = PB7 = pin 8, VCC = 10, GND = 11/31.

The project's PCB has an ISP header with the pinout as above and it already connects these pins on the chip.

---

## Build

From the **firmware** directory:

```bash
make
```

Produces **`bin/TTesterLCD32.hex`** (Intel HEX). The Makefile is set for `MCU=atmega32` and `F_CPU=16000000`.

---

## Program (upload)

From the **firmware** directory. Replace `usbasp` with your programmer type (e.g. `avrisp`) and add `-P <port>` if needed (e.g. serial programmers).

**Verify connection:**

```bash
avrdude -p atmega32 -c usbasp -n
```

**Upload flash:**

```bash
avrdude -p atmega32 -c usbasp -U flash:w:bin/TTesterLCD32.hex:i
```

If your programmer needs a slower clock (e.g. “cannot set sck period” or unstable connection), add **`-B 32`** to the avrdude command.

---

## Fuses (read before first use; set once for new chip)

**Read fuses** (no write) — run from `firmware/`, same programmer as above:

```bash
avrdude -p atmega32 -c usbasp -U lfuse:r:-:h -U hfuse:r:-:h -U lock:r:-:h
```

Example output: `lfuse: 0x3f`, `hfuse: 0xc9`, `lock: 0xff`. ATmega32 has **no extended fuse** (unlike some other AVRs).

| Fuse   | Typical (this project) | Meaning |
|--------|------------------------|--------|
| **lfuse** | 0x3f or 0xef | External crystal 8–16 MHz (CKSEL=1111), no clock divide (CKDIV8=0). Required for 16 MHz crystal; firmware assumes `F_CPU=16000000`. |
| **hfuse** | 0xc9 or 0xcf | Boot reset vector, BOD on. 0xc9 = SPIEN, EESAVE off; 0xcf = EESAVE on. |
| **lock**  | 0xff or 0xc0 | No lock (programming allowed). |

**New chip:** Factory default is often **internal RC oscillator** (CKSEL ≠ 1111). If the board uses an **external 16 MHz crystal**, set lfuse once so the MCU runs at 16 MHz, then upload firmware:

```bash
# Example: external crystal 8–16 MHz, no divide, 16K ck startup (check datasheet for your board)
avrdude -p atmega32 -c usbasp -U lfuse:w:0x3f:m -U hfuse:w:0xc9:m
```

Use a [fuse calculator](https://www.engbedded.com/fusecalc/) or the [ATmega32 datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/doc2503.pdf) to confirm; wrong fuses can lock the chip. After fuses are correct, normal workflow is **no fuse write** — just build and upload flash.

---

## Summary

| Step        | Command (from `firmware/`) |
|-------------|----------------------------|
| Build       | `make`                     |
| Verify      | `avrdude -p atmega32 -c usbasp -n` |
| Read fuses  | `avrdude -p atmega32 -c usbasp -U lfuse:r:-:h -U hfuse:r:-:h -U lock:r:-:h` |
| Upload flash| `avrdude -p atmega32 -c usbasp -U flash:w:bin/TTesterLCD32.hex:i` |

**New chip:** Read fuses first; if the board uses a 16 MHz crystal and the chip is still on factory default (internal RC), set lfuse/hfuse once (see Fuses section), then upload. After that, no fuse write is needed for normal programming.