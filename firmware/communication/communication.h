#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdint.h>

// FRAME LENGTHS
#define FRAME_RX_BYTES 10u
#define FRAME_TX_DATA  19u
#define FRAME_TX_ACK   2u
#define FRAME_TX_ERROR 3u
#define FRAME_TX_OOR_ERROR 5u

// COMMAND CODES
#define CMD_SET    0x00u
#define CMD_RESET  0x01u
#define CMD_STATUS 0x02u

// RESPONSE CODES
#define RSP_DATA       0x00u
#define RSP_ERROR      0x01u
#define RSP_ACK        0x02u
#define RSP_USER_BREAK 0x03u
#define RSP_ALARM      0x04u

// RESET CODES
#define RESET_HEATER 0x01u
#define RESET_FULL   0x02u

// ERROR CODES
#define VTT_ERR_UNKNOWN      0x00u
#define VTT_ERR_OUT_OF_RANGE 0x02u
#define VTT_ERR_CRC          0x03u
#define VTT_ERR_INVALID_CMD  0x04u
#define VTT_ERR_HARDWARE     0x06u

// ALARM CODES
#define OVERSAMP 2       // liczba probek do alarmu - 1
#define OVERIH  0x01u    // znacznik bledu IH
#define OVERIA  0x02u    // znacznik bledu IA
#define OVERIG  0x04u    // znacznik bledu IG2
#define OVERTE  0x08u    // znacznik wzrostu temperatury

// PARAMETER IDS
#define PARAM_ID_UH 0x01u
#define PARAM_ID_IH 0x02u
#define PARAM_ID_UG 0x03u
#define PARAM_ID_UA 0x04u
#define PARAM_ID_IA 0x05u
#define PARAM_ID_UE 0x06u
#define PARAM_ID_IE 0x07u
#define PARAM_ID_TM 0x08u

// CRC calculation
uint8_t  comm_crc8_run(const uint8_t *data, uint8_t len);

// LOW LEVEL SEND / RECIVE BYTES
// receive incoming byte
void comm_receive_byte(uint8_t b);

// transmit outgoing byte
void comm_transmit_byte(uint8_t b);

// MESSAGE HANDLING
// parse incoming message
uint8_t* comm_rx_msg(void);

// send ACK
void send_ack(void);

// send ERROR
void send_error(uint8_t error_code);

// send ALARM
void send_alarm(uint8_t alarm_code);

// send INPUT RANGE ERROR
void send_input_range_error(uint8_t parameter_id, uint16_t value);

// send DATA
void send_data(uint8_t a1_a2, uint16_t *data, uint8_t length);

// send USER_BREAK
void send_user_break(void);

// send LCD buffer
void send_lcd_buffer(uint8_t *buffer, uint8_t length);

// helper function to send message
void send_message(uint8_t *message, uint8_t length);

#endif
