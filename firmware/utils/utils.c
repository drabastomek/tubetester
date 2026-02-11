/*
 * Helper functions - use globals from main app (extern).
 */

#include <avr/io.h>
#include "utils.h"

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
   while( zwloka != 0 ) ;
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
