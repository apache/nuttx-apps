/****************************************************************************
 * apps/testing/nest/devices/devzero.c
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
#include <unistd.h>

#include <testing/unity.h>

#include "tests.h"

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
 * Name: devices_devzero__open_rdonly
 *
 * Description:
 *   Test open/close operation of the zero device in read only mode.
 ****************************************************************************/

static void devices_devzero__open_rdonly(void)
{
  int fd;
  fd = open(DEVZERO, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devzero__open_rdwr
 *
 * Description:
 *   Test open/close operation of the zero device in read/write mode.
 ****************************************************************************/

static void devices_devzero__open_rdwr(void)
{
  int fd;
  fd = open(DEVZERO, O_RDWR);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devzero__open_wronly
 *
 * Description:
 *   Test open/close operation of the zero device in write only mode.
 ****************************************************************************/

static void devices_devzero__open_wronly(void)
{
  int fd;
  fd = open(DEVZERO, O_WRONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devzero__readzero
 *
 * Description:
 *   Test that reading zero bytes does nothing to a buffer.
 ****************************************************************************/

static void devices_devzero__readzero(void)
{
  int fd;
  char buf[16];
  fd = open(DEVZERO, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 0xa5a5, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(0, read(fd, buf, 0));

  /* Ensure buffer contents are unchanged */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      TEST_ASSERT_EQUAL_UINT8(0xa5a5, buf[i]);
    }

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devzero__readlarge
 *
 * Description:
 *   Test that reading a full buffer of bytes completely zeroes the buffer.
 ****************************************************************************/

static void devices_devzero__readlarge(void)
{
  int fd;
  char buf[BUFSIZ];
  fd = open(DEVZERO, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 0xa5a5, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(sizeof(buf), read(fd, buf, 0));

  /* Ensure buffer contents are unchanged */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      TEST_ASSERT_EQUAL_UINT8(0, buf[i]);
    }

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devzero__writezero
 *
 * Description:
 *   Test that writing zero bytes does nothing.
 ****************************************************************************/

static void devices_devzero__writezero(void)
{
  int fd;
  char buf[16];
  fd = open(DEVZERO, O_WRONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 0xa5a5, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(0, write(fd, buf, 0));

  /* Ensure buffer contents are unchanged */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      TEST_ASSERT_EQUAL_UINT8(0xa5a5, buf[i]);
    }

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devzero__writelarge
 *
 * Description:
 *   Test that writing a buffer full of non-zero bytes returns the size of
 *   the buffer, but the buffer remains unchanged.
 ****************************************************************************/

static void devices_devzero__writelarge(void)
{
  int fd;
  char buf[16];
  fd = open(DEVZERO, O_WRONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 0xa5a5, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(sizeof(buf), write(fd, buf, sizeof(buf)));

  /* Ensure buffer contents are unchanged */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      TEST_ASSERT_EQUAL_UINT8(0xa5a5, buf[i]);
    }

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devzero__wrrd
 *
 * Description:
 *   Test that writing a buffer full of non-zero bytes returns the size of
 *   the buffer, but the buffer remains unchanged. Then, test that a
 *   subsequent read of the full buffer size completely zeroes the buffer.
 ****************************************************************************/

static void devices_devzero__wrrd(void)
{
  int fd;
  char buf[16];
  fd = open(DEVZERO, O_RDWR);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 0xa5a5, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(sizeof(buf), write(fd, buf, sizeof(buf)));

  /* Ensure buffer contents are unchanged */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      TEST_ASSERT_EQUAL_UINT8(0xa5a5, buf[i]);
    }

  TEST_ASSERT_EQUAL_INT(sizeof(buf), read(fd, buf, sizeof(buf)));

  /* Ensure buffer contents are zeroed */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      TEST_ASSERT_EQUAL_UINT8(0, buf[i]);
    }

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nest_devices_devzero
 *
 * Description:
 *   Runs the test cases for /dev/zero
 ****************************************************************************/

void nest_devices_devzero(void)
{
  RUN_TEST(devices_devzero__open_rdonly);
  RUN_TEST(devices_devzero__open_rdwr);
  RUN_TEST(devices_devzero__open_wronly);

  RUN_TEST(devices_devzero__readzero);
  RUN_TEST(devices_devzero__readlarge);

  RUN_TEST(devices_devzero__writezero);
  RUN_TEST(devices_devzero__writelarge);

  RUN_TEST(devices_devzero__wrrd);
}
