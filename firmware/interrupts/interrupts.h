#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "config/config.h"

#include <stdint.h>

/* UART RX ring: defined in interrupts.c, declared here for ISR (in main) and handler */
extern volatile uint8_t uart_rx_ring[UART_RX_RING_SIZE];
extern volatile uint8_t uart_rx_head, uart_rx_tail;

/* ADC: ISR in main sets these and pushes EVENT_ADC; handler in interrupts.c processes */
extern volatile uint16_t adc_result;
extern volatile uint8_t adc_channel;

void int1_handler(void);
void uart_txc_handler(void);
void uart_rxc_handler(void);
void timer2_tick_handler(void);
void adc_handler(void);
#endif