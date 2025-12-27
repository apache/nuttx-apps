/****************************************************************************
 * apps/testing/nuts/devices/devconsole.c
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
#include <unistd.h>
#include <string.h>

#include "tests.h"

#ifdef CONFIG_TESTING_NUTS_DEVICES_DEVCONSOLE

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
 * Name: open_rdonly
 *
 * Description:
 *   Test open/close operation of the console device in read only mode.
 ****************************************************************************/

static void open_rdonly(void **state)
{
  UNUSED(state);
  int fd;
  fd = open(DEVCONSOLE, O_RDONLY);
  assert_true(fd >= 0);
  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: open_rdwr
 *
 * Description:
 *   Test open/close operation of the console device in read/write mode.
 ****************************************************************************/

static void open_rdwr(void **state)
{
  UNUSED(state);
  int fd;
  fd = open(DEVCONSOLE, O_RDWR);
  assert_true(fd >= 0);
  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: open_wronly
 *
 * Description:
 *   Test open/close operation of the console device in write only mode.
 ****************************************************************************/

static void open_wronly(void **state)
{
  UNUSED(state);
  int fd;
  fd = open(DEVCONSOLE, O_WRONLY);
  assert_true(fd >= 0);
  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Name: readzero
 *
 * Description:
 *   Test that reading zero from /dev/console does nothing.
 ****************************************************************************/

static void readzero(void **state)
{
  UNUSED(state);
  int fd;
  char buf[16];
  fd = open(DEVCONSOLE, O_RDONLY);
  assert_true(fd >= 0);

  memset(buf, 'a', sizeof(buf));
  assert_int_equal(0, read(fd, buf, 0));

  /* Ensure buffer is unchanged */

  for (unsigned i = 0; i < sizeof(buf); i++)
    {
      assert_true(buf[i] == 'a');
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
  fd = open(DEVCONSOLE, O_WRONLY);
  assert_true(fd >= 0);

  memset(buf, 'a', sizeof(buf));
  assert_int_equal(0, write(fd, buf, 0));

  assert_int_equal(0, close(fd));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nuts_devices_devconsole
 *
 * Description:
 *   Runs the test cases for /dev/console
 ****************************************************************************/

int nuts_devices_devconsole(void)
{
  static const struct CMUnitTest tests[] = {
      cmocka_unit_test(open_rdonly), cmocka_unit_test(open_rdwr),
      cmocka_unit_test(open_wronly), cmocka_unit_test(readzero),
      cmocka_unit_test(writezero),
  };

  return cmocka_run_group_tests_name("/dev/console", tests, NULL, NULL);
}

#endif /* CONFIG_TESTING_NUTS_DEVICES_DEVCONSOLE */
