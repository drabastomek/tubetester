/*
 * Minimal config for comms-testing harness only.
 * Full firmware replaces this directory; keep communication/ API compatible.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define BIT(x) (1 << (x))
#define RATE 103 /* UBRR @ 16 MHz → 9600 baud (match production tester) */

void configure_ports(void);
void configure_processor(void);
void configure_uart(void);

#endif
