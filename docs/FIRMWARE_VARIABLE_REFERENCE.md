# Firmware variable reference

List of **every variable** used in the main firmware (AVR build), with brief description and where it is defined and used. **Scope:** `TTesterLCD32.c`, `config/config.c`, `display/display.c`, `control/control.c`, `utils/utils.c`, `protocol/vttester_remote.c`. Test harness (`firmware/test/`) is excluded.

---

## Type: `katalog` (config/config.h)

Defined in **config/config.h** (typedef struct). Tube catalog entry: name, default voltages/currents, S/R/K.

| Member    | Type     | Description | New name proposal |
|-----------|----------|-------------|-------------------|
| nazwa[9] | uint8_t  | Tube name (ASCII) | tube_name (no change) |
| uhdef    | uint8_t  | Heater voltage set (0.1 V) | voltage_heater_set |
| ihdef    | uint8_t  | Heater current set | current_heater_set |
| ug1def   | uint8_t  | G1 voltage set (0.1 V) | voltage_g1_set |
| uadef    | uint16_t | Anode voltage set (V) | voltage_anode_set |
| iadef    | uint16_t | Anode current reference | current_anode_ref |
| ug2def   | uint16_t | Screen voltage set (V) | voltage_screen_set |
| ig2def   | uint16_t | Screen current reference | current_screen_ref | 
| sdef     | uint16_t | Slope S reference | slope_ref |
| rdef     | uint16_t | R reference | r_internal_ref |
| kdef     | uint16_t | K reference | k_amplification_ref | 

---

## TTesterLCD32.c — global variables (main app state)

All defined in **TTesterLCD32.c**. Used across main, ISRs, and (via `extern`) in `control/`, `display/`, `utils/`.

### uint8_t (lines 26–51)

| Variable | New name proposal | Description | Used in (file : ref count) |
|----------|--------------------|-------------|----------------------------|
| d, i | *(keep)* | Loop / temporary indices | TTesterLCD32.c |
| txen | tx_send_flag | Flag: send LCD/measurement dump to UART | TTesterLCD32.c |
| cwart | edit_byte_ptr | Pointer to current editable byte (name or param) | TTesterLCD32.c |
| cwartmin, cwartmax | edit_byte_min, edit_byte_max | Bounds for byte being edited | TTesterLCD32.c |
| adr | edit_field_index | Cursor: which field is edited (0=number, 1–19=name/params) | TTesterLCD32.c, display/display.c, control/control.c |
| adrmin, adrmax | edit_field_min, edit_field_max | Min/max for adr | TTesterLCD32.c |
| nowa | encoder_new_turn_flag | “New” / encoder step flag (first turn of knob) | TTesterLCD32.c, control/control.c |
| stop | measurement_stop_request | Request stop after current measurement | TTesterLCD32.c |
| dziel | sync_tick_divider | Count toward next sync (0..MRUG-1); when ≥MRUG, sync=1 | TTesterLCD32.c |
| nodus | debounce_encoder_ticks_stable_cnt | Encoder debounce: ticks before encoder is "stable" | TTesterLCD32.c, control/control.c |
| dusk0 | debounce_set_button_hold_ticks | SET button hold time (debounce / long-press) | TTesterLCD32.c, control/control.c; display/display.c |
| zapisz, czytaj | save_eeprom_flag, read_eeprom_flag | Save/read EEPROM flags | TTesterLCD32.c, control/control.c |
| range, rangelcd, rangedef | ia_range_hi, ia_range_lcd, ia_range_default | Ia range (0=low, 1=high), LCD value, default | TTesterLCD32.c, control/control.c; rangelcd in protocol (MEAS result) |
| ascii[5] | *(keep)* or int2ascii_buf | Temp buffer for int→ASCII (4 digits + null) | TTesterLCD32.c, control/control.c |
| kanal | adc_channel_index | Current ADC channel index (0..13) | TTesterLCD32.c |
| takt | display_blink_phase | Display tick / blink phase (0..3) | TTesterLCD32.c, display/display.c |
| overih, overia, overig2 | overcurrent_heater_count, overcurrent_anode_count, overcurrent_screen_count | Over-current debounce counters (heater, anode, screen) | TTesterLCD32.c |
| err, errcode | alarm_error_bits, alarm_error_code | Error / alarm state (bitmask) and code | TTesterLCD32.c, display/display.c |
| probki | adc_phase_sample_count | ADC sample count within LPROB averaging phase | TTesterLCD32.c |
| pwm | heater_pwm_duty | Heater PWM duty (0..255) | TTesterLCD32.c |
| anode | anode_system_select | Which anode system (28 or 29) for dual triodes | TTesterLCD32.c, control/control.c |
| bufin[10] | legacy_rx_buffer | Legacy 10-byte RX buffer (no CRC path) | TTesterLCD32.c, control/control.c |
| buf[64] | lcd_line_buffer | Main line buffer for LCD row (and UART dump) | TTesterLCD32.c, display/display.c, control/control.c |
| rx_proto_buf[], tx_proto_buf[] | proto_rx_frame, proto_tx_frame | 8-byte protocol RX/TX frames | TTesterLCD32.c |
| remote_meas_trigger, remote_meas_pending, remote_meas_ready | *(keep)* | Remote MEAS request state | TTesterLCD32.c |
| remote_meas_index | *(keep)* | Index byte for remote MEAS response | TTesterLCD32.c |

### volatile uint8_t (ISR ↔ main) (lines 54–60)

| Variable | New name proposal | Description | Used in |
|----------|--------------------|-------------|---------|
| busy | uart_tx_busy | UART TX in progress; USART_TXC clears, char2rs waits | TTesterLCD32.c, utils/utils.c |
| sync | main_loop_sync_flag | 1 ms tick; TIMER2_COMP sets, main clears after one loop | TTesterLCD32.c |
| zwloka | delay_ticks_remaining | delay() countdown; TIMER2_COMP decrements | TTesterLCD32.c, utils/utils.c |
| irx | legacy_rx_index | Legacy 10-byte RX position | TTesterLCD32.c |
| tout | legacy_rx_timeout_ticks | Legacy RX timeout countdown | TTesterLCD32.c |
| crc | legacy_crc | Legacy checksum (10-byte path) | TTesterLCD32.c |
| rx_proto_pos | proto_rx_pos | Position in rx_proto_buf | TTesterLCD32.c |
| rx_proto_ready | proto_frame_ready | Set when 8 bytes received (valid frame) | TTesterLCD32.c |

### uint16_t (lines 62–80)

| Variable | New name proposal | Description | Used in |
|----------|--------------------|-------------|---------|
| wart, wartmin, wartmax | edit_value_ptr, edit_value_min, edit_value_max | Pointer / bounds for current numeric parameter | TTesterLCD32.c |
| start | sequencer_phase_ticks | Sequencer phase down-counter (0 = idle) | TTesterLCD32.c, display/display.c, control/control.c |
| tuh | heating_time_ticks | Heating time (ticks); from SET or catalog | TTesterLCD32.c, control/control.c |
| vref | adc_vref_scaled | Reference voltage (e.g. 509); ADC/PWM scaling | TTesterLCD32.c, control/control.c, utils/utils.c |
| adcih, adcia, adcig2 | adc_raw_ih, adc_raw_ia, adc_raw_ig2 | ADC raw (heater I, anode I, screen I) | TTesterLCD32.c |
| uhset, ihset, ug1set, uaset, iaset, ug2set, ig2set | setpoint_uh, setpoint_ih, setpoint_ug1, setpoint_ua, setpoint_ia, setpoint_ug2, setpoint_ig2 | Setpoints (heater, Ug1, Ua, Ia, Ug2, Ig2) | TTesterLCD32.c, control/control.c |
| suhadc, sihadc, sug1adc, suaadc, siaadc, sug2adc, sig2adc | adc_sum_uh, adc_sum_ih, adc_sum_ug1, adc_sum_ua, adc_sum_ia, adc_sum_ug2, adc_sum_ig2 | ADC sums (before LPROB average) | TTesterLCD32.c |
| muhadc, mihadc, mug1adc, muaadc, miaadc, mug2adc, mig2adc | adc_mean_uh, adc_mean_ih, adc_mean_ug1, adc_mean_ua, adc_mean_ia, adc_mean_ug2, adc_mean_ig2 | Averaged ADC values | TTesterLCD32.c, control/control.c |
| ug1zer, ug1ref | ug1_dac_zero, ug1_dac_ref | Ug1 zero and reference for DAC | TTesterLCD32.c, control/control.c |
| uh, ih, ua, ual, uar, ia, ial, iar, ug2, ig2, ug1, ugl, ugr | meas_uh, meas_ih, meas_ua, meas_ua_left, meas_ua_right, meas_ia, meas_ia_left, meas_ia_right, meas_ug2, meas_ig2, meas_ug1, meas_ug1_left, meas_ug1_right | Measured/sampled voltages and currents | TTesterLCD32.c, control/control.c |
| s, r, k | slope_s, resistance_r, amplification_k | Computed S, R, K (slope, anode R, μ) | TTesterLCD32.c, control/control.c, utils/utils.c |
| typ, lastyp | tube_type_index, tube_type_index_prev | Current tube type index; previous type | TTesterLCD32.c, display/display.c, control/control.c |
| uhlcd, ihlcd, ug1lcd, ualcd, ialcd, ug2lcd, ig2lcd, slcd, rlcd, klcd | lcd_uh, lcd_ih, lcd_ug1, lcd_ua, lcd_ia, lcd_ug2, lcd_ig2, lcd_s, lcd_r, lcd_k | Values sent to LCD / remote MEAS | TTesterLCD32.c, control/control.c, utils/utils.c |
| srezadc, mrezadc | adc_sum_rez, adc_mean_rez | ADC sum/mean for REZ (NTC resistor / temp) | TTesterLCD32.c |
| bufinta, bufintg2 | legacy_decoded_ua, legacy_decoded_ug2 | Decoded Ua/Ug2 from legacy 10-byte frame | TTesterLCD32.c |

### uint32_t (lines 82–83)

| Variable | New name proposal | Description | Used in |
|----------|--------------------|-------------|---------|
| lint, tint | calc_temp_a, calc_temp_b | 32-bit temporaries in ADC/heater math | TTesterLCD32.c |
| licz, temp | ug1_calc_accum, ug1_calc_temp | 32-bit temporaries in liczug1 (Ug1 conversion) | TTesterLCD32.c, control/control.c, utils/utils.c |

### katalog (lines 85–87)

| Variable | New name proposal | Description | Used in |
|----------|--------------------|-------------|---------|
| lamprem | catalog_remote | “Remote” tube params (from SET or legacy 10-byte) | TTesterLCD32.c, control/control.c |
| lamptem | catalog_current | Current tube params (editing / measurement) | TTesterLCD32.c, control/control.c |

---

## config/config.c — globals

| Variable | New name proposal | Type | Description | Used in |
|----------|--------------------|------|-------------|---------|
| AZ[37] | char_lookup_alphabet | const uint8_t | Lookup A–Z, _, 0–9 for display | config.c (def), control/control.c |
| cyrB[8], … cyrZ[8] | cyrillic_cgram_* | uint8_t[8] | Cyrillic CGRAM patterns for LCD | config.c (def), display/display.c |
| poptyp | eeprom_last_tube_type | uint16_t or EEMEM | Last selected tube type (EEPROM) | config.c (def), control/control.c, TTesterLCD32.c |
| lampeep[ELAMP] | catalog_eeprom | katalog[] or EEMEM | User EEPROM tube catalog | config.c (def), control/control.c, TTesterLCD32.c |
| lamprom[FLAMP] | catalog_rom | const katalog[] PROGMEM | ROM tube catalog | config.c (def), load_lamprom() and callers |

**Functions:** `load_lamprom(uint16_t idx, katalog *dest)` — load one catalog entry from ROM (or EEPROM for user slots). Used in TTesterLCD32.c, control/control.c.

---

## display/display.c

No new global variables. Uses `extern` from main: `buf`, `adr`, `takt`, `dusk0`, `err`, `typ`, `start`. See TTesterLCD32.c section for those.

---

## control/control.c

No new global variables. Uses `extern` from main and config (see TTesterLCD32.c and config sections).

---

## utils/utils.c

No new global variables. Uses `extern`: `busy`, `zwloka`, `s`, `r`, `k`, `ualcd`, `ialcd`, `ug2lcd`, `ig2lcd`, `slcd`, `rlcd`, `klcd`, `vref`, `licz`, `temp`. See TTesterLCD32.c and config.

---

## protocol/vttester_remote.c

No global variables. Only static locals (e.g. CRC table) and function parameters. Parsed SET result uses `vttester_set_params_t` (e.g. `tuh_ticks`); that struct is filled in this file and read in TTesterLCD32.c.

---

## Getting exact file:line for every use

- **grep:**  
  `grep -rn '\bVAR\b' firmware/TTesterLCD32.c firmware/config/config.c firmware/display firmware/control firmware/utils firmware/protocol`
- **Doxygen:** Use the Doxyfile in `firmware/docs/`; build and open HTML → “Files” → select file → “Variable Documentation” / “Referenced by”.
