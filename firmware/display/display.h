#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <avr/pgmspace.h>


void lcd_init(void);                             /* initialize LCD */    
void lcd_pulse_enable(void);                     /* pulse enable signal */
void lcd_write_nibble(uint8_t nibble);           /* write nibble to LCD */
void cmd2lcd(uint8_t rs, uint8_t bajt);          /* send command to LCD */
void gotoxy(uint8_t x, uint8_t y);               /* set cursor position */
void char2lcd(uint8_t highlight, uint8_t c);     /* display character */
void cstr2lcd(uint8_t highlight, const char* c); /* display string */
void cstr2lcd_P(uint8_t highlight, PGM_P p);     /* display string from flash */
void str2lcd(uint8_t f, uint8_t* c);             /* display string */
void int2asc( uint8_t liczba, uint8_t *ascii);   /* convert integer to ASCII */

#endif