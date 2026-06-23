# Protocol test harness — ATmega32A (UART stub for desktop testing).
# Builds comms_test_avr.hex from test_protocol_harness/ + communication/ + config/.

MCU     = atmega32a
F_CPU   = 16000000
TARGET  = comms_test_avr
BINDIR  = bin
ELF     = $(BINDIR)/$(TARGET).elf
HEX     = $(BINDIR)/$(TARGET).hex

HARNESS_DIR = test_protocol_harness

CC      = avr-gcc
OBJCOPY = avr-objcopy
SIZE    = avr-size

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL -Os -Wall -Wextra -g
CFLAGS += -I. -Iconfig -Icommunication -I$(HARNESS_DIR)
CFLAGS += -funsigned-char
CFLAGS += -ffunction-sections -fdata-sections
LDFLAGS = -Wl,--gc-sections

# Harness flow: split (default) or auto-measure after SET.
HARNESS_FLOW ?= split
ifeq ($(HARNESS_FLOW),auto)
CFLAGS += -DHARNESS_FLOW=HARNESS_FLOW_AUTO_MEASURE
else
CFLAGS += -DHARNESS_FLOW=HARNESS_FLOW_SPLIT
endif

ifneq ($(HARNESS_MEAS_DELAY_MS),)
CFLAGS += -DHARNESS_MEAS_DELAY_MS=$(HARNESS_MEAS_DELAY_MS)
endif

ifneq ($(HARNESS_TX_DELAY_MS),)
CFLAGS += -DHARNESS_TX_DELAY_MS=$(HARNESS_TX_DELAY_MS)
endif

ifeq ($(COMMS_DEBUG_BOOT),1)
CFLAGS += -DCOMMS_DEBUG_BOOT
endif

HARNESS_SRCS = \
	$(HARNESS_DIR)/main.c \
	$(HARNESS_DIR)/harness_command.c \
	$(HARNESS_DIR)/harness_tick.c \
	$(HARNESS_DIR)/scenario.c \
	$(HARNESS_DIR)/synthetic_measure.c

HARNESS_OBJS = $(HARNESS_SRCS:$(HARNESS_DIR)/%.c=$(BINDIR)/harness_%.o)
COMMON_OBJS  = $(BINDIR)/config.o $(BINDIR)/communication.o
OBJ            = $(HARNESS_OBJS) $(COMMON_OBJS)

.PHONY: all clean size flash flash-slow probe chip-prep read-fuses hex

all: $(ELF)

hex: $(HEX)

$(BINDIR):
	@mkdir -p $(BINDIR)

$(ELF): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ)
	@$(SIZE) $@

$(BINDIR)/harness_%.o: $(HARNESS_DIR)/%.c | $(BINDIR)
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

flash-slow: $(HEX)
	avrdude -p $(MCU) -c usbasp -P usb -B 32 -U flash:w:$(HEX):i

probe:
	avrdude -p $(MCU) -c usbasp -P usb -B 32 -F -v

chip-prep:
	avrdude -p $(MCU) -c usbasp -P usb -B 32 -U lfuse:w:0x3F:m -U hfuse:w:0xCF:m

read-fuses:
	avrdude -p $(MCU) -c usbasp -P usb -B 32 -U lfuse:r:-:h -U hfuse:r:-:h

clean:
	rm -f $(ELF) $(HEX) $(OBJ)
	@rmdir $(BINDIR) 2>/dev/null || true
