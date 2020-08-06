/****************************************************************************
 * examples/usrsocktest/usrsocktest_chardev.c
 * Character device node tests
 *
 *   Copyright (C) 2015, 2017 Haltian Ltd. All rights reserved.
 *   Authors: Roman Saveljev <roman.saveljev@haltian.com>
 *            Jussi Kivilinna <jussi.kivilinna@haltian.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
