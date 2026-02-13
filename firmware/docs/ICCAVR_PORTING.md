# Building with ImageCraft ICCAVR (Windows)

The firmware is developed and tested with **avr-gcc** (GNU toolchain). Building with **ImageCraft ICCAVR** on Windows can produce a larger image and, without a few adaptations, the **LCD may stay blank**. This document explains why and what is already in the tree to fix it.

---

## Why the LCD might show nothing

1. **Inline assembly**  
   The code uses GCC-style `asm("WDR")`, `asm("NOP")`, `asm("RJMP .+2")` in `config/config.h` (macros `WDR`, `NOP2`). These are used in the LCD driver for timing and in the main loop for the watchdog. If ICCAVR does not accept this syntax or generates wrong code, the MCU can reset repeatedly, hang, or send wrong signals to the LCD.

2. **Flash (PROGMEM) and EEPROM (EEMEM)**  
   The project uses **avr-libc** APIs: `PROGMEM`, `memcpy_P()`, `pgm_read_byte()`, `EEMEM`, `eeprom_read_block()` / `eeprom_write_block()`. ICCAVR uses different mechanisms; wrong or missing implementations can cause crashes before or during `display_init()` or bad data from `load_lamprom()` / EEPROM.

3. **Headers**  
   `<avr/eeprom.h>` and `<avr/pgmspace.h>` are GCC/avr-libc specific. The compatibility blocks avoid including them when **`ICCAVR`** is defined.

4. **char signedness**  
   ICCAVR treats plain **`char`** as **unsigned**; avr-gcc defaults to **signed**. Shifts and comparisons on `char` can then differ. The main Makefile uses **`-funsigned-char`** for avr-gcc so behaviour matches ICCAVR. For new code, prefer **`uint8_t`** / **`int8_t`** explicitly.

5. **ISR‑shared variables**  
   Variables written in an ISR and read in main (or the other way around) must be **`volatile`**, or the compiler may optimize away reads. In **TTesterLCD32.c**, `busy`, `sync`, `zwloka`, `irx`, `tout`, `crc`, `rx_proto_pos`, `rx_proto_ready` are declared **`volatile`** for this reason.

---

## What is already in the source (when you define ICCAVR)

The repo contains **`#ifdef ICCAVR`** blocks so that a single define is enough for a build that should show something on the LCD.

### 1. Define ICCAVR when building

**Option A — Makefile (if you build from the repo with `make`):**  
Use the ICCAVR toolchain and add **`-DICCAVR`** automatically:

```bash
make TOOLCHAIN=iccavr
```

If `iccavr` is not in your PATH, set **`ICCAVR_HOME`** to the compiler directory (no trailing slash), e.g.:

```bash
make TOOLCHAIN=iccavr ICCAVR_HOME=C:/Users/Tomek/iccv8avr
```

Output is still **`bin/TTesterLCD32.elf`** and **`bin/TTesterLCD32.hex`**. You need **avr-objcopy** and **avr-size** (e.g. from MinGW/AVR toolchain) to generate the hex and size report.

**Option B — ImageCraft IDE:**  
In the project options, add **`ICCAVR`** to the compiler preprocessor defines (e.g. **Preprocessor** / **Defines**: `ICCAVR`). Rebuild.

### 2. What the compatibility does

- **config/config.h**  
  - Does **not** include `<avr/eeprom.h>` when ICCAVR is defined.  
  - **EEPROM_READ** / **EEPROM_WRITE** are defined as **memcpy** to/from the given address (so EEPROM is “simulated” in RAM for this build).  
  - **WDR**, **NOP**, **NOP2**, **SEI**, **CLI** use **lowercase** asm mnemonics (`asm("wdr")`, `asm("nop")`, etc.) and **NOP2** is expanded to four NOPs for LCD timing.  
  - **KATALOG_PROGMEM** is empty so **lamprom** is not placed in GCC PROGMEM.

- **config/config.c**  
  - Does **not** include `<avr/eeprom.h>` or `<avr/pgmspace.h>` when ICCAVR is defined.  
  - **poptyp** and **lampeep** are normal (RAM) variables so **EEPROM_READ** / **EEPROM_WRITE** as memcpy work; EEPROM is not persisted across power cycle in this build.  
  - **lamprom** is a normal const array (in flash if the compiler puts const in flash, else in RAM).  
  - **load_lamprom()** uses **memcpy** instead of **memcpy_P** when ICCAVR is defined.

- **protocol/vttester_remote.c**  
  - **crc8_table** is in RAM and **crc8_compute()** reads it directly (no PROGMEM / no **pgm_read_byte**) when ICCAVR is defined.

- **MCU and interrupt headers (same as original ImageCraft project)**  
  When **ICCAVR** is defined, the firmware uses ImageCraft’s headers and interrupt style instead of avr-gcc’s:  
  - **`<iom32v.h>`** is included instead of **`<avr/io.h>`** in **TTesterLCD32.c**, **display/display.c**, **control/control.c**, **utils/utils.c**. This matches the old working code (`old_code/TTesterLCD32_2.06.c`) and ensures correct register and port definitions for the ATmega32.  
  - **Interrupts** are declared with **`#pragma interrupt_handler name:vector`** and the handler names **ext_int1**, **timer2_comp**, **usart_rxc**, **usart_txc**, **adc** (vector numbers 3, 5, 14, 16, 17). The same ISR bodies are compiled for both toolchains; only the declaration and name differ.  
  - **`<avr/interrupt.h>`** is not used for ICCAVR; the pragmas take its place.

With these changes, the code should link and run without avr-libc’s eeprom/pgmspace, and the LCD should get valid timing from the asm macros and correct I/O from **iom32v.h**.

### 3. If the LCD still does not show

- **Asm syntax**  
  If your ICCAVR version expects different inline asm (e.g. **`#asm`** / **`#endasm`** or a different mnemonic format), edit the **`#if defined(ICCAVR)`** block in **config/config.h** (WDR, NOP2) to match.

- **Headers**  
  The ICCAVR build uses **`<iom32v.h>`** (and **`#pragma interrupt_handler`** for ISRs), matching the original ImageCraft project. If your IDE or target uses a different device header (e.g. **`<iom32.h>`** or **`<ioatmega32.h>`**), change the **`#if defined(ICCAVR)`** blocks that include **`<iom32v.h>`** in **TTesterLCD32.c**, **display/display.c**, **control/control.c**, **utils/utils.c**.

- **Debug**  
  Use a debugger to confirm **main()** reaches **display_init()** and that **cmd2lcd** runs (watchdog and NOP2 execute) without reset or hang.

### 4. LCD blank – checklist (avr-gcc and ICCAVR)

If the LCD stays blank on real hardware with **either** toolchain, work through this list:

| Check | What to verify |
|-------|----------------|
| **Wiring** | LCD data D4–D7 = PORTC PC4–PC7, EN = PC3, RS = PC2. Schematic must match; wrong port/pins = no valid signals. |
| **Power-on delay** | HD44780 often needs **≥40 ms** after Vcc before the first command. The firmware uses ~50 ms; if you changed it, keep it at least 40 ms. |
| **Timer / delay()** | `delay()` relies on the TIMER2 compare ISR decrementing `zwloka`. If the timer is not running (wrong F_CPU, timer not started, or interrupts disabled), the code can **hang in the first delay(30)** and the LCD never gets any commands → blank. Confirm F_CPU matches the actual crystal (e.g. 16 MHz) and that **SEI** runs before **display_init()**. |
| **F_CPU / fuses** | If the MCU runs at 8 MHz but F_CPU is 16 MHz (or the opposite), init timing and all delays scale wrong; some panels are marginal. |
| **Contrast / backlight** | V0 and backlight wiring can make the panel look blank even when driven correctly. |
| **Watchdog** | If the WDT timeout is shorter than the time from reset to the end of **display_init()**, the MCU can reset in a loop and the LCD never finishes init. The firmware kicks the watchdog inside **delay()** so long inits are safe; ensure you don’t shorten the WDT period without that. |
| **4-bit init** | The code uses 0x28 (4-bit, 2 lines) directly. Some modules want a “wake” sequence (e.g. 0x33, 0x32) first; if the above are OK and the LCD still does nothing, try adding that before 0x28. |

### 5. ROM size

ICCAVR often produces slightly more code than avr-gcc. **ROM 83% (27013 bytes)** is still within the ATmega32 32 KB limit.

### 6. Optional: real EEPROM and flash later

For production you may want real EEPROM persistence and **lamprom** in flash to save RAM. Then:

- Implement **EEPROM_READ** / **EEPROM_WRITE** with ICCAVR’s EEPROM API and place **poptyp** / **lampeep** in EEPROM (e.g. **`#pragma data: eeprom`** or ICCAVR’s attribute).  
- Place **lamprom** in flash with ICCAVR’s method and implement **load_lamprom()** with their read-from-flash API.  
- Optionally put **crc8_table** in flash and use their flash-read in **crc8_compute()** to reduce RAM.

The current **ICCAVR** build trades “no real EEPROM + lamprom/crc in RAM” for minimal changes and a working LCD.
