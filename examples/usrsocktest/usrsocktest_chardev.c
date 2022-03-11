/****************************************************************************
 * apps/examples/usrsocktest/usrsocktest_chardev.c
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
#include <errno.h>

#include "defines.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int us_fd;
static int us_fd_two;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: char_dev test group setup
 *
 * Description:
 *   Setup function executed before each testcase in this test group
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/

TEST_SETUP(char_dev)
{
  us_fd = -1;
  us_fd_two = -1;
}

/****************************************************************************
 * Name: char_dev test group teardown
 *
 * Description:
 *   Setup function executed after each testcase in this test group
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/

TEST_TEAR_DOWN(char_dev)
{
  int ret;

  if (us_fd >= 0)
    {
      ret = close(us_fd);
      assert(ret >= 0);
    }

  if (us_fd_two >= 0)
    {
      ret = close(us_fd_two);
      assert(ret >= 0);
    }
}

/****************************************************************************
 * Name: open_rw
 *
 * Description:
 *   Simple test for opening and closing usrsock node
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/

TEST(char_dev, open_rw)
{
  int ret;

  us_fd = open(USRSOCK_NODE, O_RDWR);
  TEST_ASSERT_TRUE(us_fd >= 0);

  ret = close(us_fd);
  TEST_ASSERT_TRUE(ret >= 0);
  us_fd = -1;
}

/****************************************************************************
 * Name: reopen_rw
 *
 * Description:
 *   Repeated simple test for opening and closing usrsock node, reopen should
 *   work
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/

TEST(char_dev, reopen_rw)
{
  int ret;

  us_fd = open(USRSOCK_NODE, O_RDWR);
  TEST_ASSERT_TRUE(us_fd >= 0);

  ret = close(us_fd);
  TEST_ASSERT_TRUE(ret >= 0);
  us_fd = -1;

  us_fd = open(USRSOCK_NODE, O_RDWR);
  TEST_ASSERT_TRUE(us_fd >= 0);

  ret = close(us_fd);
  TEST_ASSERT_TRUE(ret >= 0);
  us_fd = -1;
}

/****************************************************************************
 * Name: no_multiple_open
 *
 * Description:
 *   No permission for multiple access,
 *   first user should be only user.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/

TEST(char_dev, no_multiple_open)
{
  us_fd = open(USRSOCK_NODE, O_RDWR);
  TEST_ASSERT_TRUE(us_fd >= 0);

  us_fd_two = open(USRSOCK_NODE, O_RDWR);
  TEST_ASSERT_FALSE(us_fd_two >= 0);
  TEST_ASSERT_EQUAL(EPERM, errno);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(char_dev)
{
  RUN_TEST_CASE(char_dev, open_rw);
  RUN_TEST_CASE(char_dev, reopen_rw);
  RUN_TEST_CASE(char_dev, no_multiple_open);
}
