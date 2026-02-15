/*
 * Unit tests for utils: int2asc (4 decimal digits, LSB first in ascii[0..3]).
 */
#include "test_common.h"
#include "../utils/utils.h"
#include <string.h>

/* int2asc writes units to ascii[0], tens to ascii[1], hundreds to ascii[2], thousands to ascii[3]. */
static void test_int2asc_zero(void)
{
   uint8_t ascii[5];
   memset(ascii, 0x55, sizeof(ascii));
   int2asc(0, ascii);
   TEST_ASSERT(ascii[0] == '0' && ascii[1] == '0' && ascii[2] == '0' && ascii[3] == '0');
}

static void test_int2asc_one_digit(void)
{
   uint8_t ascii[5];
   int2asc(7, ascii);
   TEST_ASSERT(ascii[0] == '7' && ascii[1] == '0' && ascii[2] == '0' && ascii[3] == '0');
}

static void test_int2asc_two_digits(void)
{
   uint8_t ascii[5];
   int2asc(42, ascii);
   TEST_ASSERT(ascii[0] == '2' && ascii[1] == '4' && ascii[2] == '0' && ascii[3] == '0');
}

static void test_int2asc_three_digits(void)
{
   uint8_t ascii[5];
   int2asc(123, ascii);
   TEST_ASSERT(ascii[0] == '3' && ascii[1] == '2' && ascii[2] == '1' && ascii[3] == '0');
}

static void test_int2asc_four_digits(void)
{
   uint8_t ascii[5];
   int2asc(9999, ascii);
   TEST_ASSERT(ascii[0] == '9' && ascii[1] == '9' && ascii[2] == '9' && ascii[3] == '9');
}

static void test_int2asc_300(void)
{
   uint8_t ascii[5];
   int2asc(300, ascii);
   TEST_ASSERT(ascii[0] == '0' && ascii[1] == '0' && ascii[2] == '3' && ascii[3] == '0');
}

static void test_int2asc_240(void)
{
   uint8_t ascii[5];
   int2asc(240, ascii);
   TEST_ASSERT(ascii[0] == '0' && ascii[1] == '4' && ascii[2] == '2' && ascii[3] == '0');
}

static void test_int2asc_255(void)
{
   uint8_t ascii[5];
   int2asc(255, ascii);
   TEST_ASSERT(ascii[0] == '5' && ascii[1] == '5' && ascii[2] == '2' && ascii[3] == '0');
}

void run_utils_tests(void)
{
   test_int2asc_zero();
   test_int2asc_one_digit();
   test_int2asc_two_digits();
   test_int2asc_three_digits();
   test_int2asc_four_digits();
   test_int2asc_300();
   test_int2asc_240();
   test_int2asc_255();
}
