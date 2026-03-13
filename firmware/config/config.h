#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <avr/eeprom.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/* EEPROM helpers (macros defined in config.c; declare here so main can use after including avr/eeprom.h) */
#ifndef EEPROM_READ
#define EEPROM_READ(addr, var)          ((var) = eeprom_read_word((const uint16_t*)(uint16_t)(addr)))
#define EEPROM_READ_BLOCK(addr, p, sz)  eeprom_read_block((void*)(p), (const void*)(uint16_t)(addr), (size_t)(sz))
#define EEPROM_WRITE(addr, val)         eeprom_update_word((uint16_t*)(uint16_t)(addr), (uint16_t)(val))
#define EEPROM_WRITE_BYTE(addr, val)    eeprom_update_byte((uint8_t*)(uint16_t)(addr), (uint8_t)(val))
#endif

#define BIT(x)  (1<<(x))
#define ESC     '\033'
#define WDR     __asm__ __volatile__("WDR")
#define SEI     __asm__ __volatile__("SEI")
#define CLI     asm("CLI")
#define NOP     __asm__ __volatile__("nop")
#define NOP2    asm("RJMP .+2")

#define SET200  PORTB |= BIT(0)          // ustaw zakres 200mA
#define RST200  PORTB &= ~BIT(0)          // ustaw zakres 40mA

#define SETSEL  PORTB |= BIT(1)                   // ustaw UA2
#define RSTSEL  PORTB &= ~BIT(1)                  // ustaw UA1

#define DUSK0   ( PIND & BIT(2) ) == 0        // K0 nacisniety
#define RIGHT   ( PIND & BIT(6) ) == 0        // obrot w prawo

#define TOPPWM  ICR1                      // gorna granica PWM
#define PWMUG2  OCR1A                               // PWM Ug2
#define PWMUA   OCR1B                                // PWM Ua
#define PWMUH   OCR0                                // 8bit Uh

#define SPKON   TCCR2 |= BIT(COM20)               // wlacz ton
#define SPKOFF  TCCR2 &= ~BIT(COM20)             // wylacz ton

#define CLKUG1SET PORTB |= BIT(2)              // ustaw CLKUG1
#define CLKUG1RST PORTB &= ~BIT(2)             // kasuj CLKUG1

#define RATE    103                              // 9600,N,8,1
#define MS1     250                      // podstawa czasu 1ms
#define MRUG    250                      // taktowanie migania
#define DMIN    20                          // czas drgan 20ms
#define DMAX    250                        // czas drgan 250ms
#define MS250   250           // min czas miedzy ramkami 250ms
#define LPROB   63            // liczba usrednianych probek 64
#define LCD_RS PC2
#define LCD_E  PC3

#define HITEMP  10240         // 80"C = 0.8V/5.12V * 1024 * 64
#define LOTEMP  8960          // 70"C = 0.7V/5.12V * 1024 * 64

#define OVERSAMP 2              // liczba probek do alarmu - 1
#define OVERIH  0b00000001                // znacznik bledu IH
#define OVERIA  0b00000010                // znacznik bledu IA
#define OVERIG  0b00000100               // znacznik bledu IG2
#define OVERTE  0b00001000     // znacznik wzrostu temperatury
#define OVERTX  0b00010000   // znacznik bledu odebranej ramki

#define ADRREZ  0b00000000                // adres REZ w ADMUX
#define ADRIH   0b00000001                 // adres IH w ADMUX
#define ADRUH   0b00000010                 // adres UH w ADMUX
#define ADRUA   0b00000011                 // adres UA w ADMUX
#define ADRIA   0b00000100                 // adres IA w ADMUX
#define ADRUG2  0b00000101                // adres UG2 w ADMUX
#define ADRIG2  0b00000110                // adres IG2 w ADMUX
#define ADRUG1  0b00000111                // adres UG1 w ADMUX

#define FLAMP   (uint16_t)(1+1+88)    // PwrSup+Remote+98
#define ELAMP   (uint16_t)(39)               // 38+Tubes

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

// RING BUFFER for interrupts
#define EVENT_TIMER2      0x01
#define EVENT_INT1        0x02
#define EVENT_UART_TXC    0x04
#define EVENT_UART_RXC    0x08
#define EVENT_ADC         0x10
#define EVENT_RING_SIZE   32
#define UART_RX_RING_SIZE 8

/* Shared type: tube catalog entry */
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

/* Globals defined in config.c (declared here for TTesterLCD32.c) */
extern uint8_t d, i, busy, sync, txen, *cwart, cwartmin, cwartmax;
extern uint8_t adr, adrmin, adrmax, nowa, stop, zwloka, dziel, nodus, dusk0;
extern uint8_t zapisz, czytaj, range, rangelcd, rangedef, channel, takt;
extern uint8_t overih, overia, overig2, err, errcode, probki, pwm, anode;
extern uint8_t irx, tout, crc;
extern uint8_t bufin[10];
/* ascii[5], buf[64] in main only */

extern volatile uint16_t *wart, wartmin, wartmax, start, tuh, vref;
extern volatile uint16_t adcih, adcia, adcig2;
extern volatile uint16_t uhset, ihset, ug1set, uaset, iaset, ug2set, ig2set;
extern volatile uint16_t suhadc, sihadc, sug1adc, suaadc, siaadc, sug2adc, sig2adc;
extern volatile uint16_t muhadc, mihadc, mug1adc, muaadc, miaadc, mug2adc, mig2adc;
extern volatile uint16_t ug1zer, ug1ref, uh, ih, ua, ual, uar, ia, ial, iar;
extern volatile uint16_t ug2, ig2, ug1, ugl, ugr, s, r, k, typ, lastyp;
extern volatile uint16_t uhlcd, ihlcd, ug1lcd, ualcd, ialcd, ug2lcd, ig2lcd, slcd, rlcd, klcd;
extern volatile uint16_t srezadc, mrezadc, bufinta, bufintg2;

extern uint32_t lint, tint, licz, temp;

extern katalog lamprem, lamptem;

extern uint8_t AZ[37];

// /* Map byte to AZ index 0..36 (defined in config.c, declared here for main) */
// extern uint8_t az_index(uint8_t c);

/* PROGMEM / EEMEM data defined in config.c */
extern const katalog lamprom[];   /* lamprom[FLAMP] in config.c */
extern katalog lampeep[];        /* lampeep[ELAMP] EEMEM in config.c */
extern uint16_t poptyp;          /* EEMEM in config.c */

// /* Cyrillic CGRAM glyphs (defined in config.c) */
extern char cyrZ[8], cyrG[8], cyrB[8], cyrD[8], cyrI[8], cyrP[8], cyrC[8], cyrF[8];

#endif /* CONFIG_H */
