/*
 * Control logic: menu, katalog, parameter editing, calculations.
 * Uses extern state from main; fills buf[] and updates setpoints/EEPROM.
 */

#include "config/config.h"
#include "control/control.h"
#include "utils/utils.h"
#ifndef VTTESTER_HOST_TEST
#if defined(ICCAVR)
#include <iom32v.h>
#else
#include <avr/io.h>
#endif
#endif

extern uint8_t buf[64], adr, nowa, nodus, dusk0, zapisz, czytaj;
extern uint8_t range, rangelcd, rangedef, txen, bufin[10];
extern uint16_t typ, start, tuh, vref;
extern uint16_t uhset, ihset, ug1set, uaset, ug2set;
extern uint16_t muhadc, mihadc, mug1adc, muaadc, miaadc, mug2adc, mig2adc;
extern uint16_t ug1zer, ug1ref, uh, ih, ua, ia, ug2, ig2, ug1;
extern uint16_t uhlcd, ihlcd, ug1lcd, ualcd, ialcd, ug2lcd, ig2lcd, slcd, rlcd, klcd;
extern uint16_t s, r, k, lastyp;
extern uint32_t licz, temp;
extern uint8_t anode;
extern katalog lamprem, lamptem;
extern uint16_t poptyp;
extern const uint8_t AZ[37];
extern void load_lamprom(uint16_t idx, katalog *dest);
extern katalog lampeep[ELAMP];
extern void char2rs(uint8_t bajt);
extern void cstr2rs(const char *q);

void control_update(uint8_t *ascii)
{
   uint8_t i;

   /* Wyswietlanie Numeru / Pobieranie nowej Nazwy / Edycja Nazwy */
   if (adr == 0) {
      int2asc(typ, ascii);
      buf[0] = ascii[2];
      buf[1] = ascii[1];
      buf[2] = ascii[0];

      if (typ < FLAMP) {
         if (typ == 1) {
            lamptem = lamprem;
         } else {
            load_lamprom(typ, &lamptem);
            if ((typ != lastyp) && (start != (FUH+2))) {
               load_lamprom(1, &lamprem);
               anode = 28;
               ug1set = ug1zer = ug1ref = liczug1(240);
               uhset = ihset = uaset = ug2set = 0;
            }
         }
         tuh = (lamptem.nazwa[8] - '0') * 240;
         for (i = 0; i < 9; i++) buf[i+4] = (uint8_t)lamptem.nazwa[i];
      } else {
         EEPROM_READ(&lampeep[typ-FLAMP], lamptem);
         tuh = (lamptem.nazwa[8] - 27) * 240;
         for (i = 0; i < 9; i++) buf[i+4] = AZ[(uint8_t)lamptem.nazwa[i]];
      }
      lastyp = typ;
      nowa = 1;
   }

   if ((typ == 0) || (typ == 1)) {
      anode = lamptem.nazwa[7];
      buf[11] = AZ[anode];
   }

   if (typ >= FLAMP) {
      if ((adr > 0) && (adr < 10)) {
         if (czytaj == 1) EEPROM_READ(&lampeep[typ-FLAMP].nazwa, lamptem.nazwa);
         for (i = 0; i < 9; i++) buf[i+4] = AZ[(uint8_t)lamptem.nazwa[i]];
         czytaj = 0;
         if (dusk0 == DMAX) zapisz = 1;
         if ((nodus == DMIN) && (zapisz == 1)) {
            EEPROM_WRITE(&lampeep[typ-FLAMP].nazwa[adr-1], lamptem.nazwa[adr-1]);
            zapisz = 0;
         }
      } else {
         czytaj = 1;
      }
   }

   /* Ug1 */
   ug1ref = liczug1(lamptem.ug1def);
   temp = mug1adc;
   temp *= 725;
   licz = 40960000;
   if (licz > temp) licz -= temp; else licz = 0;
   licz >>= 16;
   licz *= vref;
   licz /= 1000;
   ug1 = (uint16_t)licz;
   if (start == (FUH+2)) licz = ug1lcd;
   if ((adr == 0) || (adr == 10)) {
      if (dusk0 == DMAX) { licz = lamptem.ug1def; zapisz = 1; }
      if ((nodus == DMIN) && (adr == 10) && (zapisz == 1)) {
         zapisz = 0;
         if (typ == 0) ug1set = ug1ref;
         if (typ >= FLAMP) EEPROM_WRITE(&lampeep[typ-FLAMP].ug1def, lamptem.ug1def);
      }
   }
   int2asc((uint16_t)licz, ascii);
   buf[24] = (ascii[2] != '0') ? ascii[2] : ' ';
   buf[25] = ascii[1];
   buf[26] = '.';
   buf[27] = ascii[0];

   /* Uh */
   licz = muhadc;
   licz *= vref;
   licz >>= 14;
   temp = mihadc;
   temp *= vref;
   temp >>= 16;
   if (licz > temp) licz -= temp; else licz = 0;
   licz /= 10;
   uh = (uint16_t)licz;
   if (start == (FUH+2)) licz = uhlcd;
   if ((adr == 0) || (adr == 11)) {
      if (dusk0 == DMAX) { licz = lamptem.uhdef; zapisz = 1; }
      if ((nodus == DMIN) && (adr == 11) && (zapisz == 1)) {
         zapisz = 0;
         if (typ == 0) { uhset = lamptem.uhdef; ihset = lamptem.ihdef = 0; }
         if (typ >= FLAMP) EEPROM_WRITE(&lampeep[typ-FLAMP].uhdef, lamptem.uhdef);
      }
   }
   int2asc((uint16_t)licz, ascii);
   buf[14] = (ascii[2] != '0') ? ascii[2] : ' ';
   buf[15] = ascii[1];
   buf[16] = '.';
   buf[17] = ascii[0];

   /* Ih */
   licz = mihadc;
   licz *= vref;
   licz >>= 15;
   ih = (uint16_t)licz;
   if (start == (FUH+2)) licz = ihlcd;
   if ((adr == 0) || (adr == 12)) {
      if (dusk0 == DMAX) { licz = lamptem.ihdef; zapisz = 1; }
      if ((nodus == DMIN) && (adr == 12) && (zapisz == 1)) {
         zapisz = 0;
         if (typ == 0) { ihset = lamptem.ihdef; uhset = lamptem.uhdef = 0; }
         if (typ >= FLAMP) EEPROM_WRITE(&lampeep[typ-FLAMP].ihdef, lamptem.ihdef);
      }
   }
   int2asc((uint16_t)licz, ascii);
   if (ascii[2] != '0') {
      buf[19] = ascii[2]; buf[20] = ascii[1]; buf[21] = ascii[0];
   } else {
      buf[19] = ' ';
      if (ascii[1] != '0') { buf[20] = ascii[1]; buf[21] = ascii[0]; }
      else { buf[20] = ' '; buf[21] = (ascii[0] != '0') ? ascii[0] : ' '; }
   }
   buf[22] = '0';

   /* Ua */
   licz = muaadc;
   licz *= vref;
   licz /= 107436;
   ua = (uint16_t)licz;
   if (start == (FUH+2)) licz = ualcd;
   if ((adr == 0) || (adr == 13)) {
      if (dusk0 == DMAX) { licz = lamptem.uadef; zapisz = 1; }
      if ((nodus == DMIN) && (adr == 13) && (zapisz == 1)) {
         zapisz = 0;
         if (typ == 0) uaset = lamptem.uadef;
         if (typ >= FLAMP) EEPROM_WRITE(&lampeep[typ-FLAMP].uadef, lamptem.uadef);
      }
   }
   int2asc((uint16_t)licz, ascii);
   if (ascii[2] != '0') { buf[29] = ascii[2]; buf[30] = ascii[1]; }
   else { buf[29] = ' '; buf[30] = (ascii[1] != '0') ? ascii[1] : ' '; }
   buf[31] = ascii[0];

   /* Ia */
   licz = miaadc;
   licz *= vref;
   licz >>= 14;
   temp = muaadc;
   temp *= vref;
   if (range == 0) temp *= 10;
   temp /= 4369064;
   if (licz > temp) licz -= temp; else licz = 0;
   ia = (uint16_t)licz;
   if (start == (FUH+2)) licz = ialcd;
   rangedef = 0;
   if ((adr == 0) || (adr == 14)) {
      if (dusk0 == DMAX) { licz = lamptem.iadef; rangedef = 1; zapisz = 1; }
      if ((nodus == DMIN) && (adr == 14) && (zapisz == 1)) {
         zapisz = 0;
         if (typ >= FLAMP) EEPROM_WRITE(&lampeep[typ-FLAMP].iadef, lamptem.iadef);
      }
   }
   int2asc((uint16_t)licz, ascii);
   if ((rangedef == 0) && (((start != (FUH+2)) && (range == 0)) || ((start == (FUH+2)) && (rangelcd == 0)))) {
      buf[33] = (ascii[3] != '0') ? ascii[3] : ' ';
      buf[34] = ascii[2]; buf[35] = '.'; buf[36] = ascii[1]; buf[37] = ascii[0];
   } else {
      if (ascii[3] != '0') { buf[33] = ascii[3]; buf[34] = ascii[2]; }
      else { buf[33] = ' '; buf[34] = (ascii[2] != '0') ? ascii[2] : ' '; }
      buf[35] = ascii[1]; buf[36] = '.'; buf[37] = ascii[0];
   }

   /* Ug2 */
   licz = mug2adc;
   licz *= vref;
   licz /= 107436;
   ug2 = (uint16_t)licz;
   if (start == (FUH+2)) licz = ug2lcd;
   if ((adr == 0) || (adr == 15)) {
      if (dusk0 == DMAX) { licz = lamptem.ug2def; zapisz = 1; }
      if ((nodus == DMIN) && (adr == 15) && (zapisz == 1)) {
         zapisz = 0;
         if (typ == 0) ug2set = lamptem.ug2def;
         if (typ >= FLAMP) EEPROM_WRITE(&lampeep[typ-FLAMP].ug2def, lamptem.ug2def);
      }
   }
   int2asc((uint16_t)licz, ascii);
   if (ascii[2] != '0') { buf[39] = ascii[2]; buf[40] = ascii[1]; }
   else { buf[39] = ' '; buf[40] = (ascii[1] != '0') ? ascii[1] : ' '; }
   buf[41] = ascii[0];

   /* Ig2 */
   licz = mig2adc;
   licz *= vref;
   licz >>= 13;
   temp = mug2adc;
   temp *= vref;
   temp *= 10;
   temp /= 4369064;
   if (licz > temp) licz -= temp; else licz = 0;
   ig2 = (uint16_t)licz;
   if (start == (FUH+2)) licz = ig2lcd;
   if ((adr == 0) || (adr == 16)) {
      if (dusk0 == DMAX) { licz = lamptem.ig2def; zapisz = 1; }
      if ((nodus == DMIN) && (adr == 16) && (zapisz == 1)) {
         zapisz = 0;
         if (typ >= FLAMP) EEPROM_WRITE(&lampeep[typ-FLAMP].ig2def, lamptem.ig2def);
      }
   }
   int2asc((uint16_t)licz, ascii);
   buf[43] = (ascii[3] != '0') ? ascii[3] : ' ';
   buf[44] = ascii[2]; buf[45] = '.'; buf[46] = ascii[1]; buf[47] = ascii[0];

   /* S */
   licz = s;
   if (start == (FUH+2)) licz = slcd;
   if ((adr == 0) || (adr == 17)) {
      if (dusk0 == DMAX) { licz = lamptem.sdef; zapisz = 1; }
      if ((nodus == DMIN) && (adr == 17) && (zapisz == 1)) {
         zapisz = 0;
         if (typ >= FLAMP) EEPROM_WRITE(&lampeep[typ-FLAMP].sdef, lamptem.sdef);
      }
   }
   int2asc(licz, ascii);
   buf[49] = (ascii[2] != '0') ? ascii[2] : ' ';
   buf[50] = ascii[1]; buf[51] = '.'; buf[52] = ascii[0];

   /* R */
   licz = r;
   if (start == (FUH+2)) licz = rlcd;
   if ((adr == 0) || (adr == 18)) {
      if (dusk0 == DMAX) { licz = lamptem.rdef; zapisz = 1; }
      if ((nodus == DMIN) && (adr == 18) && (zapisz == 1)) {
         zapisz = 0;
         if (typ >= FLAMP) EEPROM_WRITE(&lampeep[typ-FLAMP].rdef, lamptem.rdef);
      }
   }
   int2asc(licz, ascii);
   buf[54] = (ascii[2] != '0') ? ascii[2] : ' ';
   buf[55] = ascii[1]; buf[56] = '.'; buf[57] = ascii[0];

   /* K */
   licz = k;
   if (start == (FUH+2)) licz = klcd;
   if ((adr == 0) || (adr == 19)) {
      if (dusk0 == DMAX) { licz = lamptem.kdef; zapisz = 1; }
      if ((nodus == DMIN) && (adr == 19) && (zapisz == 1)) {
         zapisz = 0;
         if (typ >= FLAMP) EEPROM_WRITE(&lampeep[typ-FLAMP].kdef, lamptem.kdef);
      }
   }
   int2asc(licz, ascii);
   if (ascii[3] != '0') { buf[59] = ascii[3]; buf[60] = ascii[2]; buf[61] = ascii[1]; }
   else {
      buf[59] = ' ';
      if (ascii[2] != '0') { buf[60] = ascii[2]; buf[61] = ascii[1]; }
      else { buf[60] = ' '; buf[61] = (ascii[1] != '0') ? ascii[1] : ' '; }
   }
   buf[62] = ascii[0];

   /* Wyslanie pomiarow do PC */
   if (txen) {
      EEPROM_WRITE(&poptyp, typ);
      if (typ == 1) {
         for (i = 0; i < 10; i++) char2rs(bufin[i]);
      } else {
         cstr2rs("\r\n");
         for (i = 0; i < 63; i++) {
            if (buf[i] != '\0') char2rs(buf[i]); else cstr2rs("  ");
         }
      }
      txen = 0;
   }
}
