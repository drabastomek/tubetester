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
void send_cyrillic_chars(void);                  /* send cyrillic characters to LCD */
void show_splash(void);                          /* show splash screen */
void update_display(void);                       /* update display */

void prepare_values_to_display(void);
void update_name_and_number(void);
void update_ug1(void);
void update_uh(void);
void update_ih(void);
void update_ua(void);
void update_ia(void);
void update_ug2(void);
void update_ig2(void);
void update_s(void);
void update_r(void);
void update_k(void);

#endif