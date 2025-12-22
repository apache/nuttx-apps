/****************************************************************************
 * apps/testing/nest/devices/devurandom.c
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

#include <nuttx/config.h>

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <testing/unity.h>

#include "tests.h"

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
  if (*a == *b) return 0;
  if (*a < *b) return -1;
  return 1;
}

/****************************************************************************
 * Name: devices_devurandom__open_rdonly
 *
 * Description:
 *   Test open/close operation of the urandom device in read only mode.
 ****************************************************************************/

static void devices_devurandom__open_rdonly(void)
{
  int fd;
  fd = open(DEVURANDOM, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devurandom__open_rdwr
 *
 * Description:
 *   Test open/close operation of the urandom device in read/write mode.
 ****************************************************************************/

static void devices_devurandom__open_rdwr(void)
{
  int fd;
  fd = open(DEVURANDOM, O_RDWR);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devurandom__open_wronly
 *
 * Description:
 *   Test open/close operation of the urandom device in write only mode.
 ****************************************************************************/

static void devices_devurandom__open_wronly(void)
{
  int fd;
  fd = open(DEVURANDOM, O_WRONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devurandom__readzero
 *
 * Description:
 *   Test that reading zero from /dev/urandom does nothing.
 ****************************************************************************/

static void devices_devurandom__readzero(void)
{
  int fd;
  uint8_t buf[16];
  fd = open(DEVURANDOM, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 0xa5, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(0, read(fd, buf, 0));

  /* Ensure buffer is unchanged */

  for (unsigned i = 0; i < sizeof(buf); i++)
    {
      TEST_ASSERT_EQUAL_CHAR(0xa5, buf[i]);
    }

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devurandom__writezero
 *
 * Description:
 *   Test that writing zero bytes does nothing.
 ****************************************************************************/

static void devices_devurandom__writezero(void)
{
  int fd;
  char buf[16];
  fd = open(DEVURANDOM, O_WRONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 0xa5a5, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(0, write(fd, buf, 0));

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devurandom__readlarge
 *
 * Description:
 *   Test that reading a full buffer overwrites all its contents with a
 *   random character.
 ****************************************************************************/

static void devices_devurandom__readlarge(void)
{
  int fd;
  uint8_t buf[32];
  unsigned count;
  fd = open(DEVURANDOM, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 0xa5, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(sizeof(buf), read(fd, buf, sizeof(buf)));

  count = 0;
  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      if (buf[i] == 0xa5) count++;
    }

  /* With 256 possible byte values and 32 draws, the chances that we see
   * more than 2 bytes unchanged is < 0.001 %
   */

  TEST_ASSERT_LESS_OR_EQUAL_UINT(2, count);

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devurandom__uniform
 *
 * Description:
 *   Perform the Kolmogorov-Smirnov test for uniformity to ensure a uniform
 *   random distribution of returned bytes with a 0.05 significance level.
 ****************************************************************************/

static void devices_devurandom__uniform(void)
{
  int fd;
  uint8_t buf[50];
  float d_plus;
  float d_minus;
  float d;
  float temp_plus;
  float temp_minus;
  fd = open(DEVURANDOM, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  /* Get sampled bytes and sort them in ascending order */

  memset(buf, 0xa5, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(sizeof(buf), read(fd, buf, sizeof(buf)));

  qsort(buf, sizeof(buf), sizeof(uint8_t), comp_bytes);

  /* Calculate D+ and D- */

  d_plus = -(float)buf[0];
  d_minus = (float)buf[0] + 1.0f / (float)sizeof(buf);
  for (unsigned int i = 0; i < (float)sizeof(buf); i++)
    {
      temp_plus = (float)i / (float)sizeof(buf) - (float)buf[i];
      temp_minus = (float)buf[i] - ((float)(i - 1) / (float)sizeof(buf));

      if (temp_plus > d_plus) d_plus = temp_plus;
      if (temp_minus > d_minus) d_minus = temp_minus;
    }

  /* Calculate D value and compare against value pulled for K-S table */

  d = d_minus > d_plus ? d_minus : d_plus;
  TEST_ASSERT_GREATER_THAN_FLOAT(0.18845f, d);

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nest_devices_devurandom
 *
 * Description:
 *   Runs the test cases for /dev/urandom
 ****************************************************************************/

void nest_devices_devurandom(void)
{
  RUN_TEST(devices_devurandom__open_rdonly);
  RUN_TEST(devices_devurandom__open_rdwr);
  RUN_TEST(devices_devurandom__open_wronly);

  RUN_TEST(devices_devurandom__readzero);
  RUN_TEST(devices_devurandom__writezero);

  RUN_TEST(devices_devurandom__readlarge);
  RUN_TEST(devices_devurandom__uniform);
}
