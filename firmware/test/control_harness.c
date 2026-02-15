/*
 * Harness for control unit tests: defines all externs and stubs (load_lamprom, AZ).
 * Use with test config (katalog, FLAMP, ELAMP, EEPROM stubs).
 */
#include "config/config.h"
#include <string.h>

/* Control externs */
uint8_t buf[64], adr, nowa, nodus, dusk0, zapisz, czytaj;
uint16_t typ;
uint8_t range, rangelcd, rangedef, txen, bufin[10];
uint16_t start, tuh, vref;
uint16_t uhset, ihset, ug1set, uaset, ug2set;
uint16_t muhadc, mihadc, mug1adc, muaadc, miaadc, mug2adc, mig2adc;
uint16_t ug1zer, ug1ref, uh, ih, ua, ia, ug2, ig2, ug1;
uint16_t uhlcd, ihlcd, ug1lcd, ualcd, ialcd, ug2lcd, ig2lcd, slcd, rlcd, klcd;
uint16_t s, r, k, lastyp;
uint32_t licz, temp;
uint8_t anode;
katalog lamprem, lamptem;
uint16_t poptyp;
katalog lampeep[ELAMP];

/* AZ: index 0..25 A-Z, 26=_, 27..35 '0'..'9' (nazwa bytes are indices into AZ) */
const uint8_t AZ[37] = {
   'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
   '_', '0','1','2','3','4','5','6','7','8','9'
};

/* Minimal ROM catalog: index 0=PWR SUP, 1=REMOTE, 2=6N1P (nazwa as ASCII; nazwa[8] used for tuh) */
static const katalog lamprom[FLAMP] = {
   { {'P','W','R',' ','S','U','P',' ','0'}, 0, 0, 240, 0, 0, 0, 0, 0, 0, 0 },
   { {'R','E','M','O','T','E',' ',' ','1'}, 0, 0, 240, 0, 0, 0, 0, 0, 0, 0 },
   { {'6','N','1','P',' ',' ','G','1','1'}, 63, 0, 40, 250, 75, 0, 0, 45, 8, 35 },
};

void load_lamprom(uint16_t idx, katalog *dest)
{
   if (idx < FLAMP)
      memcpy(dest, &lamprom[idx], sizeof(katalog));
}
