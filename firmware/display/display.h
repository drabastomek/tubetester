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

void cmd2lcd(char rs, char bajt);
void gotoxy(char x, char y);
void char2lcd(char f, char c);
void cstr2lcd(char f, const unsigned char *c);
void str2lcd(char f, unsigned char *c);

#endif /* VTTESTER_DISPLAY_H */
