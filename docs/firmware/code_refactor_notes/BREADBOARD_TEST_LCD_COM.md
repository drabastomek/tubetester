# Breadboard test: LCD + serial protocol only

Minimal setup to run the firmware on a breadboard with a 16 MHz crystal, an HD44780 4×20 LCD, and serial (9600 8N1) for the VTTester protocol. Assumes ATmega32 and the existing code; you can omit PWM, encoder, and ADC wiring for a “display + comms” test if the code still boots (see note below).

---

## What you need

| Item | Notes |
|------|--------|
| **ATmega32** | DIP-40 or whatever package you use. |
| **16 MHz crystal** | Same as the one you desoldered. |
| **2 × 22 pF capacitors** | Crystal load caps (one from each crystal pin to GND). |
| **5 V supply** | USB, bench supply, or regulator. Don’t rely on the Arduino to power the breadboard (see below). |
| **HD44780 4×20 LCD** | 4-bit mode: D4–D7, RS, E, VCC, GND, V0 (contrast). |
| **USB‑serial adapter** | CP2102, CH340, FT232, etc. (3.3 V or 5 V logic). For **serial comms** to the ATmega32 (9600 8N1). |
| **Arduino Uno (or similar)** | Used **only as an ISP programmer** to flash the ATmega32. |
| **Wires, breadboard** | As needed. |

You do **not** need the full VTTester analog front‑end, encoder, or high‑voltage parts for this test.

---

## Arduino Uno: programmer vs serial

- **Arduino Uno as programmer (ISP): yes.**  
  Load the **ArduinoISP** sketch, connect the 6 ISP lines (RESET, MOSI, MISO, SCK, GND; see below), and use avrdude to flash the ATmega32. This works well.

- **Arduino Uno as serial link to the ATmega32: awkward.**  
  The Uno’s USB serial is tied to the 328P’s UART. When the Uno is running ArduinoISP, that UART is not a transparent bridge to the outside. To talk 9600 to the ATmega32 you’d have to either reflash the Uno with a serial passthrough sketch or use a second device. So in practice:

**Recommended:** Use the **Uno only as ISP**. Use a **separate USB‑serial adapter** (CP2102/CH340/FT232) connected to the ATmega32’s UART for the VTTester protocol. That way you don’t reflash the Uno and you get a dedicated COM port for the Python backend or terminal.

---

## Wiring

### Power and clock (breadboard)

- **5 V** and **GND** to the ATmega32 (and LCD, and USB‑serial adapter GND). Power the breadboard from its own supply (e.g. USB cable to a 5 V rail); do **not** power the ATmega32 from the Uno’s 5 V when using the Uno as ISP (to avoid backfeeding).
- **16 MHz crystal** between XTAL1 and XTAL2; 22 pF from each crystal pin to GND.
- **AVCC** and **AREF** tied to 5 V (or as in the original schematic) if you have them; GND to common ground.

### LCD (4‑bit, HD44780)

From the firmware (PORTC and display code):

| LCD pin | ATmega32 | Notes |
|---------|----------|--------|
| D4      | PC4      | |
| D5      | PC5      | |
| D6      | PC6      | |
| D7      | PC7      | |
| RS      | PC2      | |
| E       | PC3      | |
| VCC     | 5 V      | |
| GND     | GND      | |
| V0       | 10 kΩ pot between 5 V and GND (contrast) | |

### UART (serial for protocol)

ATmega32 USART is on **PD0 (RXD)** and **PD1 (TXD)**:

| ATmega32 | USB‑serial adapter | Notes |
|----------|--------------------|--------|
| PD0 (RXD) | **TX** of adapter  | MCU receives from PC |
| PD1 (TXD) | **RX** of adapter  | MCU sends to PC |
| GND      | GND                | Common ground |

Use 9600 8N1. If the adapter is 3.3 V, check that the ATmega32 is 5 V (or use a 3.3 V ATmega32); 5 V MCU → 3.3 V adapter RX may need a divider or a 5 V‑tolerant adapter.

### Arduino Uno as ISP programmer

1. Load **ArduinoISP** in the Arduino IDE (File → Examples → 11.ArduinoISP → ArduinoISP) and upload it to the Uno.
2. Wire the Uno to the ATmega32 (target) **only for programming**:

   | Arduino Uno (ISP) | ATmega32 |
   |-------------------|----------|
   | 10                | RESET    |
   | 11                | MOSI (PD2 on ATmega32) |
   | 12                | MISO (PD3) |
   | 13                | SCK (PD4)  |
   | GND               | GND       |

   Do **not** connect 5 V from the Uno to the target; power the target from the breadboard supply. (You can share GND.)

3. Flash with avrdude, for example:
   ```bash
   avrdude -p atmega32 -c arduino -P /dev/cu.usbmodem* -U flash:w:bin/TTesterLCD32.hex:i
   ```
   On Windows use something like `COM3` for `-P`. After flashing, disconnect the ISP lines (and optionally the Uno); keep the USB‑serial adapter connected for comms.

---

## After power‑up

1. The LCD should show the usual splash / menu (or whatever the firmware draws at init). If it stays blank, check F_CPU, Timer2, and wiring in `config/config.h` and the firmware docs.
2. Open the COM port of the **USB‑serial adapter** at **9600 8N1** (Serial Monitor, or the Python backend from `src/backend`). Send a valid 8‑byte SET or MEAS frame (with CRC) to test the protocol; the firmware should respond with ACK or measurement.

---

## Optional: “LCD + UART only” without full hardware

The main loop and init assume timers, ADC, and some I/O. For a minimal “LCD + UART” run you still need:

- Correct **F_CPU** (16 MHz) and **RATE** (103) in config so the UART and Timer2 (used by `delay()`) are correct.
- **Timer2** running so `delay()` and the sync tick work; otherwise the first `delay(30)` in the LCD init can hang and the display stays blank.

If you leave encoder, ADC, or PWM unconnected, the firmware may still run and only those features will behave oddly; that’s often enough to confirm LCD and serial protocol on the breadboard.
