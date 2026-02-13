# VTTester firmware - ATmega32
# Toolchain: make [TOOLCHAIN=gcc]   -> avr-gcc (default)
#            make TOOLCHAIN=iccavr  -> ImageCraft ICCAVR (add -DICCAVR, see docs/ICCAVR_PORTING.md)
# Requires avr-gcc + avr-binutils for TOOLCHAIN=gcc. For iccavr, have iccavr in PATH or set ICCAVR_HOME.

MCU     = atmega32
F_CPU   = 16000000
TOOLCHAIN ?= gcc

# --- Toolchain selection ---
ifeq ($(TOOLCHAIN),iccavr)
  ICCAVR_PREFIX  := $(if $(ICCAVR_HOME),$(ICCAVR_HOME)/,)
  CC             = $(ICCAVR_PREFIX)iccavr
  OBJCOPY        = avr-objcopy
  SIZE           = avr-size
  # MCU_DEF for ICCAVR: default ATMEGA32; set to ATMega32 if your IDE uses that (e.g. ICCAVR_MCU_DEF=ATMega32)
  ICCAVR_MCU_DEF ?= $(shell echo $(MCU) | tr a-z A-Z)
  CFLAGS         = -c -DICCAVR -D$(ICCAVR_MCU_DEF) -DF_CPU=$(F_CPU)UL -I.
  CFLAGS        += -e -g -MLongJump -MHasMul -MEnhanced
  LDFLAGS        = -o
  LDFLAGS_POST   = -m$(ICCAVR_MCU_DEF) -g
  # ICCAVR may need -L and -l for lib path; set LDFLAGS_ICCAVR_EXTRA if needed
  LDFLAGS_ICCAVR_EXTRA ?=
  COMPILE_CMD = $(CC) $(CFLAGS) $< -o $@
  LINK_CMD    = $(CC) $(LDFLAGS) $@ $^ $(LDFLAGS_POST) $(LDFLAGS_ICCAVR_EXTRA)
else
  CC      = avr-gcc
  OBJCOPY = avr-objcopy
  SIZE    = avr-size
  CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL -Os -Wall -Wextra
  CFLAGS += -funsigned-char
  CFLAGS += -ffunction-sections -fdata-sections -I.
  LDFLAGS = -mmcu=$(MCU) -Wl,--gc-sections
  LDFLAGS_POST   =
  LDFLAGS_ICCAVR_EXTRA =
  COMPILE_CMD = $(CC) $(CFLAGS) -c -o $@ $<
  LINK_CMD    = $(CC) $(LDFLAGS) -o $@ $^
endif

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

# Flash: .text + .data must fit in 32KB. Large const data (lamprom, crc8_table) use PROGMEM.
#
# ICCAVR (Windows): set ICCAVR_HOME to the compiler dir if iccavr not in PATH, e.g. make TOOLCHAIN=iccavr ICCAVR_HOME=C:/Users/Tomek/iccv8avr
