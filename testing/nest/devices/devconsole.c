/****************************************************************************
 * apps/testing/nest/devices/devconsole.c
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

#define DEVCONSOLE "/dev/console"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: devices_devconsole__open_rdonly
 *
 * Description:
 *   Test open/close operation of the console device in read only mode.
 ****************************************************************************/

static void devices_devconsole__open_rdonly(void)
{
  int fd;
  fd = open(DEVCONSOLE, O_RDONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devconsole__open_rdwr
 *
 * Description:
 *   Test open/close operation of the console device in read/write mode.
 ****************************************************************************/

static void devices_devconsole__open_rdwr(void)
{
  int fd;
  fd = open(DEVCONSOLE, O_RDWR);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devconsole__open_wronly
 *
 * Description:
 *   Test open/close operation of the console device in write only mode.
 ****************************************************************************/

static void devices_devconsole__open_wronly(void)
{
  int fd;
  fd = open(DEVCONSOLE, O_WRONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Name: devices_devconsole__readzero
 *
 * Description:
 *   Test that reading zero from /dev/console does nothing.
 ****************************************************************************/

static void devices_devconsole__readzero(void)
{
  int fd;
  char buf[16];
  fd = open(DEVCONSOLE, O_RDONLY);
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
 * Name: devices_devconsole__writezero
 *
 * Description:
 *   Test that writing zero bytes does nothing.
 ****************************************************************************/

static void devices_devconsole__writezero(void)
{
  int fd;
  char buf[16];
  fd = open(DEVCONSOLE, O_WRONLY);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

  memset(buf, 'a', sizeof(buf));
  TEST_ASSERT_EQUAL_INT(0, write(fd, buf, 0));

  TEST_ASSERT_EQUAL(0, close(fd));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nest_devices_devconsole
 *
 * Description:
 *   Runs the test cases for /dev/console
 ****************************************************************************/

void nest_devices_devconsole(void)
{
  RUN_TEST(devices_devconsole__open_rdonly);
  RUN_TEST(devices_devconsole__open_rdwr);
  RUN_TEST(devices_devconsole__open_wronly);

  RUN_TEST(devices_devconsole__readzero);
  RUN_TEST(devices_devconsole__writezero);
}
