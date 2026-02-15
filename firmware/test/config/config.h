/*
 * Minimal config for host unit tests (no AVR).
 * Defines only what protocol, display, and control tests need.
 */
#ifndef VTTESTER_CONFIG_H
#define VTTESTER_CONFIG_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Protocol SET limits (must match firmware config/config.h) */
#define VTTESTER_SET_HEAT_IDX_MAX    7
#define VTTESTER_SET_UA_MAX          300
#define VTTESTER_SET_UG2_MAX         300
#define VTTESTER_SET_UG1_MAX         240
#define VTTESTER_SET_TUH_INDEX_MAX   63
#define VTTESTER_SET_UA_SCALE        10
#define VTTESTER_SET_UG1_P4_STEP     5
#define VTTESTER_SET_TUH_TICK_SCALE  2

/* Display row builders (match firmware config) */
#define FUH        (uint16_t)(2)
#define DMIN      20
#define DMAX      250
#define OVERIH    0b00000001
#define OVERIA    0b00000010
#define OVERIG    0b00000100
#define OVERTE    0b00001000

/* Control tests: catalog and EEPROM stubs */
#define FLAMP_TEST  3u
#define ELAMP_TEST  2u
typedef struct {
   uint8_t nazwa[9];
   uint8_t uhdef;
   uint8_t ihdef;
   uint8_t ug1def;
   uint16_t  uadef;
   uint16_t  iadef;
   uint16_t  ug2def;
   uint16_t  ig2def;
   uint16_t  sdef;
   uint16_t  rdef;
   uint16_t  kdef;
} katalog;

#define FLAMP  FLAMP_TEST
#define ELAMP  ELAMP_TEST
#define EEPROM_READ(addr, var)  memcpy((void*)&(var), (const void*)(uintptr_t)(addr), sizeof(var))
#define EEPROM_WRITE(addr, val) memcpy((void*)(uintptr_t)(addr), (const void*)&(val), sizeof(val))

#endif /* VTTESTER_CONFIG_H */
