/*
 * LCD display - output to 4x20 LCD from display buffer.
 * Primitives: cmd2lcd, gotoxy, char2lcd, cstr2lcd, str2lcd.
 * display_init() = LCD hw init + CGRAM + splash screen.
 * display_refresh() = draw current screen from main'slope_s lcd_line_buffer[] (call each main_loop_sync_flag tick).
 */

#ifndef VTTESTER_DISPLAY_H
#define VTTESTER_DISPLAY_H

#include <stdint.h>

void display_init(void);
void display_refresh(void);

/* Build 20-char line for each row (null-terminated at [20]). Testable without hardware. */
void display_build_row0(const uint8_t *lcd_line_buffer, uint8_t edit_field_index, uint16_t sequencer_phase_ticks, char *out);
void display_build_row1(const uint8_t *lcd_line_buffer, uint8_t edit_field_index, char *out);
void display_build_row2(const uint8_t *lcd_line_buffer, uint8_t edit_field_index, uint8_t alarm_error_bits, char *out);
void display_build_row3(uint16_t tube_type_index, uint16_t sequencer_phase_ticks, uint8_t debounce_set_button_hold_ticks, const uint8_t *lcd_line_buffer, uint8_t edit_field_index, char *out);

void cmd2lcd(uint8_t rs, uint8_t bajt);
void gotoxy(uint8_t x, uint8_t y);
void char2lcd(uint8_t f, uint8_t c);
void cstr2lcd(uint8_t f, const uint8_t *c);
void str2lcd(uint8_t f, uint8_t *c);

#endif /* VTTESTER_DISPLAY_H */
