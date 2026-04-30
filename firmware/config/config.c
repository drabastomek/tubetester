#include "config.h"

void configure_processor(void)
{
	ACSR = BIT(ACD); /* wylacz komparator — same as full firmware */
}

void configure_ports(void)
{
	/* Konfiguracja portow */
	/*                        7   6   5   4   3   2   1   0
	                          UG1 IG2 UG2  IA  UA  UH  IH REZ */
	DDRA  = 0x00;
	PORTA = 0x00;
	/*                        D7  D6  D5  K1  UH CKG SEL RNG */
	DDRB  = 0x0f;
	PORTB = 0xf0;
	/*                        D7  D6  D5  D4  ENA RS SDA SCL */
	DDRC  = 0xfc;
	PORTC = 0x03;
	/*                        SPK DIR UG2  UA CLK  K0 TXD RXD */
	DDRD  = 0xb2;
	PORTD = 0x4f;
}

void configure_uart(void)
{
	UBRRL = RATE;
	UCSRB = BIT(RXCIE) | BIT(RXEN) | BIT(TXCIE) | BIT(TXEN);
	UCSRC = BIT(URSEL) | BIT(UCSZ1) | BIT(UCSZ0);
}
