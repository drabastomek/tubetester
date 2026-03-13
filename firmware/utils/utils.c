#include "utils.h"

uint8_t az_index(uint8_t c)
{
   if (c <= 36u) return c;
   if (c >= 'A' && c <= 'Z') return (uint8_t)(c - 'A');
   if (c == '_') return 26u;
   if (c >= '0' && c <= '9') return (uint8_t)(27u + (c - '0'));
   return 26u;
}