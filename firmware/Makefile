# comms-testing — UART harness only (communication/ + minimal config).
# Friend’s full tree replaces config/ and this main; keep communication/ as the shared API.

MCU     = atmega32a
F_CPU   = 16000000
TARGET  = comms_test_avr
BINDIR  = bin
ELF     = $(BINDIR)/$(TARGET).elf
HEX     = $(BINDIR)/$(TARGET).hex
OBJ     = $(BINDIR)/TTesterLCD32.o $(BINDIR)/config.o $(BINDIR)/communication.o

CC      = avr-gcc
OBJCOPY = avr-objcopy
SIZE    = avr-size

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL -Os -Wall -Wextra -g
CFLAGS += -I. -Iconfig -Icommunication
CFLAGS += -funsigned-char
CFLAGS += -ffunction-sections -fdata-sections
LDFLAGS = -Wl,--gc-sections

.PHONY: all clean size flash flash-slow probe chip-prep read-fuses hex

all: $(ELF)

hex: $(HEX)

$(BINDIR):
	@mkdir -p $(BINDIR)

$(ELF): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ)
	@$(SIZE) $@

$(BINDIR)/TTesterLCD32.o: TTesterLCD32.c config/config.h communication/communication.h | $(BINDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINDIR)/config.o: config/config.c config/config.h | $(BINDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINDIR)/communication.o: communication/communication.c communication/communication.h | $(BINDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@

size: $(ELF)
	$(SIZE) $(ELF)

flash: $(HEX)
	avrdude -p $(MCU) -c usbasp -P usb -B 8 -U flash:w:$(HEX):i

probe:
	avrdude -p $(MCU) -c usbasp -P usb -B 32 -F -v

chip-prep:
	avrdude -p $(MCU) -c usbasp -P usb -B 32 -U lfuse:w:0x3F:m -U hfuse:w:0xCF:m

read-fuses:
	avrdude -p $(MCU) -c usbasp -P usb -B 32 -U lfuse:r:-:h -U hfuse:r:-:h

clean:
	rm -f $(ELF) $(HEX) $(OBJ)
	@rmdir $(BINDIR) 2>/dev/null || true
