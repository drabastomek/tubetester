/*
 * Test runner: run all unit tests and exit 0 if all pass, 1 otherwise.
 */
#include "test_common.h"
#include <inttypes.h>
#include <stdio.h>

uint32_t test_failed;
uint32_t test_run;

extern void run_protocol_tests(void);
extern void run_utils_tests(void);
extern void run_display_tests(void);
extern void run_control_tests(void);

int main(void)
{
   TEST_RESET();
   run_protocol_tests();
   run_utils_tests();
   run_display_tests();
   run_control_tests();
   if (TEST_FAILED()) {
      fprintf(stderr, "FAILED: %" PRIu32 " / %" PRIu32 "\n", TEST_FAILED(), TEST_RUN());
      return 1;
   }
   printf("OK: %" PRIu32 " tests\n", TEST_RUN());
   return 0;
}
