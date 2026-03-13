#include "display.h"
#include "config/config.h"

#include <util/delay.h>

void lcd_pulse_enable(void)
{
	PORTC |=  (1 << LCD_E);
	_delay_us(1);
	PORTC &= ~(1 << LCD_E);
	_delay_us(100);
}

void lcd_write_nibble(uint8_t nibble)
{
	PORTC &= ~((1 << PC4) | (1 << PC5) | (1 << PC6) | (1 << PC7));
	PORTC |= (nibble & 0x0F) << 4;
	lcd_pulse_enable();
}

void cmd2lcd( uint8_t rs, uint8_t bajt )
{
   if (rs) PORTC |=  (1 << LCD_RS);
	else    PORTC &= ~(1 << LCD_RS);
   lcd_write_nibble(bajt >> 4);
	lcd_write_nibble(bajt & 0x0F);
}

void gotoxy( uint8_t x, uint8_t y )
{
   // cmd2lcd( 0, 0x80 | (64 * (y % 2) + 20 * (y / 2) + x) ); // 4x20
   static const uint8_t line_addr[4] = { 0x00, 0x40, 0x14, 0x54 };
	if (y > 3 || x > 19) return;
	cmd2lcd(0, 0x80 | (line_addr[y] + x));
}

void char2lcd( uint8_t highlight, uint8_t c )
{
   /* Never send 0x00 (CGRAM 0) or 0xFF (full block) to LCD - can look like "filled squares" from bad/uninit data */
   if (c == 0x00 || c == 0xFF) c = ' ';
   (void)highlight;
   cmd2lcd(1, c);
}

void cstr2lcd( uint8_t highlight, const char* c )
{
   while( *c ) { char2lcd( highlight, (uint8_t)* c++ ); }
}

/* Send string from flash (PROGMEM) to LCD; use for PSTR("...") literals */
void cstr2lcd_P( uint8_t highlight, PGM_P p )
{
   uint8_t ch;
   while( (ch = pgm_read_byte(p)) != 0 )
   {
      if ( ch == 0xFF ) ch = ' ';
      char2lcd( highlight, ch );
      p++;
   }
}

void lcd_init(void)
{
	/* Wait for LCD power-up (DDRC/PORTC already set in main) */
	_delay_ms(50);

	lcd_write_nibble(0x03);
	_delay_ms(5);
	lcd_write_nibble(0x03);
	_delay_us(150);
	lcd_write_nibble(0x03);
	_delay_us(150);
	lcd_write_nibble(0x02);

	cmd2lcd(0, 0x28);
	cmd2lcd(0, 0x0C);
	cmd2lcd(0, 0x06);
	cmd2lcd(0, 0x01);
	_delay_ms(2);
}

void str2lcd( uint8_t f, uint8_t* c )
{
   uint8_t ch;
   while( (ch = *c) != 0 )
   {
      if ( ch == 0xFF ) ch = ' ';   /* avoid full block on LCD (e.g. bad/external buf) */
      char2lcd( f, ch );
      c++;
   }
}

void int2asc( uint8_t liczba, uint8_t *ascii)
{
   uint8_t i, temp;

   for( i = 0; i < 4; i++ )
   {
      temp = liczba % 10;
      liczba /= 10;
      ascii[i] = '0' + temp;
   }
}