/****************************************************************************
 * apps/examples/usrsocktest/usrsocktest_poll.c
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
#include <poll.h>
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
 * Name: connectreceive
 *
 * Description:
 *   Non-blocking connect and receive
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

static void connectreceive(struct usrsocktest_daemon_conf_s *dconf)
{
  int flags;
  int count;
  ssize_t ret;
  size_t datalen;
  void *data;
  struct sockaddr_in addr;
  char databuf[4];
  struct pollfd pfd;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  dconf->endpoint_recv_avail_from_start = false;
  dconf->endpoint_recv_avail = 3;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Make socket non-blocking */

  flags = fcntl(sd, F_GETFL, 0);
  TEST_ASSERT_TRUE(flags >= 0);
  TEST_ASSERT_EQUAL(O_RDWR, flags & O_RDWR);
  TEST_ASSERT_EQUAL(0, flags & O_NONBLOCK);
  ret = fcntl(sd, F_SETFL, flags | O_NONBLOCK);
  TEST_ASSERT_EQUAL(0, ret);
  flags = fcntl(sd, F_GETFL, 0);
  TEST_ASSERT_TRUE(flags >= 0);
  TEST_ASSERT_EQUAL(O_RDWR, flags & O_RDWR);
  TEST_ASSERT_EQUAL(O_NONBLOCK, flags & O_NONBLOCK);

  /* poll for input (instant timeout). */

  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = sd;
  pfd.events = POLLIN;
  ret = poll(&pfd, 1, 0);
  TEST_ASSERT_EQUAL(1, ret);
  TEST_ASSERT_EQUAL(0, pfd.revents & POLLERR);
  TEST_ASSERT_EQUAL(POLLHUP, pfd.revents & POLLHUP);
  TEST_ASSERT_EQUAL(POLLIN, pfd.revents & POLLIN);
  TEST_ASSERT_EQUAL(0, pfd.revents & POLLOUT);

  /* Do connect, should succeed instantly. */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  if (!dconf->delay_all_responses)
    {
      TEST_ASSERT_EQUAL(0, ret);
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
      TEST_ASSERT_EQUAL(0,
          usrsocktest_daemon_get_num_waiting_connect_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());
    }
  else
    {
      TEST_ASSERT_EQUAL(-1, ret);
      TEST_ASSERT_EQUAL(EINPROGRESS, errno);
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

      for (count = 0;
           usrsocktest_daemon_get_num_connected_sockets() != 1; count++)
        {
          TEST_ASSERT_TRUE(count <= 3);
          usleep(25 * 1000);
        }

      ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
      TEST_ASSERT_EQUAL(-1, ret);
      TEST_ASSERT_EQUAL(EISCONN, errno);
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());
    }

  /* poll for input (instant timeout). */

  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = sd;
  pfd.events = POLLIN;
  ret = poll(&pfd, 1, 0);
  TEST_ASSERT_EQUAL(0, ret);

  /* poll for input (with timeout). */

  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = sd;
  pfd.events = POLLIN;
  ret = poll(&pfd, 1, 10);
  TEST_ASSERT_EQUAL(0, ret);

  /* poll for input (no timeout). */

  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = sd;
  pfd.events = POLLIN;
  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('r', 100));
  ret = poll(&pfd, 1, -1);
  TEST_ASSERT_EQUAL(1, ret);
  TEST_ASSERT_EQUAL(POLLIN, pfd.revents);

  /* Receive data from remote, daemon returns 3 bytes. */

  data = databuf;
  datalen = sizeof(databuf);
  ret = recvfrom(sd, data, datalen, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(3, ret);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("abc", data, 3);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(3, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* poll for input (instant timeout). */

  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = sd;
  pfd.events = POLLIN;
  ret = poll(&pfd, 1, 0);
  TEST_ASSERT_EQUAL(0, ret);

  /* Make more data avail */

  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('r', 0));
  for (count = 0;
       usrsocktest_daemon_get_num_recv_empty_sockets() > 0; count++)
    {
      TEST_ASSERT_TRUE(count <= 3);
      usleep(5 * 1000);
    }

  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* poll for input (no timeout). */

  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = sd;
  pfd.events = POLLIN;
  ret = poll(&pfd, 1, -1);
  TEST_ASSERT_EQUAL(1, ret);
  TEST_ASSERT_EQUAL(POLLIN, pfd.revents);

  /* Receive data from remote, daemon returns 3 bytes. */

  data = databuf;
  datalen = sizeof(databuf);
  ret = recvfrom(sd, data, datalen, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(3, ret);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("abc", data, 3);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(6, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Close socket */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(6, usrsocktest_daemon_get_recv_bytes());
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
 * Name: connectsend
 *
 * Description:
 *   Non-blocking connect and receive
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

static void connectsend(struct usrsocktest_daemon_conf_s *dconf)
{
  int flags;
  ssize_t ret;
  size_t datalen;
  void *data;
  struct sockaddr_in addr;
  struct pollfd pfd;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  dconf->endpoint_recv_avail_from_start = true;
  dconf->endpoint_recv_avail = 3;
  dconf->endpoint_block_send = false;
  dconf->endpoint_block_connect = true;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Make socket non-blocking */

  flags = fcntl(sd, F_GETFL, 0);
  TEST_ASSERT_TRUE(flags >= 0);
  TEST_ASSERT_EQUAL(O_RDWR, flags & O_RDWR);
  TEST_ASSERT_EQUAL(0, flags & O_NONBLOCK);
  ret = fcntl(sd, F_SETFL, flags | O_NONBLOCK);
  TEST_ASSERT_EQUAL(0, ret);
  flags = fcntl(sd, F_GETFL, 0);
  TEST_ASSERT_TRUE(flags >= 0);
  TEST_ASSERT_EQUAL(O_RDWR, flags & O_RDWR);
  TEST_ASSERT_EQUAL(O_NONBLOCK, flags & O_NONBLOCK);

  /* poll for input (instant timeout). */

  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = sd;
  pfd.events = POLLIN;
  ret = poll(&pfd, 1, 0);
  TEST_ASSERT_EQUAL(1, ret);
  TEST_ASSERT_EQUAL(0, pfd.revents & POLLERR);
  TEST_ASSERT_EQUAL(POLLHUP, pfd.revents & POLLHUP);
  TEST_ASSERT_EQUAL(POLLIN, pfd.revents & POLLIN);
  TEST_ASSERT_EQUAL(0, pfd.revents & POLLOUT);

  /* Start non-blocking connect. */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EINPROGRESS, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* poll for input (no timeout).
   * As send is ready after established connection,
   * poll will exit with POLLOUT.
   */

  memset(&pfd, 0, sizeof(pfd));
  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('E', 100));
  pfd.fd = sd;
  pfd.events = POLLOUT;
  ret = poll(&pfd, 1, -1);
  TEST_ASSERT_EQUAL(1, ret);
  TEST_ASSERT_EQUAL(POLLOUT, pfd.revents);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EISCONN, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Send data to remote. */

  data = "abcdeFG";
  datalen = strlen(data);
  ret = write(sd, data, datalen);
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
 * Name: daemonabort
 *
 * Description:
 *   poll with daemon abort
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

static void daemonabort(struct usrsocktest_daemon_conf_s *dconf)
{
  int flags;
  ssize_t ret;
  struct sockaddr_in addr;
  struct pollfd pfd;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  dconf->endpoint_recv_avail_from_start = false;
  dconf->endpoint_recv_avail = 3;
  dconf->endpoint_block_send = false;
  dconf->endpoint_block_connect = true;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Make socket non-blocking */

  flags = fcntl(sd, F_GETFL, 0);
  TEST_ASSERT_TRUE(flags >= 0);
  TEST_ASSERT_EQUAL(O_RDWR, flags & O_RDWR);
  TEST_ASSERT_EQUAL(0, flags & O_NONBLOCK);
  ret = fcntl(sd, F_SETFL, flags | O_NONBLOCK);
  TEST_ASSERT_EQUAL(0, ret);
  flags = fcntl(sd, F_GETFL, 0);
  TEST_ASSERT_TRUE(flags >= 0);
  TEST_ASSERT_EQUAL(O_RDWR, flags & O_RDWR);
  TEST_ASSERT_EQUAL(O_NONBLOCK, flags & O_NONBLOCK);

  /* poll for input (instant timeout). */

  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = sd;
  pfd.events = POLLIN;
  ret = poll(&pfd, 1, 0);
  TEST_ASSERT_EQUAL(1, ret);
  TEST_ASSERT_EQUAL(0, pfd.revents & POLLERR);
  TEST_ASSERT_EQUAL(POLLHUP, pfd.revents & POLLHUP);
  TEST_ASSERT_EQUAL(POLLIN, pfd.revents & POLLIN);
  TEST_ASSERT_EQUAL(0, pfd.revents & POLLOUT);

  /* Start non-blocking connect. */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EINPROGRESS, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* poll for input (no timeout). Stop daemon forcefully. */

  memset(&pfd, 0, sizeof(pfd));
  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('S', 100));
  pfd.fd = sd;
  pfd.events = POLLOUT;
  ret = poll(&pfd, 1, -1);
  TEST_ASSERT_EQUAL(1, ret);
  TEST_ASSERT_EQUAL(POLLERR, pfd.revents & POLLERR);
  TEST_ASSERT_EQUAL(POLLHUP, pfd.revents & POLLHUP);

  /* Stopping daemon should succeed. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);

  /* poll for input (no timeout). */

  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = sd;
  pfd.events = POLLOUT;
  ret = poll(&pfd, 1, -1);
  TEST_ASSERT_EQUAL(1, ret);
  TEST_ASSERT_EQUAL(POLLERR, pfd.revents & POLLERR);
  TEST_ASSERT_EQUAL(POLLHUP, pfd.revents & POLLHUP);
  TEST_ASSERT_EQUAL(0, pfd.revents & POLLIN);
  TEST_ASSERT_EQUAL(0, pfd.revents & POLLOUT);

  /* Close socket */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_send_bytes());
  TEST_ASSERT_EQUAL(-ENODEV,
        usrsocktest_daemon_get_num_recv_empty_sockets());
}

/****************************************************************************
 * Name: poll test group setup
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

TEST_SETUP(poll)
{
  sd = -1;
  started = false;
}

/****************************************************************************
 * Name: poll test group teardown
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

TEST_TEAR_DOWN(poll)
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

TEST(poll, connectreceive)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  connectreceive(&usrsocktest_daemon_config);
}

TEST(poll, connectreceivedelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  connectreceive(&usrsocktest_daemon_config);
}

TEST(poll, connectsend)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  connectsend(&usrsocktest_daemon_config);
}

TEST(poll, connectsenddelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  connectsend(&usrsocktest_daemon_config);
}

TEST(poll, daemonabort)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  daemonabort(&usrsocktest_daemon_config);
}

TEST(poll, daemonabortdelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  daemonabort(&usrsocktest_daemon_config);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(poll)
{
  RUN_TEST_CASE(poll, connectreceive);
  RUN_TEST_CASE(poll, connectreceivedelay);
  RUN_TEST_CASE(poll, connectsend);
  RUN_TEST_CASE(poll, connectsenddelay);
  RUN_TEST_CASE(poll, daemonabort);
  RUN_TEST_CASE(poll, daemonabortdelay);
}
