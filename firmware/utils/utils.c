/*
 * Helper functions - use globals from main app (extern).
 */

#ifdef VTTESTER_HOST_TEST
/* Host unit test build: no AVR, stubs for hardware-dependent functions. */
# include <stddef.h>
# include "utils.h"
void char2rs(unsigned char bajt) { (void)bajt; }
void cstr2rs(const char *q) { (void)q; }
void delay(unsigned char opoz) { (void)opoz; }
void zersrk(void) {}
unsigned int liczug1(unsigned int pug1) { (void)pug1; return 0; }
#else
# if defined(ICCAVR)
#  include <iom32v.h>
# else
#  include <avr/io.h>
# endif
# include "config/config.h"
# include "utils.h"

/* Globals used by helpers (defined in TTesterLCD32.c) */
extern unsigned char busy, zwloka;
extern unsigned int s, r, k, ualcd, ialcd, ug2lcd, ig2lcd, slcd, rlcd, klcd, vref;
extern unsigned long licz, temp;

void char2rs(unsigned char bajt)
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

void delay(unsigned char opoz)
{
   zwloka = opoz + 1;
   while( zwloka != 0 ) { WDR; }
}

void zersrk(void)
{
   s = r = k = ualcd = ialcd = ug2lcd = ig2lcd = slcd = rlcd = klcd = 0;
}

unsigned int liczug1(unsigned int pug1)
{
   licz = 640000;
   licz *= vref;
   temp = 1024000;
   temp *= pug1;
   licz -= temp;
   licz /= 725;
   licz /= vref;
   return (unsigned int)licz;
}
#endif /* !VTTESTER_HOST_TEST */

void int2asc(unsigned int liczba, unsigned char *ascii)
{
   unsigned char i, t;
   for (i = 0; i < 4; i++) {
      t = (unsigned char)(liczba % 10);
      liczba /= 10;
      ascii[i] = '0' + t;
   }
}
