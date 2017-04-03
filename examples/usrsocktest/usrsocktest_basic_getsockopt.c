/****************************************************************************
 * examples/usrsocktest/usrsocktest_basic_getsockopt.c
 * Basic getsockopt tests
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
 * Name: Open
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

static void Open(FAR struct usrsocktest_daemon_conf_s *dconf)
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
 * Name: BasicGetSockOpt test group setup
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

TEST_SETUP(BasicGetSockOpt)
{
  sd = -1;
  started = false;
}

/****************************************************************************
 * Name: BasicGetSockOpt test group teardown
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

TEST_TEAR_DOWN(BasicGetSockOpt)
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

TEST(BasicGetSockOpt, Open)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  Open(&usrsocktest_daemon_config);
}

TEST(BasicGetSockOpt, OpenDelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  Open(&usrsocktest_daemon_config);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(BasicGetSockOpt)
{
  RUN_TEST_CASE(BasicGetSockOpt, Open);
  RUN_TEST_CASE(BasicGetSockOpt, OpenDelay);
}
