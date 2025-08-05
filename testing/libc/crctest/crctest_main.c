/****************************************************************************
 * apps/testing/libc/crctest/crctest_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmocka.h>

#include <nuttx/crc8.h>
#include <nuttx/crc16.h>
#include <nuttx/crc32.h>
#include <nuttx/crc64.h>

/****************************************************************************
 * Private Type
 ****************************************************************************/

typedef struct
{
  uint8_t length;
  uint8_t data[9];
} crc_test_data_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static crc_test_data_t crc_test_data[] =
{
  {
    4,
    { 0x00, 0x00, 0x00, 0x00 }
  },
  {
    3,
    { 0xf2, 0x01, 0x83 }
  },
  {
    4,
    { 0x0f, 0xaa, 0x00, 0x55 }
  },
  {
    4,
    { 0x00, 0xff, 0x55, 0x11 }
  },
  {
    9,
    { 0x33, 0x22, 0x55, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff }
  },
  {
    3,
    { 0x92, 0x6b, 0x55 }
  },
  {
    4,
    { 0xff, 0xff, 0xff, 0xff }
  },
};

static uint8_t crc8h1d_expected_crc[] =
{
  0x59, 0x37, 0x79, 0xb8, 0xcb, 0x8c, 0x74
};

static uint8_t crc8h2f_expected_crc[] =
{
  0x12, 0xc2, 0xc6, 0x77, 0x11, 0x33, 0x6c
};

static uint16_t crc16h1021_expected_crc[] =
{
  0x84c0, 0xd374, 0x2023, 0xb8f9, 0xf53f, 0x0745, 0x1d0f
};

static uint16_t crc16h8005_expected_crc[] =
{
  0x0000, 0xc2e1, 0x0be3, 0x6ccf, 0xae98, 0xe24e, 0x9401
};

static uint32_t crc32h04c11db7_expected_crc[] =
{
  0x2144df1c, 0x24ab9d77, 0xb6c9b287, 0x32a06212,
  0xb0ae863d, 0x9cdea29b, 0xffffffff
};

static uint32_t crc32hf4acfb13_expected_crc[] =
{
  0x6fb32240, 0x4f721a25, 0x20662df8, 0x9bd7996e,
  0xa65a343d, 0xee688a78, 0xffffffff
};

static uint64_t crc64emac_expected_crc[] =
{
  0xf4a586351e1b9f4b, 0x319c27668164f1c6,
  0x54c5d0f7667c1575, 0xa63822be7e0704e6,
  0x701eceb219a8e5d5, 0x5faa96a9b59f3e4e,
  0xffffffff00000000
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_case_crc8h1d
 ****************************************************************************/

static void test_case_crc8h1d(void **state)
{
  uint8_t index;
  uint8_t crc8_value;

  for (index = 0;
       index < sizeof(crc_test_data) / sizeof(crc_test_data[0]);
       index++)
    {
      crc8_value = crc8h1d(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_equal(crc8_value, crc8h1d_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
      crc8_value = crc8h1d(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_not_equal(crc8_value, crc8h1d_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
    }
}

/****************************************************************************
 * Name: test_case_crc8h2f
 ****************************************************************************/

static void test_case_crc8h2f(void **state)
{
  uint8_t index;
  uint8_t crc8_value;

  for (index = 0;
       index < sizeof(crc_test_data) / sizeof(crc_test_data[0]);
       index++)
    {
      crc8_value = crc8h2f(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_equal(crc8_value, crc8h2f_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
      crc8_value = crc8h2f(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_not_equal(crc8_value, crc8h2f_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
    }
}

/****************************************************************************
 * Name: test_case_crc16h1021
 ****************************************************************************/

static void test_case_crc16h1021(void **state)
{
  uint8_t index;
  uint16_t crc16_value;

  for (index = 0;
       index < sizeof(crc_test_data) / sizeof(crc_test_data[0]);
       index++)
    {
      crc16_value = crc16h1021(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_equal(crc16_value, crc16h1021_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
      crc16_value = crc16h1021(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_not_equal(crc16_value, crc16h1021_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
    }
}

/****************************************************************************
 * Name: test_case_crc16h8005
 ****************************************************************************/

static void test_case_crc16h8005(void **state)
{
  uint8_t index;
  uint16_t crc16_value;

  for (index = 0;
       index < sizeof(crc_test_data) / sizeof(crc_test_data[0]);
       index++)
    {
      crc16_value = crc16h8005(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_equal(crc16_value, crc16h8005_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
      crc16_value = crc16h8005(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_not_equal(crc16_value, crc16h8005_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
    }
}

/****************************************************************************
 * Name: test_case_crc32h04c11db7
 ****************************************************************************/

static void test_case_crc32h04c11db7(void **state)
{
  uint8_t index;
  uint32_t crc32_value;

  for (index = 0;
       index < sizeof(crc_test_data) / sizeof(crc_test_data[0]);
       index++)
    {
      crc32_value = crc32h04c11db7(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_equal(crc32_value, crc32h04c11db7_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
      crc32_value = crc32h04c11db7(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_not_equal(crc32_value, crc32h04c11db7_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
    }
}

/****************************************************************************
 * Name: test_case_crc32hf4acfb13
 ****************************************************************************/

static void test_case_crc32hf4acfb13(void **state)
{
  uint8_t index;
  uint32_t crc32_value;

  for (index = 0;
       index < sizeof(crc_test_data) / sizeof(crc_test_data[0]);
       index++)
    {
      crc32_value = crc32hf4acfb13(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_equal(crc32_value, crc32hf4acfb13_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
      crc32_value = crc32hf4acfb13(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_not_equal(crc32_value, crc32hf4acfb13_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
    }
}

/****************************************************************************
 * Name: test_case_crc64emac
 ****************************************************************************/

static void test_case_crc64emac(void **state)
{
  uint8_t index;
  uint64_t crc64_value;

  for (index = 0;
       index < sizeof(crc_test_data) / sizeof(crc_test_data[0]);
       index++)
    {
      crc64_value = crc64emac(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_equal(crc64_value, crc64emac_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
      crc64_value = crc64emac(crc_test_data[index].data,
        crc_test_data[index].length);
      assert_int_not_equal(crc64_value, crc64emac_expected_crc[index]);

      crc_test_data[index].data[0] ^= 0xff;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * crctest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test(test_case_crc8h1d),
    cmocka_unit_test(test_case_crc8h2f),
    cmocka_unit_test(test_case_crc16h1021),
    cmocka_unit_test(test_case_crc16h8005),
    cmocka_unit_test(test_case_crc32h04c11db7),
    cmocka_unit_test(test_case_crc32hf4acfb13),
    cmocka_unit_test(test_case_crc64emac),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
