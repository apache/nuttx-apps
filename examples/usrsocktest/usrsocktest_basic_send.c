/****************************************************************************
 * apps/examples/usrsocktest/usrsocktest_basic_send.c
 * Basic sending tests
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

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
 * Name: basic_send_send
 *
 * Description:
 *   Open socket and send
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

static void basic_send_send(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  struct sockaddr_in addr;
  ssize_t ret;
  size_t datalen;
  const void *data;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_send_bytes());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Send on non-connected socket */

  data = "data";
  datalen = strlen("data");
  ret = send(sd, data, datalen, 0);
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(ENOTCONN, errno);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Sendto with address */

  data = "data";
  datalen = strlen("data");
  ret = sendto(sd, data, datalen, 0, (FAR const struct sockaddr *)&addr,
               sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(ENOTCONN, errno);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Connect socket to available endpoint */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());

  /* Sendto with address */

  data = "data";
  datalen = strlen("data");
  ret = sendto(sd, data, datalen, 0, (FAR const struct sockaddr *)&addr,
               sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EISCONN, errno);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());

  /* Close socket */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_send_bytes());

  /* Stopping daemon should succeed. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(-ENODEV,
                    usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_send_bytes());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Name: connect_send
 *
 * Description:
 *   Send over connected socket
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

static void connect_send(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  struct sockaddr_in addr;
  ssize_t ret;
  size_t datalen;
  const void *data;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_send_bytes());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Connect socket to available endpoint */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());

  /* Send data to remote */

  data = "data";
  datalen = strlen("data");
  ret = send(sd, data, datalen, 0);
  TEST_ASSERT_EQUAL(datalen, ret);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(datalen, usrsocktest_daemon_get_send_bytes());

  /* Close socket */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(datalen, usrsocktest_daemon_get_send_bytes());

  /* Stopping daemon should succeed. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(-ENODEV,
                    usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Name: basic_send test group setup
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

TEST_SETUP(basic_send)
{
  sd = -1;
  started = false;
}

/****************************************************************************
 * Name: basic_send test group teardown
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

TEST_TEAR_DOWN(basic_send)
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

TEST(basic_send, basic_send_send)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  basic_send_send(&usrsocktest_daemon_config);
}

TEST(basic_send, basic_send_send_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  basic_send_send(&usrsocktest_daemon_config);
}

TEST(basic_send, connect_send)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  connect_send(&usrsocktest_daemon_config);
}

TEST(basic_send, connect_send_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  connect_send(&usrsocktest_daemon_config);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(basic_send)
{
  RUN_TEST_CASE(basic_send, basic_send_send);
  RUN_TEST_CASE(basic_send, basic_send_send_delay);
  RUN_TEST_CASE(basic_send, connect_send);
  RUN_TEST_CASE(basic_send, connect_send_delay);
}
