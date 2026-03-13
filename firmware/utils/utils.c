#include "utils.h"
#include "config/config.h"

uint8_t az_index(uint8_t c)
{
   if (c <= 36u) return c;
   if (c >= 'A' && c <= 'Z') return (uint8_t)(c - 'A');
   if (c == '_') return 26u;
   if (c >= '0' && c <= '9') return (uint8_t)(27u + (c - '0'));
   return 26u;
}

void zersrk(void)
{
   s = r = k = ualcd = ialcd = ug2lcd = ig2lcd = slcd = rlcd = klcd = 0;
}

uint16_t liczug1(uint16_t pug1)
{
   uint32_t acc = 640000UL;
   uint32_t sub = 1024000UL;
   acc *= vref;
   sub *= pug1;
   acc -= sub;
   acc /= 725;
   acc /= vref;
   return (uint16_t)acc;
}