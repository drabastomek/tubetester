/*
 * LCD display - hardware output and refresh from buf[].
 * Uses extern buf, adr, typ, start, err, dusk0, takt from main.
 */

#include "config/config.h"
#include "display/display.h"
#include "utils/utils.h"
#include <string.h>

#ifndef VTTESTER_HOST_TEST
#if defined(ICCAVR)
#include <iom32v.h>
#else
#include <avr/io.h>
#endif

/* From main */
extern uint8_t buf[64], adr, takt, dusk0, err;
extern uint16_t typ, start;

void cmd2lcd(uint8_t rs, uint8_t bajt)
{
   delay(1);
   if (rs) { RSSET; } else { RSRST; }
   PORTC &= 0x0f;
   PORTC |= (bajt & 0xf0);
   ENSET;
   NOP2; NOP2; NOP2; NOP2;
   ENRST;
   PORTC &= 0x0f;
   PORTC |= ((bajt << 4) & 0xf0);
   ENSET;
   NOP2; NOP2; NOP2; NOP2;
   ENRST;
}

void gotoxy(uint8_t x, uint8_t y)
{
   cmd2lcd(0, 0x80 | (64 * (y % 2) + 20 * (y / 2) + x)); /* 4x20 */
}

void char2lcd(uint8_t f, uint8_t c)
{
   cmd2lcd(1, ((f == 1) && (takt == 0)) ? ' ' : c);
}

void cstr2lcd(uint8_t f, const uint8_t *c)
{
   while (*c) { char2lcd(f, *c); c++; }
}

void str2lcd(uint8_t f, uint8_t *c)
{
   while (*c) { char2lcd(f, *c); c++; }
}

#endif /* !VTTESTER_HOST_TEST */

/* LCD hw init, CGRAM (cyr*), splash. Main does delay loop + cstr2rs after. */
#ifndef VTTESTER_HOST_TEST
void display_init(void)
{
   uint8_t i;
   extern uint8_t cyrB[8], cyrC[8], cyrD[8], cyrF[8], cyrG[8], cyrI[8], cyrP[8], cyrZ[8];

   /* HD44780 needs ≥40 ms after Vcc before first command */
   delay(50);
   cmd2lcd(0, 0x28);
   cmd2lcd(0, 0x06);
   cmd2lcd(0, 0x0c);
   cmd2lcd(0, 0x01);
   cmd2lcd(0, 0x40);

   cmd2lcd(0, (0x40 | 0x00)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrB[i]); }
   cmd2lcd(0, (0x40 | 0x08)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrC[i]); }
   cmd2lcd(0, (0x40 | 0x10)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrD[i]); }
   cmd2lcd(0, (0x40 | 0x18)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrF[i]); }
   cmd2lcd(0, (0x40 | 0x20)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrG[i]); }
   cmd2lcd(0, (0x40 | 0x28)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrI[i]); }
   cmd2lcd(0, (0x40 | 0x30)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrP[i]); }
   cmd2lcd(0, (0x40 | 0x38)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrZ[i]); }

   gotoxy(0, 0); cstr2lcd(0, (const uint8_t *)"   VTTester 2.06    ");
   gotoxy(0, 1); cstr2lcd(0, (const uint8_t *)"Tomasz|Adam |       ");
   gotoxy(0, 2); cstr2lcd(0, (const uint8_t *)"Gumny |Tatus|       ");
   gotoxy(0, 3); cstr2lcd(0, (const uint8_t *)"forum-trioda.pl/    ");
}
#endif /* !VTTESTER_HOST_TEST */

/* Build row 0: number (buf[0..2]), name (buf[4..12]), " G", "-", Ug1 (buf[24..27]). */
void display_build_row0(const uint8_t *buf, uint8_t adr, uint16_t start, char *out)
{
   uint8_t i;
   (void)adr;
   (void)start;
   out[0] = buf[0];
   out[1] = buf[1];
   out[2] = buf[2];
   out[3] = ' ';
   for (i = 0; i < 9; i++) out[4 + i] = buf[4 + i];
   out[13] = ' ';
   out[14] = 'G';
   out[15] = '-';
   out[16] = buf[24];
   out[17] = buf[25];
   out[18] = buf[26];
   out[19] = buf[27];
   out[20] = '\0';
}

void display_build_row1(const uint8_t *buf, uint8_t adr, char *out)
{
   uint8_t i = 0;
   (void)adr;
   out[i++] = 'H';
   out[i++] = '=';
   out[i++] = buf[14];
   out[i++] = buf[15];
   out[i++] = buf[16];
   out[i++] = buf[17];
   out[i++] = 'V';
   out[i++] = ' ';
   out[i++] = 'A';
   out[i++] = '=';
   out[i++] = buf[29];
   out[i++] = buf[30];
   out[i++] = buf[31];
   out[i++] = ' ';
   out[i++] = 'G';
   out[i++] = '2';
   out[i++] = '=';
   out[i++] = buf[39];
   out[i++] = buf[40];
   out[i++] = buf[41];
   out[20] = '\0';
}

void display_build_row2(const uint8_t *buf, uint8_t adr, uint8_t err, char *out)
{
   uint8_t i = 0, e = ' ';
   if ((err & OVERIH) == OVERIH) e = 'H';
   if ((err & OVERIA) == OVERIA) e = 'A';
   if ((err & OVERIG) == OVERIG) e = 'G';
   if ((err & OVERTE) == OVERTE) e = 'T';
   (void)adr;
   out[i++] = e;
   out[i++] = ' ';
   out[i++] = buf[19];
   out[i++] = buf[20];
   out[i++] = buf[21];
   out[i++] = ' ';
   out[i++] = 'm';
   out[i++] = 'A';
   out[i++] = ' ';
   out[i++] = buf[33];
   out[i++] = buf[34];
   out[i++] = buf[35];
   out[i++] = buf[36];
   out[i++] = buf[37];
   out[i++] = ' ';
   out[i++] = buf[43];
   out[i++] = buf[44];
   out[i++] = buf[45];
   out[i++] = buf[46];
   out[i++] = buf[47];
   out[20] = '\0';
}

void display_build_row3(uint16_t typ, uint16_t start, uint8_t dusk0, const uint8_t *buf, uint8_t adr, char *out)
{
   uint8_t ascii[5];
   uint8_t i;
   (void)adr;
   if (typ > 1) {
      memset(out, ' ', 20);
      out[0] = 'S';
      out[1] = '=';
      out[2] = buf[49];
      out[3] = buf[50];
      out[4] = buf[51];
      out[5] = buf[52];
      out[6] = ' ';
      out[7] = 'R';
      out[8] = '=';
      out[9] = buf[54];
      out[10] = buf[55];
      out[11] = buf[56];
      out[12] = buf[57];
      if ((start <= (FUH+2)) || (dusk0 == DMAX)) {
         out[13] = ' ';
         out[14] = 'K';
         out[15] = '=';
         out[16] = buf[59];
         out[17] = buf[60];
         out[18] = buf[61];
         out[19] = buf[62];
      } else {
         int2asc(start >> 2, ascii);
         out[13] = ' ';
         out[14] = 'T';
         out[15] = '=';
         if (ascii[2] != '0') {
            out[16] = ascii[2];
            out[17] = ascii[1];
         } else {
            out[16] = ' ';
            out[17] = ascii[1] != '0' ? ascii[1] : ' ';
         }
         out[18] = ascii[0];
         out[19] = 's';
      }
   } else {
      if (typ == 0) {
         for (i = 0; i < 20; i++) out[i] = ' ';
      } else {
         memcpy(out, " TX/RX: 9600,8,N,1  ", 20);
      }
   }
   out[20] = '\0';
}

#ifndef VTTESTER_HOST_TEST
/* Draw current screen from buf[]; row 3 depends on typ (local vs power supply/remote). */
void display_refresh(void)
{
   uint8_t i, errcode;
   uint8_t ascii[5];

   gotoxy(0, 0);
   str2lcd((adr == 0) && (start != (FUH+2)), &buf[0]);

   gotoxy(3, 0);
   char2lcd(0, ' ');
   for (i = 0; i < 9; i++)
      char2lcd(((adr-1) == i) || ((i == 7) && (start == (FUH+2)) && ((buf[11] == '1') || (buf[11] == '2'))), buf[i+4]);
   gotoxy(13, 0);
   cstr2lcd(0, (const uint8_t *)" G");
   char2lcd(adr == 10, '-');
   str2lcd(adr == 10, &buf[24]);

   gotoxy(0, 1);
   cstr2lcd(0, (const uint8_t *)"H=");
   str2lcd(adr == 11, &buf[14]);
   cstr2lcd(0, (const uint8_t *)"V A=");
   str2lcd(adr == 13, &buf[29]);
   cstr2lcd(0, (const uint8_t *)" G2=");
   str2lcd(adr == 15, &buf[39]);

   gotoxy(0, 2);
   errcode = ' ';
   if ((err & OVERIH) == OVERIH) errcode = 'H';
   if ((err & OVERIA) == OVERIA) errcode = 'A';
   if ((err & OVERIG) == OVERIG) errcode = 'G';
   if ((err & OVERTE) == OVERTE) errcode = 'T';
   char2lcd(0, errcode);
   char2lcd(0, ' ');
   str2lcd(adr == 12, &buf[19]);
   cstr2lcd(0, (const uint8_t *)"mA ");
   str2lcd(adr == 14, &buf[33]);
   char2lcd(0, ' ');
   str2lcd(adr == 16, &buf[43]);

   gotoxy(0, 3);
   if (typ > 1) {
      gotoxy(0, 3);
      cstr2lcd(0, (const uint8_t *)"S=");
      str2lcd(adr == 17, &buf[49]);
      gotoxy(6, 3);
      cstr2lcd(0, (const uint8_t *)" R=");
      str2lcd(adr == 18, &buf[54]);
      gotoxy(13, 3);
      if ((start <= (FUH+2)) || (dusk0 == DMAX)) {
         cstr2lcd(0, (const uint8_t *)" K=");
         str2lcd(adr == 19, &buf[59]);
      } else {
         cstr2lcd(0, (const uint8_t *)" T=");
         int2asc(start >> 2, ascii);
         if (ascii[2] != '0') {
            char2lcd(0, ascii[2]);
            char2lcd(0, ascii[1]);
         } else {
            char2lcd(0, ' ');
            if (ascii[1] != '0') {
               char2lcd(0, ascii[1]);
            } else {
               char2lcd(0, ' ');
            }
         }
         char2lcd(0, ascii[0]);
         char2lcd(0, 's');
      }
   } else {
      if (typ == 0) {
         cstr2lcd(0, (const uint8_t *)"                    ");
      } else {
         cstr2lcd(0, (const uint8_t *)" TX/RX: 9600,8,N,1  ");
      }
   }
}
#endif /* !VTTESTER_HOST_TEST */
