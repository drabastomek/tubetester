# VTTester firmware - ATmega32
# Toolchain: avr-gcc + avr-binutils. See firmware/docs/8MHZ_INTERNAL_CLOCK.md for 8 MHz internal clock testing.

MCU     = atmega32
F_CPU   = 16000000

CC      = avr-gcc
OBJCOPY = avr-objcopy
SIZE    = avr-size
CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL -Os -Wall -Wextra
CFLAGS += -funsigned-char -ffunction-sections -fdata-sections -I.
LDFLAGS = -mmcu=$(MCU) -Wl,--gc-sections
COMPILE_CMD = $(CC) $(CFLAGS) -c -o $@ $<
LINK_CMD    = $(CC) $(LDFLAGS) -o $@ $^

# Main app (adjust if your build uses IAR headers; then switch to IAR toolchain)
SRCS    = TTesterLCD32.c
# Config: ROM tube catalog and constants
SRCS   += config/config.c
# Display: LCD output and refresh from buf[]
SRCS   += display/display.c
# Control: menu, katalog, editing, calculations (fills buf[])
SRCS   += control/control.c
# Utils: helper functions (UART, delay, zersrk, liczug1, int2asc)
SRCS   += utils/utils.c
# Protocol (standalone C, no AVR-specific deps in protocol layer)
SRCS   += protocol/vttester_remote.c

# All build artifacts under bin/ (preserves source dir structure: bin/config/config.o etc.)
BINDIR  = bin
OBJS    = $(addprefix $(BINDIR)/, $(SRCS:.c=.o))
TARGET  = TTesterLCD32
ELF     = $(BINDIR)/$(TARGET).elf
HEX     = $(BINDIR)/$(TARGET).hex

.PHONY: all clean flash size test

all: $(HEX)

$(ELF): $(OBJS)
	$(LINK_CMD)

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	@echo "Flash used: $$( $(SIZE) -d $< 2>/dev/null | awk 'NR==2 {print $$1 + $$2}' ) bytes (of 32768). Hex file on disk is larger (Intel HEX overhead)."

$(BINDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(COMPILE_CMD)

clean:
	rm -rf $(BINDIR)

size: $(ELF)
	$(SIZE) $(ELF)

# Host unit tests (protocol parse/send, utils int2asc). No AVR.
test:
	$(MAKE) -C test run

# Uncomment and set PORT/BAUD for your programmer, e.g. usbasp, avrisp
# flash: $(HEX)
#	avrdude -p $(MCU) -c usbasp -P usb -U flash:w:$(HEX):i

# Flash: .text + .data must fit in 32KB. Large const data (catalog_rom, crc8_table) use PROGMEM.
