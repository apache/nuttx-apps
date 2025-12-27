/****************************************************************************
 * apps/testing/nuts/devices/devzero.c
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tests.h"

#ifdef CONFIG_TESTING_NUTS_DEVICES_DEVZERO

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEVZERO "/dev/zero"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: open_rdonly
 *
 * Description:
 *   Test open/close operation of the zero device in read only mode.
 ****************************************************************************/

static void open_rdonly(void **state)
{
  UNUSED(state);
  int fd;
  fd = open(DEVZERO, O_RDONLY);
  assert_true(fd >= 0);
  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: open_rdwr
 *
 * Description:
 *   Test open/close operation of the zero device in read/write mode.
 ****************************************************************************/

static void open_rdwr(void **state)
{
  UNUSED(state);
  int fd;
  fd = open(DEVZERO, O_RDWR);
  assert_true(fd >= 0);
  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: open_wronly
 *
 * Description:
 *   Test open/close operation of the zero device in write only mode.
 ****************************************************************************/

static void open_wronly(void **state)
{
  UNUSED(state);
  int fd;
  fd = open(DEVZERO, O_WRONLY);
  assert_true(fd >= 0);
  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: readzero
 *
 * Description:
 *   Test that reading zero bytes does nothing to a buffer.
 ****************************************************************************/

static void readzero(void **state)
{
  UNUSED(state);
  int fd;
  char buf[16];
  fd = open(DEVZERO, O_RDONLY);
  assert_true(fd >= 0);

  memset(buf, 0xa5, sizeof(buf));
  assert_int_equal(0, read(fd, buf, 0));

  /* Ensure buffer contents are unchanged */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      assert_uint_equal(0xa5, (uint8_t)buf[i]);
    }

  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: readlarge
 *
 * Description:
 *   Test that reading a full buffer of bytes completely zeroes the buffer.
 ****************************************************************************/

static void readlarge(void **state)
{
  UNUSED(state);
  int fd;
  char buf[BUFSIZ];
  fd = open(DEVZERO, O_RDONLY);
  assert_true(fd >= 0);

  memset(buf, 0xa5, sizeof(buf));
  assert_int_equal(sizeof(buf), read(fd, buf, sizeof(buf)));

  /* Ensure buffer contents are zeroed */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      assert_uint_equal(0, (uint8_t)buf[i]);
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
  fd = open(DEVZERO, O_WRONLY);
  assert_true(fd >= 0);

  memset(buf, 0xa5, sizeof(buf));
  assert_int_equal(0, write(fd, buf, 0));

  /* Ensure buffer contents are unchanged */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      assert_uint_equal(0xa5, (uint8_t)buf[i]);
    }

  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: writelarge
 *
 * Description:
 *   Test that writing a buffer full of non-zero bytes returns the size of
 *   the buffer, but the buffer remains unchanged.
 ****************************************************************************/

static void writelarge(void **state)
{
  UNUSED(state);
  int fd;
  char buf[16];
  fd = open(DEVZERO, O_WRONLY);
  assert_true(fd >= 0);

  memset(buf, 0xa5, sizeof(buf));
  assert_int_equal(sizeof(buf), write(fd, buf, sizeof(buf)));

  /* Ensure buffer contents are unchanged */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      assert_uint_equal(0xa5, (uint8_t)buf[i]);
    }

  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: wrrd
 *
 * Description:
 *   Test that writing a buffer full of non-zero bytes returns the size of
 *   the buffer, but the buffer remains unchanged. Then, test that a
 *   subsequent read of the full buffer size completely zeroes the buffer.
 ****************************************************************************/

static void wrrd(void **state)
{
  UNUSED(state);
  int fd;
  char buf[16];
  fd = open(DEVZERO, O_RDWR);
  assert_true(fd >= 0);

  memset(buf, 0xa5, sizeof(buf));
  assert_int_equal(sizeof(buf), write(fd, buf, sizeof(buf)));

  /* Ensure buffer contents are unchanged */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      assert_uint_equal(0xa5, (uint8_t)buf[i]);
    }

  assert_int_equal(sizeof(buf), read(fd, buf, sizeof(buf)));

  /* Ensure buffer contents are zeroed */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      assert_uint_equal(0, (uint8_t)buf[i]);
    }

  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nuts_devices_devzero
 *
 * Description:
 *   Runs the test cases for /dev/zero
 ****************************************************************************/

int nuts_devices_devzero(void)
{
  static const struct CMUnitTest tests[] = {
      cmocka_unit_test(open_rdonly), cmocka_unit_test(open_rdwr),
      cmocka_unit_test(open_wronly), cmocka_unit_test(readzero),
      cmocka_unit_test(readlarge),   cmocka_unit_test(writezero),
      cmocka_unit_test(writelarge),  cmocka_unit_test(wrrd),
  };

  return cmocka_run_group_tests_name("/dev/zero", tests, NULL, NULL);
}

#endif /* CONFIG_TESTING_NUTS_DEVICES_DEVZERO */
