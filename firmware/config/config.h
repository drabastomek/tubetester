
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define BIT(x) (1 << (x))
#define RATE 103 /* UBRR @ 16 MHz → 9600 baud (match production tester) */
#define LOW(x)  ((char)((int)(x)))
#define HIGH(x) ((char)(((int)(x))>>8))

// limits for the tester
#define UH_MIN 0u
#define UH_MAX 200u
#define UG_MIN 0u
#define UG_MAX 240u
#define UA_MIN 0u
#define UA_MAX 300u
#define UE_MIN 0u
#define UE_MAX 0u
#define IH_MIN 0u
#define IH_MAX 250u
#define IA_MIN 0u
#define IA_MAX 2000u   
#define IE_MIN 0u
#define IE_MAX 0u
#define S_MIN 0u
#define S_MAX 999u
#define TM_MIN 0u
#define TM_MAX 36u

void configure_ports(void);
void configure_processor(void);
void configure_uart(void);

#endif
