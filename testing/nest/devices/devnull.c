/****************************************************************************
 * apps/testing/nest/devices/devnull.c
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

#define DEVNULL "/dev/null"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: devices_devnull__open_rdonly
 *
 * Description:
 *   Test open/close operation of the null device in read only mode.
 ****************************************************************************/

static void devices_devnull__open_rdonly(void)
{
  int fd;
  fd = open(DEVNULL, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devnull__open_rdwr
 *
 * Description:
 *   Test open/close operation of the null device in read/write mode.
 ****************************************************************************/

static void devices_devnull__open_rdwr(void)
{
  int fd;
  fd = open(DEVNULL, O_RDWR);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devnull__open_wronly
 *
 * Description:
 *   Test open/close operation of the null device in write only mode.
 ****************************************************************************/

static void devices_devnull__open_wronly(void)
{
  int fd;
  fd = open(DEVNULL, O_WRONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devnull__readzero
 *
 * Description:
 *   Test the result of reading 0 bytes.
 ****************************************************************************/

static void devices_devnull__readzero(void)
{
  int fd;
  uint8_t buf[16];
  fd = open(DEVNULL, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 0xa5a5, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(0, read(fd, buf, 0));

  /* Buffer contents should not have changed */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      TEST_ASSERT_EQUAL_UINT8(0xa5a5, buf[i]);
    }

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devnull__readlarge
 *
 * Description:
 *   Test the result of reading a full buffer of BUFSIZ bytes. Nothing should
 *   be read and nothing should happen to the buffer.
 ****************************************************************************/

static void devices_devnull__readlarge(void)
{
  int fd;
  uint8_t buf[BUFSIZ];
  fd = open(DEVNULL, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 0xa5a5, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(buf)));

  /* Buffer contents should be unchanged */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      TEST_ASSERT_EQUAL_UINT8(0xa5a5, buf[i]);
    }

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devnull__writezero
 *
 * Description:
 *   Test the result of writing zero bytes to /dev/null.
 ****************************************************************************/

static void devices_devnull__writezero(void)
{
  int fd;
  uint8_t buf[16];
  fd = open(DEVNULL, O_WRONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  TEST_ASSERT_EQUAL_INT(0, write(fd, buf, 0));

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devnull__writelarge
 *
 * Description:
 *   Test the result of writing a full buffer of non-zero bytes to /dev/null.
 ****************************************************************************/

static void devices_devnull__writelarge(void)
{
  int fd;
  uint8_t buf[BUFSIZ];
  fd = open(DEVNULL, O_WRONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 0xa5a5, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(sizeof(buf), write(fd, buf, sizeof(buf)));

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devnull__wrrd
 *
 * Description:
 *   Test the result of writing a buffer of non-zero bytes to /dev/null and
 *   then subsequently reading the whole buffer back. Nothing should happen
 *   to the buffer.
 ****************************************************************************/

static void devices_devnull__wrrd(void)
{
  int fd;
  uint8_t buf[BUFSIZ];
  fd = open(DEVNULL, O_RDWR);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 0xa5a5, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(sizeof(buf), write(fd, buf, sizeof(buf)));

  /* Buffer contents should be unchanged */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      TEST_ASSERT_EQUAL_UINT8(0xa5a5, buf[i]);
    }

  TEST_ASSERT_EQUAL_INT(0, read(fd, buf, sizeof(buf)));

  /* Buffer contents should be unchanged */

  for (unsigned int i = 0; i < sizeof(buf); i++)
    {
      TEST_ASSERT_EQUAL_UINT8(0xa5a5, buf[i]);
    }

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nest_devices_devnull
 *
 * Description:
 *   Runs the test cases for /dev/null
 ****************************************************************************/

void nest_devices_devnull(void)
{
  RUN_TEST(devices_devnull__open_rdonly);
  RUN_TEST(devices_devnull__open_rdwr);
  RUN_TEST(devices_devnull__open_wronly);

  RUN_TEST(devices_devnull__readzero);
  RUN_TEST(devices_devnull__readlarge);

  RUN_TEST(devices_devnull__writezero);
  RUN_TEST(devices_devnull__writelarge);

  RUN_TEST(devices_devnull__wrrd);
}
