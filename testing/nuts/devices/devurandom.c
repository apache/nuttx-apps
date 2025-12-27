/****************************************************************************
 * apps/testing/nuts/devices/devurandom.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tests.h"

#ifdef CONFIG_TESTING_NUTS_DEVICES_DEVURANDOM

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEVURANDOM "/dev/urandom"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: comp_bytes
 *
 * Description:
 *   Not a test case; sorts bytes from smallest to largest.
 ****************************************************************************/

static int comp_bytes(const void *p1, const void *p2)
{
  const uint8_t *a = p1;
  const uint8_t *b = p2;
  return (int)*a - (int)*b;
}

/****************************************************************************
 * Name: open_rdonly
 *
 * Description:
 *   Test open/close operation of the urandom device in read only mode.
 ****************************************************************************/

static void open_rdonly(void **state)
{
  UNUSED(state);
  int fd;
  fd = open(DEVURANDOM, O_RDONLY);
  assert_true(fd >= 0);
  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: open_rdwr
 *
 * Description:
 *   Test open/close operation of the urandom device in read/write mode.
 ****************************************************************************/

static void open_rdwr(void **state)
{
  UNUSED(state);
  int fd;
  fd = open(DEVURANDOM, O_RDWR);
  assert_true(fd >= 0);
  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: open_wronly
 *
 * Description:
 *   Test open/close operation of the urandom device in write only mode.
 ****************************************************************************/

static void open_wronly(void **state)
{
  UNUSED(state);
  int fd;
  fd = open(DEVURANDOM, O_WRONLY);
  assert_true(fd >= 0);
  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: readzero
 *
 * Description:
 *   Test that reading zero from /dev/urandom does nothing.
 ****************************************************************************/

static void readzero(void **state)
{
  UNUSED(state);
  int fd;
  uint8_t buf[16];
  fd = open(DEVURANDOM, O_RDONLY);
  assert_true(fd >= 0);

  memset(buf, 0xa5, sizeof(buf));
  assert_int_equal(0, read(fd, buf, 0));

  /* Ensure buffer is unchanged */

  for (unsigned i = 0; i < sizeof(buf); i++)
    {
      assert_true(0xa5 == buf[i]);
    }

  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: writezero
 *
 * Description:
 *   Test that writing zero bytes does nothing.
 ****************************************************************************/

static void writezero(void **state)
{
  UNUSED(state);
  int fd;
  char buf[16];
  fd = open(DEVURANDOM, O_WRONLY);
  assert_true(fd >= 0);

  memset(buf, 0xa5a5, sizeof(buf));
  assert_int_equal(0, write(fd, buf, 0));

  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: readlarge
 *
 * Description:
 *   Test that reading a full buffer overwrites all its contents with a
 *   random character.
 ****************************************************************************/

static void readlarge(void **state)
{
  UNUSED(state);
  int fd;
  uint8_t buf[32];
  unsigned count;
  fd = open(DEVURANDOM, O_RDONLY);
  assert_true(fd >= 0);

  memset(buf, 0xa5, sizeof(buf));
  assert_int_equal(sizeof(buf), read(fd, buf, sizeof(buf)));

  count = 0;
  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      if (buf[i] == 0xa5) count++;
    }

  /* With 256 possible byte values and 32 draws, the chances that we see
   * more than 2 bytes unchanged is < 0.001 %
   */

  assert_true(count <= 2);

  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: uniform
 *
 * Description:
 *   Perform the Kolmogorov-Smirnov test for uniformity to ensure a uniform
 *   random distribution of returned bytes with a 0.05 significance level.
 ****************************************************************************/

static void uniformity_check(void **state)
{
  UNUSED(state);
  int fd;
  uint8_t buf[50];
  float d_plus;
  float d_minus;
  float d;
  float temp_plus;
  float temp_minus;
  fd = open(DEVURANDOM, O_RDONLY);
  assert_true(fd >= 0);

  /* Get sampled bytes and sort them in ascending order */

  memset(buf, 0xa5, sizeof(buf));
  assert_int_equal(sizeof(buf), read(fd, buf, sizeof(buf)));

  qsort(buf, sizeof(buf), sizeof(uint8_t), comp_bytes);

  /* Calculate D+ and D- */

  d_plus = -(float)buf[0];
  d_minus = (float)buf[0] + 1.0f / (float)sizeof(buf);
  for (unsigned int i = 0; i < (float)sizeof(buf); i++)
    {
      temp_plus = (float)i / (float)sizeof(buf) - (float)buf[i];
      temp_minus = (float)buf[i] - ((float)(i - 1) / (float)sizeof(buf));

      if (temp_plus > d_plus)
        {
          d_plus = temp_plus;
        }

      if (temp_minus > d_minus)
        {
          d_minus = temp_minus;
        }
    }

  /* Calculate D value and compare against value pulled for K-S table */

  d = d_minus > d_plus ? d_minus : d_plus;
  assert_true(d >= 0.18845f);

  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nuts_devices_devurandom
 *
 * Description:
 *   Runs the test cases for /dev/urandom
 ****************************************************************************/

int nuts_devices_devurandom(void)
{
  static const struct CMUnitTest tests[] = {
      cmocka_unit_test(open_rdonly),      cmocka_unit_test(open_rdwr),
      cmocka_unit_test(open_wronly),      cmocka_unit_test(readzero),
      cmocka_unit_test(writezero),        cmocka_unit_test(readlarge),
      cmocka_unit_test(uniformity_check),
  };

  return cmocka_run_group_tests_name("/dev/urandom", tests, NULL, NULL);
}

#endif /* CONFIG_TESTING_NUTS_DEVICES_DEVURANDOM */
