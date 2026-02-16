# ATmega16 on original PCB: ISP, read fuses, LCD check, upload v.1.16 (LEGACY)

**This file is for reference only** (Arduino-as-ISP issues, MINI PROG firmware update, ATmega16/v.1.16). The main upload doc is for ATmega32 with a working programmer.

---

Read fuses and upload the legacy hex (`old_code/v.1.16`) to the ATmega16 on the original tube-tester PCB. Also covers LCD wiring (J2) and v.1.16 firmware.

**Recommended path (verified):** Use a **USBasp-style programmer** (e.g. **MINI PROG S51 & AVR**). If the programmer reports "cannot set sck period", update its firmware once using an Arduino Uno as ISP (with a **10–22 µF cap from Uno RESET to GND** to avoid protocol errors). After that, `avrdude -p atmega16 -c usbasp` works for verify, read fuses, and upload; **no fuse writes are required** for programming. Arduino Uno as ISP is documented as an alternative but may report the wrong device signature on this PCB; USBasp is reliable.

---

## 1. Alternative: Arduino Uno as ISP to the ATmega16 (PCB)

**On the PCB, pin 1 is black (you confirmed).** ATmega16 40‑pin DIP: pin 1 = PB0, then PB1…PB7 = pins 2–8, pin 9 = RESET, pin 10 = VCC, pins 11 and 31 = GND.

- **Power the PCB** from its own supply (not from the Uno). Use a common **GND** between Uno and PCB.  
  **Use the regulator input:** feed **7–12 V** (e.g. 8 V) into the **input** of the board's LM7805 so it can output a stable 5 V. Do *not* connect 5 V directly to the 5 V rail (past the regulator)—that can droop under load (e.g. to ~4.2 V) and the MCU/LCD may not run or the display stays blank.
- Connect **only** these 5 lines for programming:

| Arduino Uno (ISP) | Wire to PCB (ATmega16) |
|-------------------|------------------------|
| **Pin 10**        | **RESET** (ATmega16 pin 9) |
| **Pin 11 (MOSI)** | **MOSI** = **PB5** (ATmega16 pin 6) |
| **Pin 12 (MISO)** | **MISO** = **PB6** (ATmega16 pin 7) |
| **Pin 13 (SCK)**  | **SCK**  = **PB7** (ATmega16 pin 8) |
| **GND**           | **GND** (e.g. PCB pin 11 or 31) |

The ISP connector on the board is as follows (view from the top)

```
MISO (Pin 7)    1[ ] o 2    5V
SCK  (Pin 8)    3 o  o 4    MOSI (Pin 6)
RES  (Pin 9)    5 o  o 6    GND
```

Do **not** connect 5 V from the Uno to the PCB to avoid backfeeding. Only GND and the four ISP signals + RESET.

**Before anything else:** upload the **ArduinoISP** sketch to the Uno (File → Examples → 11.ArduinoISP → ArduinoISP), then connect the six wires above.

**Note:** On this PCB, Arduino-as-ISP has been observed to report **ATmega328P** signature (1E 95 0F) instead of ATmega16 (1E 94 03) even with correct wiring and two different Unos. For reliable ISP, use a USBasp-style programmer (section 1b).

---

## 1b. Using a USB programmer (USBasp / MINI PROG S51) — recommended

If you use a **USBasp-style** programmer (e.g. **MINI PROG S51 & AVR**), use `-c usbasp` and no `-P` (USB only).

**"cannot set sck period" (avrdude 7+/8+):** Newer avrdude always sends a "set SCK period" command during init. Many clones (including MINI PROG S51) have **old USBasp firmware** that doesn't support that command, so avrdude exits with an error. `-B 32` does **not** fix it (avrdude still sends the command). You have two options:

1. **Use an older avrdude** that only warns instead of exiting (e.g. avrdude 6.3 from [Savannah](https://download.savannah.gnu.org/releases/avrdude/) or build from source with an older tag). Then add `-B 32` for a slow clock.
2. **Update the programmer's firmware** (recommended): Put the MINI PROG in **self-programming** mode (JP2 / "Self Program" jumper on, or see the unit's label). Connect an **Arduino Uno as ISP** to the MINI PROG's 6‑pin header (Uno 10→RST, 11→MOSI, 12→MISO, 13→SCK, GND→GND). **Power the MINI PROG** from USB or its own supply so the ATmega8 is running. Flash [USBasp firmware 2011-05-28](https://www.fischl.de/usbasp/) (e.g. `usbasp.atmega8.2011-05-28.hex` if the programmer uses an ATmega8). Use **`-c avrisp`** (often more reliable than `-c arduino` for this) and **`-r`** so avrdude waits after opening the port. Example (replace port):
   ```bash
   avrdude -c avrisp -P /dev/cu.usbmodemXXXX -b 19200 -p atmega8 -r -U flash:w:usbasp.atmega8.2011-05-28.hex:i
   ```
   If you get "protocol expects OK byte 0x10 but got 0x14" or "sync byte 0x14 but got 0x01", the Uno is **resetting when the serial port is opened** (DTR). **Fix:** put a **10–22 µF electrolytic capacitor** from the Uno's **RESET** pin to **GND** (positive leg to RESET); then run the avrdude command again. (Alternatively, press the Uno's reset button and run avrdude within 1–2 seconds.) Remove the self-program jumper and disconnect the Arduino when done. **After the firmware update**, current avrdude works with `-c usbasp`; `-B 32` is usually not needed for ATmega16.

**Once the programmer has updated firmware**, verify and program the ATmega16 (no `-P`, no fuse write required):

```bash
avrdude -p atmega16 -c usbasp -n
```

If you ever see "cannot set sck period" again, add `-B 32`. Use the same `-c usbasp` (and optional `-B 32`) for read fuses and upload.

**Pinout:** Many of these units use a **non‑standard** 6‑pin order where **pin 1 = MOSI** (standard Atmel 6‑pin has pin 1 = MISO). Match your programmer's connector to the PCB using the **signal** names, not pin numbers.

| If your programmer has… | Programmer pin → signal | Wire to PCB (ATmega16) |
|-------------------------|-------------------------|-------------------------|
| **Pin 1 = MOSI** (e.g. MINI PROG S51) | 1=MOSI, 2=VCC, 3=SCK, 4=MISO, 5=RESET, 6=GND | MOSI→pin 6, (VCC optional), SCK→pin 8, MISO→pin 7, RESET→pin 9, GND→pin 11/31 |
| **Pin 1 = MISO** (standard 6‑pin)     | 1=MISO, 2=VCC, 3=SCK, 4=MOSI, 5=RESET, 6=GND | MISO→pin 7, (VCC optional), SCK→pin 8, MOSI→pin 6, RESET→pin 9, GND→pin 11/31 |

**PCB 6‑pin ISP connector** (same as in section 1):

```
MISO (Pin 7)    1[ ] o 2    5V
SCK  (Pin 8)    3 o  o 4    MOSI (Pin 6)
RES  (Pin 9)    5 o  o 6    GND
```

So: connect the programmer's **MOSI** wire to PCB position **4** (MOSI), **MISO** to **1** (MISO), **SCK** to **3** (SCK), **RESET** to **5** (RES), **GND** to **6** (GND). Ignore programmer pin numbers; follow the labels on the PCB. Power the PCB from its own supply (e.g. 7–12 V into the 7805 input) and share GND with the programmer.

---

## 2. Read fuses and lock bits from the chip

This checks that the MCU is alive and shows current fuse configuration. No fuse write is required for programming; reads are optional.

**Find the Uno's port:**

- **macOS:** `ls /dev/cu.usb*` or `ls /dev/cu.usbmodem*`
- **Windows:** e.g. `COM3` in Device Manager
- **Linux:** e.g. `/dev/ttyACM0`

**USBasp** (no `-P`; add `-B 32` if you see "cannot set sck period"):
```bash
avrdude -p atmega16 -c usbasp -n
avrdude -p atmega16 -c usbasp -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h -U lock:r:-:h
```
**Arduino-as-ISP:** use `-c arduino -P /dev/cu.usbmodemXXXX` (replace XXXX with your port) in place of `-c usbasp`.

Example output: `lfuse: 0x3f`, `hfuse: 0xcf`, `efuse:` (skipped — ATmega16 has no extended fuse), `lock: 0xc0`. Typical meaning: **lfuse 0x3f** = external crystal, no clock divide; **hfuse 0xcf** = normal reset, BOD on; **lock 0xc0** = no lock (programming allowed). You can look up the bits in the [ATmega16 datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/doc2466.pdf) (e.g. clock source, CKDIV8, EESAVE, BOD, lock bits). **No fuse write is required** for programming.

**If you get "device not responding" or "not in sync":**

- Confirm pin 1 on the PCB (black) matches pin 1 on the ATmega16 (top-left when notch is up).
- Recheck RESET (Uno 10 → ATmega16 pin 9), MOSI/MISO/SCK (Uno 11/12/13 → ATmega16 pins 6/7/8), and common GND.
- Try adding a 10–22 µF capacitor between RESET and GND on the Uno to stabilize RESET during programming (some ArduinoISP docs suggest this).
- Ensure the PCB has 5 V and GND applied so the ATmega16 is powered.

---

## 3. Check your LCD connections (J2 → LCD)

Your J2 pinout (black = pin 1) and the v.1.16 firmware match:

| J2 (PCB) | Signal   | LCD pin (HD44780 16-pin) | v.1.16 firmware  |
|----------|----------|---------------------------|------------------|
| J2-1     | GND      | 1, 5, 16 (VSS, R/W, LED−) | —                |
| J2-2     | +5 V     | 2 (VDD)                   | —                |
| J2-3     | VLCD     | 3 (V0 contrast)           | —                |
| J2-4     | RS       | 4 (RS)                    | **PC2**          |
| J2-5     | E        | 6 (E)                     | **PC3**          |
| J2-6     | D4       | 11 (D4)                   | **PC4**          |
| J2-7     | D5       | 12 (D5)                   | **PC5**          |
| J2-8     | D6       | 13 (D6)                   | **PC6**          |
| J2-9     | D7       | 14 (D7)                   | **PC7**          |
| J2-10    | Backlight anode | 15 (LED+)           | —                |

So: **RS → PC2, E → PC3, D4–D7 → PC4–PC7**. The firmware sets `DDRC = 0xfc` (PC2–PC7 output), so the mapping is correct. If the LCD still shows nothing after a successful flash:

- **Supply voltage:** Ensure the board is powered via the LM7805 input (7–12 V), not 5 V on the rail (see section 1). Low VCC (e.g. ~4.2 V) can make the LCD appear completely blank.
- **Contrast:** Turn the VLCD pot (J2-3 → LCD pin 3) slowly; often the display is there but contrast is at 0.
- **Power:** Confirm 5 V on LCD pin 2 and GND on 1.
- **Backlight:** If your module has backlight, 5 V on pin 15 and GND on 16 (or through a small resistor).

---

## 4. Upload the v.1.16 hex

Hex file: **`firmware/old_code/v.1.16/TTesterLCD.hex`**. No fuse write is needed; the default fuses (e.g. lfuse 0x3f, hfuse 0xcf) are fine for programming and running.

**Upload with USBasp** (from project root; add `-B 32` if needed):

```bash
avrdude -p atmega16 -c usbasp -U flash:w:firmware/old_code/v.1.16/TTesterLCD.hex:i
```

**Upload with Arduino-as-ISP:** use `-c arduino -P /dev/cu.usbmodemXXXX` in place of `-c usbasp`.

`:i` = Intel HEX. After a successful write, disconnect the ISP lines. Power cycle the PCB and check the LCD; you should see the v.1.16 splash/menu if the MCU and LCD wiring are good.

**Optional: read back flash to confirm:**

```bash
avrdude -p atmega16 -c usbasp -U flash:r:flash_readback.hex:i
```

---

## Quick reference: ATmega16 pins used here

| Function | ATmega16 pin number | Port  |
|----------|---------------------|-------|
| RESET    | 9                   | RESET |
| MOSI     | 6                   | PB5   |
| MISO     | 7                   | PB6   |
| SCK      | 8                   | PB7   |
| VCC      | 10                  | —     |
| GND      | 11, 31              | —     |
| LCD RS   | (via J2-4)          | PC2   |
| LCD E    | (via J2-5)          | PC3   |
| LCD D4–D7| (via J2-6…J2-9)     | PC4–PC7 |

**Summary:** With a USBasp-style programmer (e.g. MINI PROG S51 with updated firmware): verify with `-n`, read fuses if desired, upload the hex with `-U flash:w:...`. No fuse changes are required for programming. If the LCD stays blank after a good flash, check supply (7–12 V into 7805) and contrast (VLCD pot).
