/*
 * Harness for control unit tests: defines all externs and stubs (load_lamprom, AZ).
 * Use with test config (katalog, FLAMP, ELAMP, EEPROM stubs).
 */
#include "config/config.h"
#include <string.h>

/* Control externs */
unsigned char buf[64], adr, typ, nowa, nodus, dusk0, zapisz, czytaj;
unsigned char range, rangelcd, rangedef, txen, bufin[10];
unsigned int start, tuh, vref;
unsigned int uhset, ihset, ug1set, uaset, ug2set;
unsigned int muhadc, mihadc, mug1adc, muaadc, miaadc, mug2adc, mig2adc;
unsigned int ug1zer, ug1ref, uh, ih, ua, ia, ug2, ig2, ug1;
unsigned int uhlcd, ihlcd, ug1lcd, ualcd, ialcd, ug2lcd, ig2lcd, slcd, rlcd, klcd;
unsigned int s, r, k, lastyp;
unsigned long licz, temp;
unsigned char anode;
katalog lamprem, lamptem;
unsigned int poptyp;
katalog lampeep[ELAMP];

/* AZ: index 0..25 A-Z, 26=_, 27..35 '0'..'9' (nazwa bytes are indices into AZ) */
const unsigned char AZ[37] = {
   'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
   '_', '0','1','2','3','4','5','6','7','8','9'
};

/* Minimal ROM catalog: index 0=PWR SUP, 1=REMOTE, 2=6N1P (nazwa as ASCII; nazwa[8] used for tuh) */
static const katalog lamprom[FLAMP] = {
   { {'P','W','R',' ','S','U','P',' ','0'}, 0, 0, 240, 0, 0, 0, 0, 0, 0, 0 },
   { {'R','E','M','O','T','E',' ',' ','1'}, 0, 0, 240, 0, 0, 0, 0, 0, 0, 0 },
   { {'6','N','1','P',' ',' ','G','1','1'}, 63, 0, 40, 250, 75, 0, 0, 45, 8, 35 },
};

void load_lamprom(unsigned int idx, katalog *dest)
{
   if (idx < FLAMP)
      memcpy(dest, &lamprom[idx], sizeof(katalog));
}
