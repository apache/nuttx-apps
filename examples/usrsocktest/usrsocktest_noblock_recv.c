/****************************************************************************
 * examples/usrsocktest/usrsocktest_noblock_recv.c
 * Receive from the socket in non-blocking mode
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
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
 * Name: receive
 *
 * Description:
 *   Non-blocking & instant connect+recv
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

static void receive(struct usrsocktest_daemon_conf_s *dconf)
{
  int flags;
  int count;
  ssize_t ret;
  size_t datalen;
  void *data;
  struct sockaddr_in addr;
  char databuf[4];
  struct sockaddr_in remoteaddr;
  socklen_t addrlen;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  dconf->endpoint_recv_avail_from_start = true;
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
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());
    }
  else
    {
      TEST_ASSERT_EQUAL(-1, ret);
      TEST_ASSERT_EQUAL(EINPROGRESS, errno);
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
      TEST_ASSERT_EQUAL(0,
                      usrsocktest_daemon_get_num_waiting_connect_sockets());

      for (count = 0; usrsocktest_daemon_get_num_connected_sockets() != 1;
           count++)
        {
          TEST_ASSERT_TRUE(count <= 3);
          usleep(25 * 1000);
        }

      ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
      TEST_ASSERT_EQUAL(-1, ret);
      TEST_ASSERT_EQUAL(EISCONN, errno);
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());
    }

  /* Receive data from remote, daemon returns 4 bytes. */

  data = databuf;
  datalen = sizeof(databuf);
  ret = recvfrom(sd, data, datalen, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(datalen, ret);
  TEST_ASSERT_EQUAL(4, datalen);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("abcd", data, 4);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(4, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Receive data from remote with address, daemon returns 3 bytes. */

  addrlen = sizeof(remoteaddr);
  ret = recvfrom(sd, data, datalen, 0, (FAR struct sockaddr *)&remoteaddr,
                 &addrlen);
  TEST_ASSERT_EQUAL(3, ret);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("abc", data, 3);
  TEST_ASSERT_EQUAL(addrlen, sizeof(remoteaddr));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(&remoteaddr, &addr, addrlen);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(datalen + ret, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Receive data from remote, daemon has 0 bytes buffered => -EAGAIN */

  data = databuf;
  datalen = sizeof(databuf);
  ret = recvfrom(sd, data, datalen, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EAGAIN, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(7, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Receive data from remote, daemon has 0 bytes buffered => -EAGAIN */

  data = databuf;
  datalen = sizeof(databuf);
  ret = read(sd, data, datalen);
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EAGAIN, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(7, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Reset recv buffer for open sockets */

  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('r', 0));
  for (count = 0;
       usrsocktest_daemon_get_num_recv_empty_sockets() > 0; count++)
    {
      TEST_ASSERT_TRUE(count <= 5);
      usleep(5 * 1000);
    }

  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Receive data from remote, daemon returns 4 bytes. */

  data = databuf;
  datalen = sizeof(databuf);
  ret = recvfrom(sd, data, datalen, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(datalen, ret);
  TEST_ASSERT_EQUAL(4, datalen);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("abcd", data, 4);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(7 + 4, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Close socket */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(7 + 4, usrsocktest_daemon_get_recv_bytes());
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
 * Name: delayed_connect
 *
 * Description:
 *   Non-blocking & delayed connect
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

static void delayed_connect(struct usrsocktest_daemon_conf_s *dconf)
{
  int flags;
  int count;
  ssize_t ret;
  size_t datalen;
  void *data;
  struct sockaddr_in addr;
  char databuf[4];
  struct sockaddr_in remoteaddr;
  socklen_t addrlen;

  /* Start test daemon. */

  dconf->endpoint_block_connect = true;
  dconf->endpoint_recv_avail_from_start = false;
  dconf->endpoint_recv_avail = 4;
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

  /* Launch connect attempt, daemon delays actual connection until
   * triggered.
   */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EINPROGRESS, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Try receive data, not connected yet. */

  data = databuf;
  datalen = sizeof(databuf);
  ret = read(sd, data, datalen);
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EAGAIN, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_send_bytes());

  /* Release delayed connect. */

  TEST_ASSERT_TRUE(usrsocktest_daemon_establish_waiting_connections());
  for (count = 0; usrsocktest_daemon_get_num_waiting_connect_sockets() > 0;
       count++)
    {
      TEST_ASSERT_TRUE(count <= 5);
      usleep(10 * 1000);
    }

  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Try receive data, not received data yet. */

  data = databuf;
  datalen = sizeof(databuf);
  addrlen = sizeof(remoteaddr);
  ret = recvfrom(sd, data, datalen, 0, (FAR struct sockaddr *)&remoteaddr,
                 &addrlen);
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EAGAIN, errno);
  TEST_ASSERT_EQUAL(0, addrlen);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_send_bytes());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Reset recv buffer for open sockets */

  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('r', 0));
  for (count = 0;
       usrsocktest_daemon_get_num_recv_empty_sockets() > 0; count++)
    {
      TEST_ASSERT_TRUE(count <= 5);
      usleep(5 * 1000);
    }

  /* Receive data from remote, daemon returns 4 bytes. */

  data = databuf;
  datalen = sizeof(databuf);
  ret = recvfrom(sd, data, datalen, 0, NULL, NULL);
  TEST_ASSERT_EQUAL(datalen, ret);
  TEST_ASSERT_EQUAL(4, datalen);
  TEST_ASSERT_EQUAL_UINT8_ARRAY("abcd", data, 4);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(4, usrsocktest_daemon_get_recv_bytes());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Close socket. */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Stopping daemon should succeed. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(-ENODEV,
                    usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(-ENODEV,
                    usrsocktest_daemon_get_num_recv_empty_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Name: no_block_recv test group setup
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

TEST_SETUP(no_block_recv)
{
  sd = -1;
  started = false;
}

/****************************************************************************
 * Name: no_block_recv test group teardown
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

TEST_TEAR_DOWN(no_block_recv)
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

TEST(no_block_recv, receive)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  receive(&usrsocktest_daemon_config);
}

TEST(no_block_recv, receive_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  receive(&usrsocktest_daemon_config);
}

TEST(no_block_recv, delayed_connect)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  delayed_connect(&usrsocktest_daemon_config);
}

TEST(no_block_recv, delayed_connect_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  delayed_connect(&usrsocktest_daemon_config);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(no_block_recv)
{
  RUN_TEST_CASE(no_block_recv, receive);
  RUN_TEST_CASE(no_block_recv, receive_delay);
  RUN_TEST_CASE(no_block_recv, delayed_connect);
  RUN_TEST_CASE(no_block_recv, delayed_connect_delay);
}
