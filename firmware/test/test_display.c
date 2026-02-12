/*
 * Unit tests for display row builders (display_build_row0..3).
 * Build with VTTESTER_HOST_TEST; links display.o (build functions only) and utils.
 */
#include "test_common.h"
#include "../display/display.h"
#include <string.h>

static unsigned char buf[64];

static void test_row0_number_name_ug1(void)
{
   char out[21];
   memset(buf, ' ', sizeof(buf));
   buf[0] = '0'; buf[1] = '0'; buf[2] = '1';
   buf[4] = '6'; buf[5] = 'N'; buf[6] = '1'; buf[7] = 'P'; buf[8] = ' '; buf[9] = ' '; buf[10] = ' '; buf[11] = ' '; buf[12] = ' ';
   buf[24] = '1'; buf[25] = '2'; buf[26] = '.'; buf[27] = '0';

   display_build_row0(buf, 0, 0, out);
   TEST_ASSERT(out[0] == '0' && out[1] == '0' && out[2] == '1');
   TEST_ASSERT(out[3] == ' ');
   TEST_ASSERT(out[4] == '6' && out[5] == 'N' && out[6] == '1' && out[7] == 'P');
   TEST_ASSERT(out[13] == ' ' && out[14] == 'G' && out[15] == '-');
   TEST_ASSERT(out[16] == '1' && out[17] == '2' && out[18] == '.' && out[19] == '0');
   TEST_ASSERT(out[20] == '\0');
}

static void test_row1_h_ua_ug2(void)
{
   char out[21];
   memset(buf, 0, sizeof(buf));
   buf[14] = '2'; buf[15] = '2'; buf[16] = '0'; buf[17] = ' ';
   buf[29] = '2'; buf[30] = '5'; buf[31] = '0';
   buf[39] = '1'; buf[40] = '0'; buf[41] = '0';

   display_build_row1(buf, 0, out);
   TEST_ASSERT(memcmp(out, "H=220 V A=250 G2=100", 20) == 0);
   TEST_ASSERT(out[20] == '\0');
}

static void test_row2_error_codes(void)
{
   char out[21];
   memset(buf, '0', sizeof(buf));
   buf[19] = '1'; buf[20] = '2'; buf[21] = '3';
   buf[33] = '4'; buf[34] = '5'; buf[35] = '.'; buf[36] = '0'; buf[37] = ' ';
   buf[43] = '6'; buf[44] = '7'; buf[45] = '.'; buf[46] = '0'; buf[47] = ' ';

   display_build_row2(buf, 0, 0, out);
   TEST_ASSERT(out[0] == ' ' && out[1] == ' ');
   TEST_ASSERT(out[2] == '1' && out[3] == '2' && out[4] == '3');
   TEST_ASSERT(memcmp(&out[9], "45.0 ", 5) == 0);
   TEST_ASSERT(memcmp(&out[15], "67.0 ", 5) == 0);
   TEST_ASSERT(out[20] == '\0');

   display_build_row2(buf, 0, 0x01, out);
   TEST_ASSERT(out[0] == 'H');
   display_build_row2(buf, 0, 0x02, out);
   TEST_ASSERT(out[0] == 'A');
   display_build_row2(buf, 0, 0x04, out);
   TEST_ASSERT(out[0] == 'G');
   display_build_row2(buf, 0, 0x08, out);
   TEST_ASSERT(out[0] == 'T');
}

static void test_row3_typ0_blank(void)
{
   char out[21];
   display_build_row3(0, 0, 0, buf, 0, out);
   for (int i = 0; i < 20; i++) TEST_ASSERT(out[i] == ' ');
   TEST_ASSERT(out[20] == '\0');
}

static void test_row3_typ1_serial(void)
{
   char out[21];
   display_build_row3(1, 0, 0, buf, 0, out);
   TEST_ASSERT(memcmp(out, " TX/RX: 9600,8,N,1  ", 20) == 0);
   TEST_ASSERT(out[20] == '\0');
}

static void test_row3_typ2_s_r_k(void)
{
   char out[21];
   memset(buf, 0, sizeof(buf));
   buf[49] = '1'; buf[50] = '2'; buf[51] = '0'; buf[52] = ' ';
   buf[54] = '5'; buf[55] = '0'; buf[56] = ' '; buf[57] = ' ';
   buf[59] = '3'; buf[60] = '0'; buf[61] = ' '; buf[62] = ' ';

   /* start <= FUH+2 => show K= */
   display_build_row3(2, 2, 0, buf, 0, out);
   TEST_ASSERT(out[0] == 'S' && out[1] == '=');
   TEST_ASSERT(memcmp(&out[2], "120 ", 4) == 0);
   TEST_ASSERT(out[7] == 'R' && out[8] == '=');
   TEST_ASSERT(memcmp(&out[9], "50  ", 4) == 0);
   TEST_ASSERT(out[13] == ' ' && out[14] == 'K' && out[15] == '=');
   TEST_ASSERT(memcmp(&out[16], "30  ", 4) == 0);
   TEST_ASSERT(out[20] == '\0');
}

static void test_row3_typ2_t_countdown(void)
{
   char out[21];
   memset(buf, 0, sizeof(buf));
   /* start >> 2 = 10 => "0100" in int2asc => 0,1,0,0 => show " 10s" (space, 1, 0, s) */
   display_build_row3(2, 40, 0, buf, 0, out);  /* start=40, dusk0=0, start>FUH+2 */
   TEST_ASSERT(out[14] == 'T' && out[15] == '=');
   TEST_ASSERT(out[19] == 's');
}

void run_display_tests(void)
{
   test_row0_number_name_ug1();
   test_row1_h_ua_ug2();
   test_row2_error_codes();
   test_row3_typ0_blank();
   test_row3_typ1_serial();
   test_row3_typ2_s_r_k();
   test_row3_typ2_t_countdown();
}
