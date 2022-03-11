/****************************************************************************
 * apps/examples/usrsocktest/usrsocktest_nodaemon.c
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

#include <sys/socket.h>
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

static int sd;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: no_daemon test group setup
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

TEST_SETUP(no_daemon)
{
  sd = -1;
}

/****************************************************************************
 * Name: no_daemon test group teardown
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

TEST_TEAR_DOWN(no_daemon)
{
  int ret;

  if (sd >= 0)
    {
      ret = close(sd);
      assert(ret >= 0);
    }
}

/****************************************************************************
 * Name: no_socket
 *
 * Description:
 *   Simple test for opening socket without usrsock daemon running
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

TEST(no_daemon, no_socket)
{
  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_EQUAL(-1, sd);
  TEST_ASSERT_TRUE(errno == EAFNOSUPPORT || errno == EPROTONOSUPPORT ||
                   errno == ENETDOWN);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(no_daemon)
{
  RUN_TEST_CASE(no_daemon, no_socket);
}
