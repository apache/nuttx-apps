/****************************************************************************
 * apps/testing/nest/devices/devascii.c
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
#include <unistd.h>

#include <testing/unity.h>

#include "tests.h"

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
 * Name: devices_devascii__open_rdonly
 *
 * Description:
 *   Test open/close operation of the ascii device in read only mode.
 ****************************************************************************/

static void devices_devascii__open_rdonly(void)
{
  int fd;
  fd = open(DEVASCII, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devascii__open_rdwr
 *
 * Description:
 *   Test open/close operation of the ascii device in read/write mode.
 ****************************************************************************/

static void devices_devascii__open_rdwr(void)
{
  int fd;
  fd = open(DEVASCII, O_RDWR);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devascii__open_wronly
 *
 * Description:
 *   Test open/close operation of the ascii device in write only mode.
 ****************************************************************************/

static void devices_devascii__open_wronly(void)
{
  int fd;
  fd = open(DEVASCII, O_WRONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devascii__readzero
 *
 * Description:
 *   Test that reading zero from /dev/ascii does nothing.
 ****************************************************************************/

static void devices_devascii__readzero(void)
{
  int fd;
  char buf[16];
  fd = open(DEVASCII, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 'a', sizeof(buf));
  TEST_ASSERT_EQUAL_INT(0, read(fd, buf, 0));

  /* Ensure buffer is unchanged */

  for (unsigned i = 0; i < sizeof(buf); i++)
    {
      TEST_ASSERT_EQUAL_CHAR('a', buf[i]);
    }

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devascii__readline
 *
 * Description:
 *   Test that reading one full line of characters returns the exact expected
 *   string of printable characters.
 *
 *   NOTE: -1 on buffer sizes allows us to keep ignore the null terminator.
 ****************************************************************************/

static void devices_devascii__readline(void)
{
  int fd;
  char buf[sizeof(g_printables)];
  fd = open(DEVASCII, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, '\0', sizeof(buf));
  TEST_ASSERT_EQUAL_INT(sizeof(buf) - 1, read(fd, buf, sizeof(buf) - 1));

  /* Ensure buffer contains the expected string. */

  TEST_ASSERT_EQUAL_MEMORY(g_printables, buf, sizeof(buf) - 1);

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devascii__readline_twice
 *
 * Description:
 *   Test that reading one full line of characters twice returns the exact
 *   expected string of printable characters twice.
 *
 *   NOTE: -1 on buffer sizes allows us to keep ignore the null terminator.
 ****************************************************************************/

static void devices_devascii__readline_twice(void)
{
  int fd;
  char buf[sizeof(g_printables) * 2];
  fd = open(DEVASCII, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, '\0', sizeof(buf));
  TEST_ASSERT_EQUAL_INT(sizeof(buf), read(fd, buf, sizeof(buf)));

  /* Ensure buffer contains the expected string twice */

  TEST_ASSERT_EQUAL_MEMORY(g_printables, buf, sizeof(g_printables) - 1);
  TEST_ASSERT_EQUAL_MEMORY(g_printables, &buf[sizeof(g_printables) - 1],
                           sizeof(g_printables) - 1);

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devascii__writezero
 *
 * Description:
 *   Test that writing zero bytes does nothing.
 ****************************************************************************/

static void devices_devascii__writezero(void)
{
  int fd;
  char buf[16];
  fd = open(DEVASCII, O_WRONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 'a', sizeof(buf));
  TEST_ASSERT_EQUAL_INT(0, write(fd, buf, 0));

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devascii__writelarge
 *
 * Description:
 *   Test that writing a full buffer of non-zero characters does nothing.
 ****************************************************************************/

static void devices_devascii__writelarge(void)
{
  int fd;
  char buf[BUFSIZ];
  fd = open(DEVASCII, O_WRONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 'a', sizeof(buf));
  TEST_ASSERT_EQUAL_INT(sizeof(buf), write(fd, buf, sizeof(buf)));

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nest_devices_devascii
 *
 * Description:
 *   Runs the test cases for /dev/ascii
 ****************************************************************************/

void nest_devices_devascii(void)
{
  RUN_TEST(devices_devascii__open_rdonly);
  RUN_TEST(devices_devascii__open_rdwr);
  RUN_TEST(devices_devascii__open_wronly);

  RUN_TEST(devices_devascii__readzero);
  RUN_TEST(devices_devascii__readline);
  RUN_TEST(devices_devascii__readline_twice);

  RUN_TEST(devices_devascii__writezero);
  RUN_TEST(devices_devascii__writelarge);
}
