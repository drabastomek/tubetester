/*
 * VTTester firmware - hardware and application configuration
 * Defines, constants, and tube catalog type. Include after MCU headers.
 */

#ifndef VTTESTER_CONFIG_H
#define VTTESTER_CONFIG_H

#include <stdint.h>

#if defined(ICCAVR)
/* Unify the calls to EEPROM_READ across iccavr and avr-gcc */
#include <string.h>
#define EEPROM_READ(addr, var)   memcpy((void*)&(var), (const void*)(addr), sizeof(var))
#define EEPROM_WRITE(addr, val)  memcpy((void*)(addr), (const void*)&(val), sizeof(val))
#else
#include <avr/eeprom.h>
/* EEPROM API (avr-libc) for use by main and control */
#define EEPROM_READ(addr, var)   eeprom_read_block((void*)&(var), (const void*)(addr), sizeof(var))
#define EEPROM_WRITE(addr, val)  eeprom_write_block((const void*)&(val), (void*)(addr), sizeof(val))
#endif

/* --- MCU / asm helpers --- */
#define BIT(x)  (1<<(x))
#define LOW(x)  ((uint8_t)((uint16_t)(x)))
#define HIGH(x) ((uint8_t)(((uint16_t)(x))>>8))
#define ESC     '\033'
#if defined(ICCAVR)
/* ImageCraft ICCAVR: use lowercase asm mnemonic; NOP2 = short delay for LCD timing */
#define WDR     asm("wdr")
#define SEI     asm("sei")
#define CLI     asm("cli")
#define NOP     asm("nop")
#define NOP2    asm("nop"); asm("nop"); asm("nop"); asm("nop")
#else
#define WDR     asm("WDR")
#define SEI     asm("SEI")
#define CLI     asm("CLI")
#define NOP     asm("NOP")
#define NOP2    asm("RJMP .+2")
#endif

/* --- Port / hardware --- */
#define SET200   PORTB |= BIT(0)
#define RST200   PORTB &= ~BIT(0)
#define SETSEL   PORTB |= BIT(1)
#define RSTSEL   PORTB &= ~BIT(1)
#define ENSET   PORTC |= BIT(3)
#define ENRST   PORTC &= ~BIT(3)
#define RSSET   PORTC |= BIT(2)
#define RSRST   PORTC &= ~BIT(2)
#define DUSK0   ( PIND & BIT(2) ) == 0
#define RIGHT   ( PIND & BIT(6) ) == 0
#define TOPPWM  ICR1
#define PWMUG2  OCR1A
#define PWMUA   OCR1B
#define PWMUH   OCR0
#define SPKON   TCCR2 |= BIT(COM20)
#define SPKOFF  TCCR2 &= ~BIT(COM20)
#define CLKUG1SET PORTB |= BIT(2)
#define CLKUG1RST PORTB &= ~BIT(2)

/* --- Timing / UART --- */
/* KWARC = crystal/clock Hz. RATE = UBRR for 9600 baud: F_CPU/(16*9600)-1. At 16 MHz: 103; at 8 MHz: 51. See docs/8MHZ_INTERNAL_CLOCK.md */
#define KWARC   16000000
#define RATE    103
#define MS1     250
#define MRUG    250
#define DMIN    20
#define DMAX    250
#define MS250   250
#define LPROB   63

/* --- ADC / alarms --- */
#define HITEMP  10240
#define LOTEMP  8960
#define OVERSAMP 2
#define OVERIH  0b00000001
#define OVERIA  0b00000010
#define OVERIG  0b00000100
#define OVERTE  0b00001000
#define OVERTX  0b00010000
#define ADRREZ  0b00000000
#define ADRIH   0b00000001
#define ADRUH   0b00000010
#define ADRUA   0b00000011
#define ADRIA   0b00000100
#define ADRUG2  0b00000101
#define ADRIG2  0b00000110
#define ADRUG1  0b00000111

/* --- Catalog sizes --- */
#define FLAMP   (uint16_t)(1+1+357)
#define ELAMP   (uint16_t)(39)

/* --- VTTester protocol (SET param limits; #define to save RAM) --- */
#define VTTESTER_SET_HEAT_IDX_MAX    7   /* P1 heater index: 0..7 only */
#define VTTESTER_SET_UA_MAX          300 /* Ua: 10 V per step (P2*10), max 300 V */
#define VTTESTER_SET_UG2_MAX         300 /* Ug2: 10 V per step (P3*10), max 300 V */
#define VTTESTER_SET_UG1_MAX         240 /* Ug1 magnitude in 0.1V (240 = -24 V); ug1def = P4*UG1_P4_STEP, same as old code */
#define VTTESTER_SET_TUH_INDEX_MAX   63 /* heating time index 0..63 (500ms per step) -> tuh_ticks = index*TUH_TICK_SCALE */
#define VTTESTER_SET_UA_SCALE        10  /* P2/P3: 10 V per step (decoded value in volts) */
#define VTTESTER_SET_UG1_P4_STEP     5   /* P4 -> ug1def in 0.1V magnitude: ug1def = P4*this (0.5 V per P4 step) */
#define VTTESTER_SET_TUH_TICK_SCALE  2   /* tuh index -> tuh_ticks (2 ticks per 500ms step) */

/* --- Sequencer timing --- */
#define TMAR    2
#define TUA     8
#define TUG2    8
#define TUG     16
#define TUH     8
#define FUA     2
#define FUG2    2
#define FUG     2
#define FUH     2
#define BIP     4

/* --- Tube catalog entry --- */
typedef struct
{
   uint8_t nazwa[9];
   uint8_t uhdef;
   uint8_t ihdef;
   uint8_t ug1def;
   uint16_t  uadef;
   uint16_t  iadef;
   uint16_t  ug2def;
   uint16_t  ig2def;
   uint16_t  sdef;
   uint16_t  rdef;
   uint16_t  kdef;
} katalog;

/* ROM tube catalog: PROGMEM only for avr-gcc firmware; host tests and ICCAVR use RAM. */
#if defined(ICCAVR) || defined(VTTESTER_HOST_TEST)
#define KATALOG_PROGMEM
#else
#define KATALOG_PROGMEM __attribute__((__progmem__))
#endif
extern const katalog lamprom[] KATALOG_PROGMEM;
void load_lamprom(uint16_t idx, katalog *dest);

/* Lookup / tables (defined in config.c) */
extern const uint8_t AZ[37];
extern uint8_t cyrZ[8], cyrG[8], cyrB[8], cyrD[8], cyrI[8], cyrP[8], cyrC[8], cyrF[8], cyrL[8], cyrE[8];

/* EEPROM (defined in config.c) */
extern uint16_t poptyp;
extern katalog lampeep[ELAMP];

#endif /* VTTESTER_CONFIG_H */
