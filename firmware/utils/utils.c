/*
 * Helper functions - use globals from main app (extern).
 */

#ifdef VTTESTER_HOST_TEST
/* Host unit test build: no AVR, stubs for hardware-dependent functions. */
# include <stddef.h>
# include "utils.h"
void char2rs(uint8_t bajt) { (void)bajt; }
void cstr2rs(const char *q) { (void)q; }
void delay(uint8_t opoz) { (void)opoz; }
void zersrk(void) {}
uint16_t liczug1(uint16_t pug1) { (void)pug1; return 0; }
#else
# if defined(ICCAVR)
#  include <iom32v.h>
# else
#  include <avr/io.h>
# endif
# include "config/config.h"
# include "utils.h"

/* Globals used by helpers (defined in TTesterLCD32.c) */
extern uint8_t busy, zwloka;
extern uint16_t s, r, k, ualcd, ialcd, ug2lcd, ig2lcd, slcd, rlcd, klcd, vref;
extern uint32_t licz, temp;

void char2rs(uint8_t bajt)
{
   UDR = bajt;
   busy = 1;
   while( busy );          /* czekaj na koniec wysylania bajtu */
}

void cstr2rs(const char *q)
{
   while( *q )
   {
      UDR = *q;
      q++;
      busy = 1;
      while( busy );
   }
}

void delay(uint8_t opoz)
{
   zwloka = opoz + 1;
   while( zwloka != 0 ) { WDR; }
}

void zersrk(void)
{
   s = r = k = ualcd = ialcd = ug2lcd = ig2lcd = slcd = rlcd = klcd = 0;
}

uint16_t liczug1(uint16_t pug1)
{
   licz = 640000;
   licz *= vref;
   temp = 1024000;
   temp *= pug1;
   licz -= temp;
   licz /= 725;
   licz /= vref;
   return (uint16_t)licz;
}
#endif /* !VTTESTER_HOST_TEST */

void int2asc(uint16_t liczba, uint8_t *ascii)
{
   uint8_t i, t;
   for (i = 0; i < 4; i++) {
      t = (uint8_t)(liczba % 10);
      liczba /= 10;
      ascii[i] = '0' + t;
   }
}
