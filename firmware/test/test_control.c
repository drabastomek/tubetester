/*
 * Unit tests for control_update: menu, katalog, buf[] fill from ADC/state.
 * Build with control_harness (globals + load_lamprom) and test config.
 */
#include "test_common.h"
#include "../control/control.h"
#include "config/config.h"
#include <string.h>

extern unsigned char buf[64], adr, typ, dusk0, zapisz, czytaj;
extern unsigned int start, vref, ug1lcd, uhlcd, ihlcd, lastyp;
extern unsigned int muhadc, mihadc, mug1adc, muaadc, miaadc, mug2adc, mig2adc;
extern unsigned int range;
extern katalog lamprem, lamptem;

/* AZ indices: A=0..Z=25, _=26, 0=27..9=35. nazwa[] uses these; buf[11] = AZ[nazwa[7]]. */
static void test_control_adr0_typ1_fills_buf_number_and_name(void)
{
   unsigned char ascii[5];
   memset(buf, 0, sizeof(buf));
   adr = 0;
   typ = 1;
   start = 0;
   dusk0 = 0;
   /* nazwa as AZ indices: 6=33, N=13, 1=28, P=15, space=26, G=6, 1=28 */
   lamprem.nazwa[0] = 33; lamprem.nazwa[1] = 13; lamprem.nazwa[2] = 28; lamprem.nazwa[3] = 15;
   lamprem.nazwa[4] = 26; lamprem.nazwa[5] = 26; lamprem.nazwa[6] = 26;
   lamprem.nazwa[7] = 6;  /* G */
   lamprem.nazwa[8] = 28;  /* 1 */

   control_update(ascii);

   TEST_ASSERT(buf[0] == '0' && buf[1] == '0' && buf[2] == '1');
   TEST_ASSERT(buf[4] == 33 && buf[5] == 13 && buf[6] == 28 && buf[7] == 15);
   TEST_ASSERT(buf[11] == 'G');  /* overwritten by AZ[nazwa[7]] */
   TEST_ASSERT(buf[12] == 28);
}

static void test_control_ug1_display_from_adc(void)
{
   unsigned char ascii[5];
   memset(buf, 0, sizeof(buf));
   adr = 0;
   typ = 1;
   start = 0;
   dusk0 = 0;
   vref = 1000;
   mug1adc = 0;
   memcpy(lamprem.nazwa, "6N1P  G11", 9);

   control_update(ascii);

   /* ug1 path: licz = (40960000 - mug1adc*725)>>16 * vref / 1000; mug1adc=0 => 625 */
   TEST_ASSERT(buf[24] == '6' && buf[25] == '2' && buf[26] == '.' && buf[27] == '5');
}

static void test_control_typ2_loads_lamprom_name(void)
{
   unsigned char ascii[5];
   memset(buf, 0, sizeof(buf));
   adr = 0;
   typ = 2;
   start = 0;
   dusk0 = 0;
   memcpy(lamprem.nazwa, "REMOTE   ", 9);

   control_update(ascii);

   /* typ==2: load_lamprom(2, &lamptem) => "6N1P  G11" */
   TEST_ASSERT(memcmp(&buf[4], "6N1P  G11", 9) == 0);
   TEST_ASSERT(buf[0] == '0' && buf[1] == '0' && buf[2] == '2');
}

void run_control_tests(void)
{
   test_control_adr0_typ1_fills_buf_number_and_name();
   test_control_ug1_display_from_adc();
   test_control_typ2_loads_lamprom_name();
}
