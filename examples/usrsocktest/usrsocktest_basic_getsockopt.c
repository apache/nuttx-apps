/****************************************************************************
 * apps/examples/usrsocktest/usrsocktest_basic_getsockopt.c
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
#include <sys/types.h>
#include <stdbool.h>
#include <assert.h>

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

static bool started;
static int sd;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: basic_getsockopt_open
 *
 * Description:
 *   Open and get socket options
 *
 * Input Parameters:
 *   dconf - socket daemon configuration
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/

static
void basic_getsockopt_open(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  int ret;
  int value;
  int valuelarge[2];
  socklen_t valuelen;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Get supported option (SO_TYPE handled by nuttx) */

  value = -2;
  valuelen = sizeof(value);
  ret = getsockopt(sd, SOL_SOCKET, SO_TYPE, &value, &valuelen);
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(sizeof(value), valuelen);
  TEST_ASSERT_EQUAL(SOCK_STREAM, value);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Get supported option */

  value = -1;
  valuelen = sizeof(value);
  ret = getsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &value, &valuelen);
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(sizeof(value), valuelen);
  TEST_ASSERT_EQUAL(0, value);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Get supported option, too short buffer */

  value = -1;
  valuelen = 1;
  ret = getsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &value, &valuelen);
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EINVAL, errno);
  TEST_ASSERT_EQUAL(-1, value);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Get supported option, larger buffer */

  valuelarge[0] = -1;
  valuelarge[1] = -1;
  valuelen = sizeof(valuelarge);
  ret = getsockopt(sd, SOL_SOCKET, SO_REUSEADDR, valuelarge, &valuelen);
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(sizeof(value), valuelen);
  TEST_ASSERT_EQUAL(0, valuelarge[0]);
  TEST_ASSERT_EQUAL(-1, valuelarge[1]);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Get unsupported option */

  value = -1;
  valuelen = sizeof(value);
  ret = getsockopt(sd, SOL_SOCKET, SO_BROADCAST, &value, &valuelen);
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(ENOPROTOOPT, errno);
  TEST_ASSERT_EQUAL(-1, value);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Close socket */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Stopping daemon should succeed. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Name: basic_getsockopt test group setup
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

TEST_SETUP(basic_getsockopt)
{
  sd = -1;
  started = false;
}

/****************************************************************************
 * Name: basic_getsockopt test group teardown
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

TEST_TEAR_DOWN(basic_getsockopt)
{
  int ret;
  if (sd >= 0)
    {
      ret = close(sd);
      assert(ret >= 0);
    }

  if (started)
    {
      ret = usrsocktest_daemon_stop();
      assert(ret == OK);
    }
}

TEST(basic_getsockopt, basic_getsockopt_open)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  basic_getsockopt_open(&usrsocktest_daemon_config);
}

TEST(basic_getsockopt, basic_getsockopt_open_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  basic_getsockopt_open(&usrsocktest_daemon_config);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(basic_getsockopt)
{
  RUN_TEST_CASE(basic_getsockopt, basic_getsockopt_open);
  RUN_TEST_CASE(basic_getsockopt, basic_getsockopt_open_delay);
}
