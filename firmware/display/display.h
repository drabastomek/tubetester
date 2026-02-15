/*
 * LCD display - output to 4x20 LCD from display buffer.
 * Primitives: cmd2lcd, gotoxy, char2lcd, cstr2lcd, str2lcd.
 * display_init() = LCD hw init + CGRAM + splash screen.
 * display_refresh() = draw current screen from main's buf[] (call each sync tick).
 */

#ifndef VTTESTER_DISPLAY_H
#define VTTESTER_DISPLAY_H

#include <stdint.h>

void display_init(void);
void display_refresh(void);

/* Build 20-char line for each row (null-terminated at [20]). Testable without hardware. */
void display_build_row0(const uint8_t *buf, uint8_t adr, uint16_t start, char *out);
void display_build_row1(const uint8_t *buf, uint8_t adr, char *out);
void display_build_row2(const uint8_t *buf, uint8_t adr, uint8_t err, char *out);
void display_build_row3(uint16_t typ, uint16_t start, uint8_t dusk0, const uint8_t *buf, uint8_t adr, char *out);

void cmd2lcd(uint8_t rs, uint8_t bajt);
void gotoxy(uint8_t x, uint8_t y);
void char2lcd(uint8_t f, uint8_t c);
void cstr2lcd(uint8_t f, const uint8_t *c);
void str2lcd(uint8_t f, uint8_t *c);

#endif /* VTTESTER_DISPLAY_H */
