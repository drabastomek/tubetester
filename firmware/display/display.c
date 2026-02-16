/*
 * LCD display - hardware output and refresh from lcd_line_buffer[].
 * Uses extern lcd_line_buffer, edit_field_index, tube_type_index, sequencer_phase_ticks, alarm_error_bits, debounce_set_button_hold_ticks, display_blink_phase from main.
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
extern uint8_t lcd_line_buffer[64], edit_field_index, display_blink_phase, debounce_set_button_hold_ticks, alarm_error_bits;
extern uint16_t tube_type_index, sequencer_phase_ticks;

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
   cmd2lcd(1, ((f == 1) && (display_blink_phase == 0)) ? ' ' : c);
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
   extern uint8_t cyrillic_cgram_B[8], cyrillic_cgram_C[8], cyrillic_cgram_D[8], cyrillic_cgram_F[8], cyrillic_cgram_G[8], cyrillic_cgram_I[8], cyrillic_cgram_P[8], cyrillic_cgram_Z[8];

   /* HD44780 needs ≥40 ms after Vcc before first command */
   delay(50);
   cmd2lcd(0, 0x28);
   cmd2lcd(0, 0x06);
   cmd2lcd(0, 0x0c);
   cmd2lcd(0, 0x01);
   cmd2lcd(0, 0x40);

   cmd2lcd(0, (0x40 | 0x00)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrillic_cgram_B[i]); }
   cmd2lcd(0, (0x40 | 0x08)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrillic_cgram_C[i]); }
   cmd2lcd(0, (0x40 | 0x10)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrillic_cgram_D[i]); }
   cmd2lcd(0, (0x40 | 0x18)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrillic_cgram_F[i]); }
   cmd2lcd(0, (0x40 | 0x20)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrillic_cgram_G[i]); }
   cmd2lcd(0, (0x40 | 0x28)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrillic_cgram_I[i]); }
   cmd2lcd(0, (0x40 | 0x30)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrillic_cgram_P[i]); }
   cmd2lcd(0, (0x40 | 0x38)); for (i = 0; i < 8; i++) { cmd2lcd(1, cyrillic_cgram_Z[i]); }

   gotoxy(0, 0); cstr2lcd(0, (const uint8_t *)"   VTTester 2.06    ");
   gotoxy(0, 1); cstr2lcd(0, (const uint8_t *)"Tomasz|Adam |       ");
   gotoxy(0, 2); cstr2lcd(0, (const uint8_t *)"Gumny |Tatus|       ");
   gotoxy(0, 3); cstr2lcd(0, (const uint8_t *)"forum-trioda.pl/    ");
}
#endif /* !VTTESTER_HOST_TEST */

/* Build row 0: number (lcd_line_buffer[0..2]), name (lcd_line_buffer[4..12]), " G", "-", Ug1 (lcd_line_buffer[24..27]). */
void display_build_row0(const uint8_t *lcd_line_buffer, uint8_t edit_field_index, uint16_t sequencer_phase_ticks, char *out)
{
   uint8_t i;
   (void)edit_field_index;
   (void)sequencer_phase_ticks;
   out[0] = lcd_line_buffer[0];
   out[1] = lcd_line_buffer[1];
   out[2] = lcd_line_buffer[2];
   out[3] = ' ';
   for (i = 0; i < 9; i++) out[4 + i] = lcd_line_buffer[4 + i];
   out[13] = ' ';
   out[14] = 'G';
   out[15] = '-';
   out[16] = lcd_line_buffer[24];
   out[17] = lcd_line_buffer[25];
   out[18] = lcd_line_buffer[26];
   out[19] = lcd_line_buffer[27];
   out[20] = '\0';
}

void display_build_row1(const uint8_t *lcd_line_buffer, uint8_t edit_field_index, char *out)
{
   uint8_t i = 0;
   (void)edit_field_index;
   out[i++] = 'H';
   out[i++] = '=';
   out[i++] = lcd_line_buffer[14];
   out[i++] = lcd_line_buffer[15];
   out[i++] = lcd_line_buffer[16];
   out[i++] = lcd_line_buffer[17];
   out[i++] = 'V';
   out[i++] = ' ';
   out[i++] = 'A';
   out[i++] = '=';
   out[i++] = lcd_line_buffer[29];
   out[i++] = lcd_line_buffer[30];
   out[i++] = lcd_line_buffer[31];
   out[i++] = ' ';
   out[i++] = 'G';
   out[i++] = '2';
   out[i++] = '=';
   out[i++] = lcd_line_buffer[39];
   out[i++] = lcd_line_buffer[40];
   out[i++] = lcd_line_buffer[41];
   out[20] = '\0';
}

void display_build_row2(const uint8_t *lcd_line_buffer, uint8_t edit_field_index, uint8_t alarm_error_bits, char *out)
{
   uint8_t i = 0, e = ' ';
   if ((alarm_error_bits & OVERIH) == OVERIH) e = 'H';
   if ((alarm_error_bits & OVERIA) == OVERIA) e = 'A';
   if ((alarm_error_bits & OVERIG) == OVERIG) e = 'G';
   if ((alarm_error_bits & OVERTE) == OVERTE) e = 'T';
   (void)edit_field_index;
   out[i++] = e;
   out[i++] = ' ';
   out[i++] = lcd_line_buffer[19];
   out[i++] = lcd_line_buffer[20];
   out[i++] = lcd_line_buffer[21];
   out[i++] = ' ';
   out[i++] = 'm';
   out[i++] = 'A';
   out[i++] = ' ';
   out[i++] = lcd_line_buffer[33];
   out[i++] = lcd_line_buffer[34];
   out[i++] = lcd_line_buffer[35];
   out[i++] = lcd_line_buffer[36];
   out[i++] = lcd_line_buffer[37];
   out[i++] = ' ';
   out[i++] = lcd_line_buffer[43];
   out[i++] = lcd_line_buffer[44];
   out[i++] = lcd_line_buffer[45];
   out[i++] = lcd_line_buffer[46];
   out[i++] = lcd_line_buffer[47];
   out[20] = '\0';
}

void display_build_row3(uint16_t tube_type_index, uint16_t sequencer_phase_ticks, uint8_t debounce_set_button_hold_ticks, const uint8_t *lcd_line_buffer, uint8_t edit_field_index, char *out)
{
   uint8_t ascii[5];
   uint8_t i;
   (void)edit_field_index;
   if (tube_type_index > 1) {
      memset(out, ' ', 20);
      out[0] = 'S';
      out[1] = '=';
      out[2] = lcd_line_buffer[49];
      out[3] = lcd_line_buffer[50];
      out[4] = lcd_line_buffer[51];
      out[5] = lcd_line_buffer[52];
      out[6] = ' ';
      out[7] = 'R';
      out[8] = '=';
      out[9] = lcd_line_buffer[54];
      out[10] = lcd_line_buffer[55];
      out[11] = lcd_line_buffer[56];
      out[12] = lcd_line_buffer[57];
      if ((sequencer_phase_ticks <= (FUH+2)) || (debounce_set_button_hold_ticks == DMAX)) {
         out[13] = ' ';
         out[14] = 'K';
         out[15] = '=';
         out[16] = lcd_line_buffer[59];
         out[17] = lcd_line_buffer[60];
         out[18] = lcd_line_buffer[61];
         out[19] = lcd_line_buffer[62];
      } else {
         int2asc(sequencer_phase_ticks >> 2, ascii);
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
      if (tube_type_index == 0) {
         for (i = 0; i < 20; i++) out[i] = ' ';
      } else {
         memcpy(out, " TX/RX: 9600,8,N,1  ", 20);
      }
   }
   out[20] = '\0';
}

#ifndef VTTESTER_HOST_TEST
/* Draw current screen from lcd_line_buffer[]; row 3 depends on tube_type_index (local vs power supply/remote). */
void display_refresh(void)
{
   uint8_t i, alarm_error_code;
   uint8_t ascii[5];

   gotoxy(0, 0);
   str2lcd((edit_field_index == 0) && (sequencer_phase_ticks != (FUH+2)), &lcd_line_buffer[0]);

   gotoxy(3, 0);
   char2lcd(0, ' ');
   for (i = 0; i < 9; i++)
      char2lcd(((edit_field_index-1) == i) || ((i == 7) && (sequencer_phase_ticks == (FUH+2)) && ((lcd_line_buffer[11] == '1') || (lcd_line_buffer[11] == '2'))), lcd_line_buffer[i+4]);
   gotoxy(13, 0);
   cstr2lcd(0, (const uint8_t *)" G");
   char2lcd(edit_field_index == 10, '-');
   str2lcd(edit_field_index == 10, &lcd_line_buffer[24]);

   gotoxy(0, 1);
   cstr2lcd(0, (const uint8_t *)"H=");
   str2lcd(edit_field_index == 11, &lcd_line_buffer[14]);
   cstr2lcd(0, (const uint8_t *)"V A=");
   str2lcd(edit_field_index == 13, &lcd_line_buffer[29]);
   cstr2lcd(0, (const uint8_t *)" G2=");
   str2lcd(edit_field_index == 15, &lcd_line_buffer[39]);

   gotoxy(0, 2);
   alarm_error_code = ' ';
   if ((alarm_error_bits & OVERIH) == OVERIH) alarm_error_code = 'H';
   if ((alarm_error_bits & OVERIA) == OVERIA) alarm_error_code = 'A';
   if ((alarm_error_bits & OVERIG) == OVERIG) alarm_error_code = 'G';
   if ((alarm_error_bits & OVERTE) == OVERTE) alarm_error_code = 'T';
   char2lcd(0, alarm_error_code);
   char2lcd(0, ' ');
   str2lcd(edit_field_index == 12, &lcd_line_buffer[19]);
   cstr2lcd(0, (const uint8_t *)"mA ");
   str2lcd(edit_field_index == 14, &lcd_line_buffer[33]);
   char2lcd(0, ' ');
   str2lcd(edit_field_index == 16, &lcd_line_buffer[43]);

   gotoxy(0, 3);
   if (tube_type_index > 1) {
      gotoxy(0, 3);
      cstr2lcd(0, (const uint8_t *)"S=");
      str2lcd(edit_field_index == 17, &lcd_line_buffer[49]);
      gotoxy(6, 3);
      cstr2lcd(0, (const uint8_t *)" R=");
      str2lcd(edit_field_index == 18, &lcd_line_buffer[54]);
      gotoxy(13, 3);
      if ((sequencer_phase_ticks <= (FUH+2)) || (debounce_set_button_hold_ticks == DMAX)) {
         cstr2lcd(0, (const uint8_t *)" K=");
         str2lcd(edit_field_index == 19, &lcd_line_buffer[59]);
      } else {
         cstr2lcd(0, (const uint8_t *)" T=");
         int2asc(sequencer_phase_ticks >> 2, ascii);
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
      if (tube_type_index == 0) {
         cstr2lcd(0, (const uint8_t *)"                    ");
      } else {
         cstr2lcd(0, (const uint8_t *)" TX/RX: 9600,8,N,1  ");
      }
   }
}
#endif /* !VTTESTER_HOST_TEST */
