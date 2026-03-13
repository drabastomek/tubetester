# VTTester firmware — avr-gcc build
# Output: firmware/bin/*.elf, *.hex

MCU     = atmega32
F_CPU   = 16000000
SRC     = TTesterLCD32.c config/config.c config/catalog.c
TARGET  = TTesterLCD32_avr
BINDIR  = bin
ELF     = $(BINDIR)/$(TARGET).elf
HEX     = $(BINDIR)/$(TARGET).hex
OBJ     = $(BINDIR)/TTesterLCD32.o $(BINDIR)/config.o $(BINDIR)/utils.o $(BINDIR)/display.o $(BINDIR)/interrupts.o

CC      = avr-gcc
OBJCOPY = avr-objcopy
SIZE    = avr-size

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL -Os -Wall -Wextra -g
CFLAGS += -I. -Iconfig -Iutils -Idisplay -Iinterrupts
CFLAGS += -funsigned-char
CFLAGS += -ffunction-sections -fdata-sections
LDFLAGS = -Wl,--gc-sections

.PHONY: all clean size flash flash-slow hex
DEFAULT_GOAL := all

all: $(ELF)

hex: $(HEX)

$(BINDIR):
	@mkdir -p $(BINDIR)

$(ELF): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ)
	@$(SIZE) $@

$(BINDIR)/TTesterLCD32.o: TTesterLCD32.c config/config.h | $(BINDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINDIR)/config.o: config/config.c config/config.h | $(BINDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINDIR)/utils.o: utils/utils.c utils/utils.h | $(BINDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINDIR)/display.o: display/display.c display/display.h | $(BINDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINDIR)/interrupts.o: interrupts/interrupts.c interrupts/interrupts.h | $(BINDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@

size: $(ELF)
	$(SIZE) $(ELF)

flash: $(HEX)
	avrdude -p $(MCU) -c usbasp -P usb -B 2 -U flash:w:$(HEX):i

# Slower clock if "target does not answer" (long wires / marginal connection)
flash-slow: $(HEX)
	avrdude -p $(MCU) -c usbasp -P usb -B 32 -U flash:w:$(HEX):i

clean:
	rm -f $(ELF) $(HEX) $(OBJ)
	@rmdir $(BINDIR) 2>/dev/null || true
