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
# include <avr/io.h>
# include "config/config.h"
# include "utils.h"

/* Globals used by helpers (defined in TTesterLCD32.c) */
extern uint8_t uart_tx_busy, delay_ticks_remaining;
extern uint16_t slope_s, resistance_r, amplification_k, lcd_ua, lcd_ia, lcd_ug2, lcd_ig2, lcd_s, lcd_r, lcd_k, adc_vref_scaled;
extern uint32_t ug1_calc_accum, ug1_calc_temp;

void char2rs(uint8_t bajt)
{
   UDR = bajt;
   uart_tx_busy = 1;
   while( uart_tx_busy );          /* czekaj na koniec wysylania bajtu */
}

void cstr2rs(const char *q)
{
   while( *q )
   {
      UDR = *q;
      q++;
      uart_tx_busy = 1;
      while( uart_tx_busy );
   }
}

void delay(uint8_t opoz)
{
   delay_ticks_remaining = opoz + 1;
   while( delay_ticks_remaining != 0 ) { WDR; }
}

void zersrk(void)
{
   slope_s = resistance_r = amplification_k = lcd_ua = lcd_ia = lcd_ug2 = lcd_ig2 = lcd_s = lcd_r = lcd_k = 0;
}

uint16_t liczug1(uint16_t pug1)
{
   ug1_calc_accum = 640000;
   ug1_calc_accum *= adc_vref_scaled;
   ug1_calc_temp = 1024000;
   ug1_calc_temp *= pug1;
   ug1_calc_accum -= ug1_calc_temp;
   ug1_calc_accum /= 725;
   ug1_calc_accum /= adc_vref_scaled;
   return (uint16_t)ug1_calc_accum;
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
