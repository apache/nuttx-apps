/****************************************************************************
 * apps/testing/nuts/devices/devascii.c
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tests.h"

#ifdef CONFIG_TESTING_NUTS_DEVICES_DEVASCII

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEVASCII "/dev/ascii"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The repeating string that is read from /dev/ascii */

static const char g_printables[] =
    "\n!\"#$%&'()*+,-./"
    "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
    "abcdefghijklmnopqrstuvwxyz{|}~";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: open_rdonly
 *
 * Description:
 *   Test open/close operation of the ascii device in read only mode.
 ****************************************************************************/

static void open_rdonly(void **state)
{
  UNUSED(state);
  int fd;
  fd = open(DEVASCII, O_RDONLY);
  assert_true(fd >= 0);
  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: open_rdwr
 *
 * Description:
 *   Test open/close operation of the ascii device in read/write mode.
 ****************************************************************************/

static void open_rdwr(void **state)
{
  UNUSED(state);
  int fd;
  fd = open(DEVASCII, O_RDWR);
  assert_true(fd >= 0);
  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: open_wronly
 *
 * Description:
 *   Test open/close operation of the ascii device in write only mode.
 ****************************************************************************/

static void open_wronly(void **state)
{
  UNUSED(state);
  int fd;
  fd = open(DEVASCII, O_WRONLY);
  assert_true(fd >= 0);
  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: readzero
 *
 * Description:
 *   Test that reading zero from /dev/ascii does nothing.
 ****************************************************************************/

static void readzero(void **state)
{
  UNUSED(state);
  int fd;
  char buf[16];
  fd = open(DEVASCII, O_RDONLY);
  assert_true(fd >= 0);

  memset(buf, 'a', sizeof(buf));
  assert_int_equal(0, read(fd, buf, 0));

  /* Ensure buffer is unchanged */

  for (unsigned i = 0; i < sizeof(buf); i++)
    {
      assert_true('a' == buf[i]);
    }

  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: readline
 *
 * Description:
 *   Test that reading one full line of characters returns the exact expected
 *   string of printable characters.
 *
 *   NOTE: -1 on buffer sizes allows us to keep ignore the null terminator.
 ****************************************************************************/

static void readline(void **state)
{
  UNUSED(state);
  int fd;
  char buf[sizeof(g_printables)];
  fd = open(DEVASCII, O_RDONLY);
  assert_true(fd >= 0);

  memset(buf, '\0', sizeof(buf));
  assert_int_equal(sizeof(buf) - 1, read(fd, buf, sizeof(buf) - 1));

  /* Ensure buffer contains the expected string. */

  assert_memory_equal(g_printables, buf, sizeof(buf) - 1);

  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: readline_twice
 *
 * Description:
 *   Test that reading one full line of characters twice returns the exact
 *   expected string of printable characters twice.
 *
 *   NOTE: -1 on buffer sizes allows us to keep ignore the null terminator.
 ****************************************************************************/

static void readline_twice(void **state)
{
  UNUSED(state);
  int fd;
  char buf[sizeof(g_printables) * 2];
  fd = open(DEVASCII, O_RDONLY);
  assert_true(fd >= 0);

  memset(buf, '\0', sizeof(buf));
  assert_int_equal(sizeof(buf), read(fd, buf, sizeof(buf)));

  /* Ensure buffer contains the expected string twice */

  assert_memory_equal(g_printables, buf, sizeof(g_printables) - 1);
  assert_memory_equal(g_printables, &buf[sizeof(g_printables) - 1],
                      sizeof(g_printables) - 1);

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
  fd = open(DEVASCII, O_WRONLY);
  assert_true(fd >= 0);

  memset(buf, 'a', sizeof(buf));
  assert_int_equal(0, write(fd, buf, 0));

  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: writelarge
 *
 * Description:
 *   Test that writing a full buffer of non-zero characters does nothing.
 ****************************************************************************/

static void writelarge(void **state)
{
  UNUSED(state);
  int fd;
  char buf[BUFSIZ];
  fd = open(DEVASCII, O_WRONLY);
  assert_true(fd >= 0);

  memset(buf, 'a', sizeof(buf));
  assert_int_equal(sizeof(buf), write(fd, buf, sizeof(buf)));

  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nuts_devices_devascii
 *
 * Description:
 *   Runs the test cases for /dev/ascii
 ****************************************************************************/

int nuts_devices_devascii(void)
{
  static const struct CMUnitTest tests[] = {
      cmocka_unit_test(open_rdonly), cmocka_unit_test(open_rdwr),
      cmocka_unit_test(open_wronly), cmocka_unit_test(readzero),
      cmocka_unit_test(readline),    cmocka_unit_test(readline_twice),
      cmocka_unit_test(writezero),   cmocka_unit_test(writelarge),
  };

  return cmocka_run_group_tests_name("/dev/ascii", tests, NULL, NULL);
}

#endif /* CONFIG_TESTING_NUTS_DEVICES_DEVASCII */
