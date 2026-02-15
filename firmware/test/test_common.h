/*
 * Common test helpers - assert and run count.
 */
#ifndef VTTESTER_TEST_COMMON_H
#define VTTESTER_TEST_COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>

extern uint32_t test_failed;
extern uint32_t test_run;

#define TEST_ASSERT(cond) do { \
   test_run++; \
   if (!(cond)) { \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      test_failed++; \
   } \
} while (0)

#define TEST_ASSERT_EQ(a, b) do { \
   test_run++; \
   if ((a) != (b)) { \
      fprintf(stderr, "FAIL %s:%d: %s == %s => %d != %d\n", __FILE__, __LINE__, #a, #b, (int)(a), (int)(b)); \
      test_failed++; \
   } \
} while (0)

#define TEST_ASSERT_MEM_EQ(a, b, n) do { \
   test_run++; \
   if (memcmp((a), (b), (n)) != 0) { \
      fprintf(stderr, "FAIL %s:%d: memcmp(%s, %s, %d)\n", __FILE__, __LINE__, #a, #b, (int)(n)); \
      test_failed++; \
   } \
} while (0)

#define TEST_RESET()  do { test_failed = 0; test_run = 0; } while (0)
#define TEST_FAILED() (test_failed)
#define TEST_RUN()    (test_run)

#endif /* VTTESTER_TEST_COMMON_H */
