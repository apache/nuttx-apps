/****************************************************************************
 * apps/examples/usrsocktest/usrsocktest_block_recv.c
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
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

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
 * Name: connect_receive
 *
 * Description:
 *   Blocking connect and receive
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

static void connect_receive(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  ssize_t ret;
  size_t datalen;
  void *data;
  struct sockaddr_in addr;
  char databuf[5];

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  dconf->endpoint_block_connect = true;
  dconf->endpoint_block_send = true;
  dconf->endpoint_recv_avail_from_start = false;
  dconf->endpoint_recv_avail = 7;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Do connect, should succeed (after connect block released). */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('E', 100));
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Receive data from remote */

  data = databuf;
  datalen = sizeof(databuf);
  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('r', 100));
  ret = recvfrom(sd, data, datalen, 0, NULL, 0);
  TEST_ASSERT_EQUAL(datalen, ret);
  TEST_ASSERT_EQUAL(5, ret);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("abcde", data, 5);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(5, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Receive data from remote */

  data = databuf;
  datalen = sizeof(databuf);
  ret = recvfrom(sd, data, datalen, 0, NULL, 0);
  TEST_ASSERT_EQUAL(2, ret);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("ab", data, 2);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(7, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

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
 * Name: no_block_connect
 *
 * Description:
 *   Non-blocking connect and blocking receive
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

static void no_block_connect(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  ssize_t ret;
  size_t datalen;
  void *data;
  struct sockaddr_in addr;
  char databuf[5];
  struct sockaddr_in remoteaddr;
  socklen_t addrlen;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  dconf->endpoint_block_connect = false;
  dconf->endpoint_block_send = true;
  dconf->endpoint_recv_avail_from_start = true;
  dconf->endpoint_recv_avail = 6;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Do connect, should succeed (after connect block released). */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('W', 100));
  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('E', 100));
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Receive data from remote */

  data = databuf;
  datalen = sizeof(databuf);
  ret = read(sd, data, datalen);
  TEST_ASSERT_EQUAL(datalen, ret);
  TEST_ASSERT_EQUAL(5, ret);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("abcde", data, 5);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(5, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Receive data from remote */

  data = databuf;
  datalen = sizeof(databuf);
  addrlen = sizeof(remoteaddr);
  ret = recvfrom(sd, data, datalen, 0, (FAR struct sockaddr *)&remoteaddr,
                 &addrlen);
  TEST_ASSERT_EQUAL(1, ret);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("a", data, 1);
  TEST_ASSERT_EQUAL(sizeof(remoteaddr), addrlen);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(&remoteaddr, &addr,
                                addrlen - sizeof(addr.sin_zero));
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(6, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Receive data from remote */

  data = databuf;
  datalen = sizeof(databuf);
  addrlen = sizeof(remoteaddr);
  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('r', 100));
  ret = recvfrom(sd, data, datalen, 0, (FAR struct sockaddr *)&remoteaddr,
                 &addrlen);
  TEST_ASSERT_EQUAL(5, ret);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("abcde", data, 5);
  TEST_ASSERT_EQUAL(sizeof(remoteaddr), addrlen);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(&remoteaddr, &addr,
                                addrlen - sizeof(addr.sin_zero));
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(11, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Close socket */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(11, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Stopping daemon should succeed. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Name: receive_timeout
 *
 * Description:
 *   Blocking connect and receive with SO_RCVTIMEO
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

static void receive_timeout(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  ssize_t ret;
  size_t datalen;
  void *data;
  struct sockaddr_in addr;
  char databuf[5];
  struct timeval tv;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  dconf->endpoint_block_connect = true;
  dconf->endpoint_block_send = true;
  dconf->endpoint_recv_avail_from_start = false;
  dconf->endpoint_recv_avail = 7;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Do connect, should succeed (after connect block released). */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('E', 100));
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Setup recv timeout. */

  tv.tv_sec = 0;
  tv.tv_usec = 100 * 1000;
  ret = setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  TEST_ASSERT_EQUAL(0, ret);

  /* Receive data from remote */

  data = databuf;
  datalen = sizeof(databuf);
  ret = recvfrom(sd, data, datalen, 0, NULL, 0);
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EAGAIN, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

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
 * Name: peek_receive
 *
 * Description:
 *   Blocking connect and receive with MSG_PEEK flag
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

static void peek_receive(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  ssize_t ret;
  size_t datalen;
  void *data;
  struct sockaddr_in addr;
  char databuf[5];

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  dconf->endpoint_block_connect = true;
  dconf->endpoint_block_send = true;
  dconf->endpoint_recv_avail_from_start = false;
  dconf->endpoint_recv_avail = 7;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Do connect, should succeed (after connect block released). */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('E', 100));
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Receive data from remote with MSG_PEEK flag */

  data = databuf;
  datalen = sizeof(databuf);
  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('r', 100));
  ret = recvfrom(sd, data, datalen, MSG_PEEK, NULL, 0);
  TEST_ASSERT_EQUAL(datalen, ret);
  TEST_ASSERT_EQUAL(5, ret);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("abcde", data, 5);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(5, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Receive data from remote */

  data = databuf;
  datalen = sizeof(databuf);
  ret = recvfrom(sd, data, datalen, 0, NULL, 0);
  TEST_ASSERT_EQUAL(datalen, ret);
  TEST_ASSERT_EQUAL(5, ret);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("abcde", data, 5);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(10, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Receive data from remote with MSG_PEEK flag */

  data = databuf;
  datalen = sizeof(databuf);
  ret = recvfrom(sd, data, datalen, MSG_PEEK, NULL, 0);
  TEST_ASSERT_EQUAL(2, ret);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("ab", data, 2);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(12, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Receive data from remote */

  data = databuf;
  datalen = sizeof(databuf);
  ret = recvfrom(sd, data, datalen, 0, NULL, 0);
  TEST_ASSERT_EQUAL(2, ret);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("ab", data, 2);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(14, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

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
 * Name: block_recv test group setup
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

TEST_SETUP(block_recv)
{
  sd = -1;
  started = false;
}

/****************************************************************************
 * Name: block_recv test group teardown
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

TEST_TEAR_DOWN(block_recv)
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

TEST(block_recv, connect_receive)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  connect_receive(&usrsocktest_daemon_config);
}

TEST(block_recv, connect_receive_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  connect_receive(&usrsocktest_daemon_config);
}

TEST(block_recv, no_block_connect)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  no_block_connect(&usrsocktest_daemon_config);
}

TEST(block_recv, no_block_connect_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  no_block_connect(&usrsocktest_daemon_config);
}

TEST(block_recv, receive_timeout)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  receive_timeout(&usrsocktest_daemon_config);
}

TEST(block_recv, receive_timeout_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  receive_timeout(&usrsocktest_daemon_config);
}

TEST(block_recv, peek_receive)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  peek_receive(&usrsocktest_daemon_config);
}

TEST(block_recv, peek_receive_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  peek_receive(&usrsocktest_daemon_config);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(block_recv)
{
  RUN_TEST_CASE(block_recv, connect_receive);
  RUN_TEST_CASE(block_recv, connect_receive_delay);
  RUN_TEST_CASE(block_recv, no_block_connect);
  RUN_TEST_CASE(block_recv, no_block_connect_delay);
  RUN_TEST_CASE(block_recv, receive_timeout);
  RUN_TEST_CASE(block_recv, receive_timeout_delay);
  RUN_TEST_CASE(block_recv, peek_receive);
  RUN_TEST_CASE(block_recv, peek_receive_delay);
}
