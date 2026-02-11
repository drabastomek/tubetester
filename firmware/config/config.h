/*
 * VTTester firmware - hardware and application configuration
 * Defines, constants, and tube catalog type. Include after MCU headers.
 */

#ifndef VTTESTER_CONFIG_H
#define VTTESTER_CONFIG_H

/* --- MCU / asm helpers --- */
#define BIT(x)  (1<<(x))
#define LOW(x)  ((char)((int)(x)))
#define HIGH(x) ((char)(((int)(x))>>8))
#define ESC     '\033'
#define WDR     asm("WDR")
#define SEI     asm("SEI")
#define CLI     asm("CLI")
#define NOP     asm("NOP")
#define NOP2    asm("RJMP .+2")

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
#define FLAMP   (unsigned int)(1+1+357)
#define ELAMP   (unsigned int)(39)

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
   unsigned char nazwa[9];
   unsigned char uhdef;
   unsigned char ihdef;
   unsigned char ug1def;
   unsigned int  uadef;
   unsigned int  iadef;
   unsigned int  ug2def;
   unsigned int  ig2def;
   unsigned int  sdef;
   unsigned int  rdef;
   unsigned int  kdef;
} katalog;

/* ROM tube catalog in flash (PROGMEM on AVR); use load_lamprom() to read */
#ifdef __AVR__
#define KATALOG_PROGMEM __attribute__((__progmem__))
#else
#define KATALOG_PROGMEM
#endif
extern const katalog lamprom[] KATALOG_PROGMEM;
void load_lamprom(unsigned int idx, katalog *dest);

/* Lookup / tables (defined in config.c) */
extern const unsigned char AZ[37];
extern char cyrZ[8], cyrG[8], cyrB[8], cyrD[8], cyrI[8], cyrP[8], cyrC[8], cyrF[8], cyrL[8], cyrE[8];

/* EEPROM (defined in config.c) */
extern unsigned int poptyp;
extern katalog lampeep[ELAMP];

#endif /* VTTESTER_CONFIG_H */
