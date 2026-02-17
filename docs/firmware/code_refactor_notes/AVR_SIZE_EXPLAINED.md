# Understanding `avr-size` output

## Default output: `avr-size TTesterLCD32.elf`

```
   text	   data	    bss	    dec	    hex	filename
  15322	   1016	    333	  16671	   411f	TTesterLCD32.elf
```

| Column | Meaning | Where it lives on AVR |
|--------|--------|------------------------|
| **text** | Code + constants in program memory (e.g. PROGMEM data). | **Flash** – this is your executable code and read-only data (e.g. `lamprom`, `crc8_table`). |
| **data** | Initialized RAM data. The *initial values* are stored in flash and copied to RAM at startup. | **Flash** (copy of initializers) + **RAM** (runtime copy). |
| **bss** | Uninitialized (zero-initialized) RAM data. No bytes stored in the .hex. | **RAM** only. |
| **dec** | Total: `text + data + bss` (overall size of the image, not a single memory type). | — |
| **hex** | Same total in hexadecimal. | — |

### What actually gets programmed

- **Flash usage** = `text + data`  
  That’s what ends up in the chip’s 32 KB flash (code + const + initializers for .data).
- **RAM usage** = `data + bss`  
  That’s what the chip’s 2 KB RAM must hold at runtime.

So for your firmware:

- Flash used ≈ **text + data** → fits in 32 KB.
- RAM used ≈ **data + bss** → must fit in 2 KB.

The **.hex file size on disk** (e.g. 42 KB) is larger than flash usage because of Intel HEX record headers and formatting; the programmer only writes the payload (flash bytes) into the device.

---

## Detailed output: `avr-size -A TTesterLCD32.elf`

Lists each **section** in the ELF:

| Section   | Meaning |
|----------|---------|
| `.text`  | Code + constants in flash (same as “text” above). |
| `.data`  | Initialized data (loaded from flash into RAM at startup). |
| `.bss`   | Zero-initialized RAM (no space in .hex). |
| `.eeprom`| EEPROM contents (often excluded from the flash .hex with `-R .eeprom`). |
| `.comment` / `.note.*` | Metadata. |
| `.debug_*` | Debug info (not programmed to the chip). |

**addr** is the section’s address (e.g. 0 = start of flash; 0x800000-style = RAM on AVR).

---

## Quick check

```bash
avr-size -d TTesterLCD32.elf
```

Then: **Flash ≈ first column + second column**, **RAM ≈ second column + third column**.
