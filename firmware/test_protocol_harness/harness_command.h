#ifndef HARNESS_COMMAND_H
#define HARNESS_COMMAND_H

#include <stdint.h>

typedef struct {
	uint8_t a1_a2;
	uint8_t uhset;
	uint8_t ihset;
	uint8_t ugset;
	uint16_t uaset;
	uint16_t ueset;
} harness_params_t;

const harness_params_t *harness_params_get(void);
void harness_command_handle(const uint8_t *rq);

#endif
