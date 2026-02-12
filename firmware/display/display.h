/*
 * LCD display - output to 4x20 LCD from display buffer.
 * Primitives: cmd2lcd, gotoxy, char2lcd, cstr2lcd, str2lcd.
 * display_init() = LCD hw init + CGRAM + splash screen.
 * display_refresh() = draw current screen from main's buf[] (call each sync tick).
 */

#ifndef VTTESTER_DISPLAY_H
#define VTTESTER_DISPLAY_H

void display_init(void);
void display_refresh(void);

/* Build 20-char line for each row (null-terminated at [20]). Testable without hardware. */
void display_build_row0(const unsigned char *buf, unsigned char adr, unsigned int start, char *out);
void display_build_row1(const unsigned char *buf, unsigned char adr, char *out);
void display_build_row2(const unsigned char *buf, unsigned char adr, unsigned char err, char *out);
void display_build_row3(unsigned char typ, unsigned int start, unsigned char dusk0, const unsigned char *buf, unsigned char adr, char *out);

void cmd2lcd(char rs, char bajt);
void gotoxy(char x, char y);
void char2lcd(char f, char c);
void cstr2lcd(char f, const unsigned char *c);
void str2lcd(char f, unsigned char *c);

#endif /* VTTESTER_DISPLAY_H */
