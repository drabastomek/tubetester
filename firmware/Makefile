# VTTester firmware - ATmega32
# Requires avr-gcc, avr-binutils. Optional: avrdude for flash.

MCU     = atmega32
F_CPU   = 16000000

CC      = avr-gcc
OBJCOPY = avr-objcopy
SIZE    = avr-size

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL -Os -Wall -Wextra
CFLAGS += -ffunction-sections -fdata-sections -I.
LDFLAGS = -mmcu=$(MCU) -Wl,--gc-sections

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

# Object files: same dir as sources (protocol/vttester_remote.o)
OBJS    = $(SRCS:.c=.o)
TARGET  = TTesterLCD32
HEX     = $(TARGET).hex
ELF     = $(TARGET).elf

.PHONY: all clean flash size

all: $(HEX)

$(ELF): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	@echo "Flash used: $$( $(SIZE) -d $< | awk 'NR==2 {print $$1 + $$2}' ) bytes (of 32768). Hex file on disk is larger (Intel HEX overhead)."

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(ELF) $(HEX)

size: $(ELF)
	$(SIZE) $(ELF)

# Uncomment and set PORT/BAUD for your programmer, e.g. usbasp, avrisp
# flash: $(HEX)
#	avrdude -p $(MCU) -c usbasp -P usb -U flash:w:$(HEX):i

# Flash: .text + .data must fit in 32KB. Large const data (lamprom, crc8_table) use PROGMEM.
