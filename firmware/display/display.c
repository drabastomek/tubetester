/*
 * LCD display - hardware output and refresh from buf[].
 * Uses extern buf, adr, typ, start, err, dusk0, takt from main.
 */

#include "config/config.h"
#include "display/display.h"
#include "utils/utils.h"
#include <avr/io.h>

/* From main */
extern unsigned char buf[64], adr, typ, takt, dusk0, err;
extern unsigned int start;

void cmd2lcd(char rs, char bajt)
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

void gotoxy(char x, char y)
{
   cmd2lcd(0, 0x80 | (64 * (y % 2) + 20 * (y / 2) + x)); /* 4x20 */
}

void char2lcd(char f, char c)
{
   cmd2lcd(1, ((f == 1) && (takt == 0)) ? ' ' : c);
}

void cstr2lcd(char f, const unsigned char *c)
{
   while (*c) { char2lcd(f, *c); c++; }
}

void str2lcd(char f, unsigned char *c)
{
   while (*c) { char2lcd(f, *c); c++; }
}

/* LCD hw init, CGRAM (cyr*), splash. Main does delay loop + cstr2rs after. */
void display_init(void)
{
   unsigned char i;
   extern char cyrB[8], cyrC[8], cyrD[8], cyrF[8], cyrG[8], cyrI[8], cyrP[8], cyrZ[8];

   delay(30);
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

   gotoxy(0, 0); cstr2lcd(0, (const unsigned char *)"   VTTester 2.06    ");
   gotoxy(0, 1); cstr2lcd(0, (const unsigned char *)"Tomasz|Adam |       ");
   gotoxy(0, 2); cstr2lcd(0, (const unsigned char *)"Gumny |Tatus|       ");
   gotoxy(0, 3); cstr2lcd(0, (const unsigned char *)"forum-trioda.pl/    ");
}

/* Draw current screen from buf[]; row 3 depends on typ (local vs power supply/remote). */
void display_refresh(void)
{
   unsigned char i, errcode;
   unsigned char ascii[5];

   gotoxy(0, 0);
   str2lcd((adr == 0) && (start != (FUH+2)), &buf[0]);

   gotoxy(3, 0);
   char2lcd(0, ' ');
   for (i = 0; i < 9; i++)
      char2lcd(((adr-1) == i) || ((i == 7) && (start == (FUH+2)) && ((buf[11] == '1') || (buf[11] == '2'))), buf[i+4]);
   gotoxy(13, 0);
   cstr2lcd(0, (const unsigned char *)" G");
   char2lcd(adr == 10, '-');
   str2lcd(adr == 10, &buf[24]);

   gotoxy(0, 1);
   cstr2lcd(0, (const unsigned char *)"H=");
   str2lcd(adr == 11, &buf[14]);
   cstr2lcd(0, (const unsigned char *)"V A=");
   str2lcd(adr == 13, &buf[29]);
   cstr2lcd(0, (const unsigned char *)" G2=");
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
   cstr2lcd(0, (const unsigned char *)"mA ");
   str2lcd(adr == 14, &buf[33]);
   char2lcd(0, ' ');
   str2lcd(adr == 16, &buf[43]);

   gotoxy(0, 3);
   if (typ > 1) {
      gotoxy(0, 3);
      cstr2lcd(0, (const unsigned char *)"S=");
      str2lcd(adr == 17, &buf[49]);
      gotoxy(6, 3);
      cstr2lcd(0, (const unsigned char *)" R=");
      str2lcd(adr == 18, &buf[54]);
      gotoxy(13, 3);
      if ((start <= (FUH+2)) || (dusk0 == DMAX)) {
         cstr2lcd(0, (const unsigned char *)" K=");
         str2lcd(adr == 19, &buf[59]);
      } else {
         cstr2lcd(0, (const unsigned char *)" T=");
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
         cstr2lcd(0, (const unsigned char *)"                    ");
      } else {
         cstr2lcd(0, (const unsigned char *)" TX/RX: 9600,8,N,1  ");
      }
   }
}
