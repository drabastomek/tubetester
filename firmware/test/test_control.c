/*
 * Unit tests for control_update: menu, katalog, buf[] fill from ADC/state.
 * Build with control_harness (globals + load_lamprom) and test config.
 */
#include "test_common.h"
#include "../control/control.h"
#include "config/config.h"
#include <string.h>

extern uint8_t lcd_line_buffer[64], edit_field_index, debounce_set_button_hold_ticks, save_eeprom_flag, read_eeprom_flag;
extern uint16_t tube_type_index;
extern uint16_t sequencer_phase_ticks, adc_vref_scaled, lcd_ug2, lcd_uh, lcd_ih, tube_type_index_prev;
extern uint16_t adc_mean_uh, adc_mean_ih, adc_mean_ug1, adc_mean_ua, adc_mean_ia, adc_mean_ug2, adc_mean_ig2;
extern uint16_t ia_range_hi;
extern katalog catalog_remote, catalog_current;

/* char_lookup_alphabet indices: A=0..Z=25, _=26, 0=27..9=35. tube_name[] uses these; lcd_line_buffer[11] = char_lookup_alphabet[tube_name[7]]. */
static void test_control_adr0_typ1_fills_buf_number_and_name(void)
{
   uint8_t ascii[5];
   memset(lcd_line_buffer, 0, sizeof(lcd_line_buffer));
   edit_field_index = 0;
   tube_type_index = 1;
   sequencer_phase_ticks = 0;
   debounce_set_button_hold_ticks = 0;
   /* tube_name as char_lookup_alphabet indices: 6=33, N=13, 1=28, P=15, space=26, G=6, 1=28 */
   catalog_remote.tube_name[0] = 33; catalog_remote.tube_name[1] = 13; catalog_remote.tube_name[2] = 28; catalog_remote.tube_name[3] = 15;
   catalog_remote.tube_name[4] = 26; catalog_remote.tube_name[5] = 26; catalog_remote.tube_name[6] = 26;
   catalog_remote.tube_name[7] = 6;  /* G */
   catalog_remote.tube_name[8] = 28;  /* 1 */

   control_update(ascii);

   TEST_ASSERT(lcd_line_buffer[0] == '0' && lcd_line_buffer[1] == '0' && lcd_line_buffer[2] == '1');
   TEST_ASSERT(lcd_line_buffer[4] == 33 && lcd_line_buffer[5] == 13 && lcd_line_buffer[6] == 28 && lcd_line_buffer[7] == 15);
   TEST_ASSERT(lcd_line_buffer[11] == 'G');  /* overwritten by char_lookup_alphabet[tube_name[7]] */
   TEST_ASSERT(lcd_line_buffer[12] == 28);
}

static void test_control_ug1_display_from_adc(void)
{
   uint8_t ascii[5];
   memset(lcd_line_buffer, 0, sizeof(lcd_line_buffer));
   edit_field_index = 0;
   tube_type_index = 1;
   sequencer_phase_ticks = 0;
   debounce_set_button_hold_ticks = 0;
   adc_vref_scaled = 1000;
   adc_mean_ug1 = 0;
   memcpy(catalog_remote.tube_name, "6N1P  G11", 9);

   control_update(ascii);

   /* ug1 path: ug1_calc_accum = (40960000 - adc_mean_ug1*725)>>16 * adc_vref_scaled / 1000; adc_mean_ug1=0 => 625 */
   TEST_ASSERT(lcd_line_buffer[24] == '6' && lcd_line_buffer[25] == '2' && lcd_line_buffer[26] == '.' && lcd_line_buffer[27] == '5');
}

static void test_control_typ2_loads_lamprom_name(void)
{
   uint8_t ascii[5];
   memset(lcd_line_buffer, 0, sizeof(lcd_line_buffer));
   edit_field_index = 0;
   tube_type_index = 2;
   sequencer_phase_ticks = 0;
   debounce_set_button_hold_ticks = 0;
   memcpy(catalog_remote.tube_name, "REMOTE   ", 9);

   control_update(ascii);

   /* tube_type_index==2: load_lamprom(2, &catalog_current) => "6N1P  G11" */
   TEST_ASSERT(memcmp(&lcd_line_buffer[4], "6N1P  G11", 9) == 0);
   TEST_ASSERT(lcd_line_buffer[0] == '0' && lcd_line_buffer[1] == '0' && lcd_line_buffer[2] == '2');
}

void run_control_tests(void)
{
   test_control_adr0_typ1_fills_buf_number_and_name();
   test_control_ug1_display_from_adc();
   test_control_typ2_loads_lamprom_name();
}
