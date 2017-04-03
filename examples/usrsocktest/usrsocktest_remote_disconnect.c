/****************************************************************************
 * usrsocktest/usrsocktest_remote_disconnect.c
 * Remote end disconnects
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

static const uint8_t tevents[] = { POLLIN, POLLOUT, POLLOUT|POLLIN, 0};
static bool started;
static int sd;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ConnectReceive
 *
 * Description:
 *   Remote end is unreachable
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

static void Unreachable(struct usrsocktest_daemon_conf_s *dconf)
{
  ssize_t ret;
  struct sockaddr_in addr;
  int flags;
  struct pollfd pfd;

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
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_unreachable_sockets());

  /* Try connect, connection not accepted. */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('F', 100));
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(ECONNREFUSED, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_unreachable_sockets());

  /* Close socket */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_unreachable_sockets());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_unreachable_sockets());

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

  /* Try connect, connection in progress. */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EINPROGRESS, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_unreachable_sockets());

  /* Disconnect pending connections. */

  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('F', 100));

  /* Poll for input (no timeout). */

  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = sd;
  pfd.events = POLLIN;
  ret = poll(&pfd, 1, -1);
  TEST_ASSERT_EQUAL(1, ret);
  TEST_ASSERT_EQUAL(POLLERR, pfd.revents & POLLERR);
  TEST_ASSERT_EQUAL(POLLHUP, pfd.revents & POLLHUP);
  TEST_ASSERT_EQUAL(0, pfd.revents & POLLIN);
  TEST_ASSERT_EQUAL(0, pfd.revents & POLLOUT);

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
 * Name: Send
 *
 * Description:
 *   Send and disconnect
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

static void Send(struct usrsocktest_daemon_conf_s *dconf)
{
  ssize_t ret;
  size_t datalen;
  void *data;
  struct sockaddr_in addr;
  int count;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  dconf->endpoint_block_connect = false;
  dconf->endpoint_block_send = false;
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
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_unreachable_sockets());

  /* Try connect, connection in progress. */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_unreachable_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_remote_disconnected_sockets());

  /* Disconnect connections. */

  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('D', 0));
  for (count = 0; usrsocktest_daemon_get_num_connected_sockets() > 0; count++)
    {
      TEST_ASSERT_TRUE(count <= 3);
      usleep(5 * 1000);
    }
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_remote_disconnected_sockets());

  for (count = 0; count < 2; count++)
    {
      /* Send data to remote */

      data = "abcde";
      datalen = strlen("abcde");
      ret = write(sd, data, datalen);
      TEST_ASSERT_EQUAL(-1, ret);
      TEST_ASSERT_EQUAL(EPIPE, errno);
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_remote_disconnected_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_send_bytes());
    }

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
 * Name: Send2
 *
 * Description:
 *   Send and disconnect
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

static void Send2(struct usrsocktest_daemon_conf_s *dconf)
{
  ssize_t ret;
  size_t datalen;
  void *data;
  struct sockaddr_in addr;
  int count;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  dconf->endpoint_block_connect = false;
  dconf->endpoint_block_send = true;
  dconf->endpoint_recv_avail_from_start = false;
  dconf->endpoint_recv_avail = 7;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_unreachable_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_remote_disconnected_sockets());

  /* Try connect. */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_unreachable_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_remote_disconnected_sockets());

  /* Disconnect connections with delay. */

  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('D', 100));

  for (count = 0; count < 2; count++)
    {
      /* Send data to remote */

      data = "abcde";
      datalen = strlen("abcde");
      ret = write(sd, data, datalen);
      TEST_ASSERT_EQUAL(-1, ret);
      TEST_ASSERT_EQUAL(EPIPE, errno);
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_remote_disconnected_sockets());
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_send_bytes());
    }

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
 * Name: Receive
 *
 * Description:
 *   Receive and disconnect
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

static void Receive(struct usrsocktest_daemon_conf_s *dconf)
{
  ssize_t ret;
  size_t datalen;
  void *data;
  struct sockaddr_in addr;
  char databuf[5];
  int count;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  dconf->endpoint_block_connect = false;
  dconf->endpoint_block_send = false;
  dconf->endpoint_recv_avail_from_start = true;
  dconf->endpoint_recv_avail = 1;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_unreachable_sockets());

  /* Try connect. */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_unreachable_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_remote_disconnected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  /* Disconnect connections. */

  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('D', 0));
  for (count = 0; usrsocktest_daemon_get_num_connected_sockets() > 0; count++)
    {
      TEST_ASSERT_TRUE(count <= 3);
      usleep(5 * 1000);
    }
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_remote_disconnected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_recv_empty_sockets());

  for (count = 0; count < 2; count++)
    {
      /* Read from closed remote (EOF) */

      data = databuf;
      datalen = sizeof(databuf);
      ret = read(sd, data, datalen);
      TEST_ASSERT_EQUAL(0, ret);
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_remote_disconnected_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_recv_bytes());
    }

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
 * Name: Receive2
 *
 * Description:
 *   Receive and disconnect
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

static void Receive2(struct usrsocktest_daemon_conf_s *dconf)
{
  ssize_t ret;
  size_t datalen;
  void *data;
  struct sockaddr_in addr;
  char databuf[5];
  int count;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  dconf->endpoint_block_connect = false;
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
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_unreachable_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_remote_disconnected_sockets());

  /* Try connect. */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_unreachable_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_remote_disconnected_sockets());

  /* Disconnect connections with delay. */

  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('D', 100));

  for (count = 0; count < 2; count++)
    {
      /* Read from closed remote (EOF) */

      data = databuf;
      datalen = sizeof(databuf);
      ret = read(sd, data, datalen);
      TEST_ASSERT_EQUAL(0, ret);
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_remote_disconnected_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_recv_bytes());
    }

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
 * Name: Poll
 *
 * Description:
 *   Poll and disconnect
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

static void Poll(struct usrsocktest_daemon_conf_s *dconf)
{
  ssize_t ret;
  struct sockaddr_in addr;
  int flags;
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

  /* Poll for input (instant timeout). */

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

  /* Poll for input/output (no timeout). Fail connection. */

  memset(&pfd, 0, sizeof(pfd));
  TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('F', 100));
  pfd.fd = sd;
  pfd.events = POLLOUT | POLLIN;
  ret = poll(&pfd, 1, -1);
  TEST_ASSERT_EQUAL(1, ret);
  TEST_ASSERT_EQUAL(POLLERR, pfd.revents & POLLERR);
  TEST_ASSERT_EQUAL(POLLHUP, pfd.revents & POLLHUP);
  TEST_ASSERT_EQUAL(0, pfd.revents & POLLIN);
  TEST_ASSERT_EQUAL(0, pfd.revents & POLLOUT);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_remote_disconnected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_unreachable_sockets());

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
 * Name: Poll2
 *
 * Description:
 *   Poll and disconnect
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

static void Poll2(struct usrsocktest_daemon_conf_s *dconf)
{
  ssize_t ret;
  size_t datalen;
  void *data;
  struct sockaddr_in addr;
  char databuf[5];
  int flags;
  struct pollfd pfd;
  int count;
  const uint8_t *events = tevents;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  dconf->endpoint_recv_avail_from_start = false;
  dconf->endpoint_recv_avail = 3;
  dconf->endpoint_block_send = true;
  dconf->endpoint_block_connect = false;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  do
    {
      TEST_ASSERT_TRUE(*events == POLLIN || *events == POLLOUT || *events == (POLLOUT|POLLIN));

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

      /* Poll for input (instant timeout). */

      memset(&pfd, 0, sizeof(pfd));
      pfd.fd = sd;
      pfd.events = *events;
      ret = poll(&pfd, 1, 0);
      TEST_ASSERT_EQUAL(1, ret);
      TEST_ASSERT_EQUAL(0, pfd.revents & POLLERR);
      TEST_ASSERT_EQUAL(POLLHUP, pfd.revents & POLLHUP);
      TEST_ASSERT_EQUAL(*events & POLLIN, pfd.revents & POLLIN);
      TEST_ASSERT_EQUAL(0, pfd.revents & POLLOUT);

      /* Start non-blocking connect. */

      inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
      addr.sin_family = AF_INET;
      addr.sin_port = htons(255);
      ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
      if (!dconf->delay_all_responses)
        {
          TEST_ASSERT_EQUAL(0, ret);
          TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
          TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
          TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
          TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_recv_empty_sockets());
        }
      else
        {
          TEST_ASSERT_EQUAL(-1, ret);
          TEST_ASSERT_EQUAL(EINPROGRESS, errno);
          TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
          TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

          for (count = 0; usrsocktest_daemon_get_num_connected_sockets() != 1; count++)
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

      /* Poll for output (no timeout). Close connection. */

      memset(&pfd, 0, sizeof(pfd));
      TEST_ASSERT_TRUE(usrsocktest_send_delayed_command('D', 100));
      pfd.fd = sd;
      pfd.events = *events;
      ret = poll(&pfd, 1, -1);
      TEST_ASSERT_EQUAL(1, ret);
      TEST_ASSERT_EQUAL(0, pfd.revents & POLLERR);
      TEST_ASSERT_EQUAL(POLLHUP, pfd.revents & POLLHUP);
      TEST_ASSERT_EQUAL(*events & POLLIN, pfd.revents & POLLIN);
      TEST_ASSERT_EQUAL(0, pfd.revents & POLLOUT);
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
      TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_remote_disconnected_sockets());

      for (count = 0; count < 2; count++)
        {
          /* Read from closed remote (EOF) */

          data = databuf;
          datalen = sizeof(databuf);
          ret = read(sd, data, datalen);
          TEST_ASSERT_EQUAL(0, ret);
          TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
          TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_remote_disconnected_sockets());
          TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
          TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_recv_bytes());
        }

      for (count = 0; count < 2; count++)
        {
          /* Send data to remote */

          data = "abcde";
          datalen = strlen("abcde");
          ret = write(sd, data, datalen);
          TEST_ASSERT_EQUAL(-1, ret);
          TEST_ASSERT_EQUAL(EPIPE, errno);
          TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
          TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
          TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_remote_disconnected_sockets());
          TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_send_bytes());
        }

      /* Close socket */

      TEST_ASSERT_TRUE(close(sd) >= 0);
      sd = -1;
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

      events++;
    }
  while (*events);

  /* Stopping daemon should succeed. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Name: RemoteDisconnect test group setup
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

TEST_SETUP(RemoteDisconnect)
{
  sd = -1;
  started = false;
}

/****************************************************************************
 * Name: RemoteDisconnect test group teardown
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

TEST_TEAR_DOWN(RemoteDisconnect)
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

TEST(RemoteDisconnect, Unreachable)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  Unreachable(&usrsocktest_daemon_config);
}

TEST(RemoteDisconnect, UnreachableDelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  Unreachable(&usrsocktest_daemon_config);
}

TEST(RemoteDisconnect, Send)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  Send(&usrsocktest_daemon_config);
}

TEST(RemoteDisconnect, SendDelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  Send(&usrsocktest_daemon_config);
}

TEST(RemoteDisconnect, Send2)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  Send2(&usrsocktest_daemon_config);
}

TEST(RemoteDisconnect, Send2Delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  Send2(&usrsocktest_daemon_config);
}

TEST(RemoteDisconnect, Receive)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  Receive(&usrsocktest_daemon_config);
}

TEST(RemoteDisconnect, ReceiveDelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  Receive(&usrsocktest_daemon_config);
}

TEST(RemoteDisconnect, Receive2)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  Receive2(&usrsocktest_daemon_config);
}

TEST(RemoteDisconnect, Receive2Delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  Receive2(&usrsocktest_daemon_config);
}

TEST(RemoteDisconnect, Poll)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  Poll(&usrsocktest_daemon_config);
}

TEST(RemoteDisconnect, PollDelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  Poll(&usrsocktest_daemon_config);
}

TEST(RemoteDisconnect, Poll2)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  Poll2(&usrsocktest_daemon_config);
}

TEST(RemoteDisconnect, Poll2Delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  Poll2(&usrsocktest_daemon_config);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(RemoteDisconnect)
{
  RUN_TEST_CASE(RemoteDisconnect, Unreachable);
  RUN_TEST_CASE(RemoteDisconnect, UnreachableDelay);
  RUN_TEST_CASE(RemoteDisconnect, Send);
  RUN_TEST_CASE(RemoteDisconnect, SendDelay);
  RUN_TEST_CASE(RemoteDisconnect, Send2);
  RUN_TEST_CASE(RemoteDisconnect, Send2Delay);
  RUN_TEST_CASE(RemoteDisconnect, Receive);
  RUN_TEST_CASE(RemoteDisconnect, ReceiveDelay);
  RUN_TEST_CASE(RemoteDisconnect, Receive2);
  RUN_TEST_CASE(RemoteDisconnect, Receive2Delay);
  RUN_TEST_CASE(RemoteDisconnect, Poll);
  RUN_TEST_CASE(RemoteDisconnect, PollDelay);
  RUN_TEST_CASE(RemoteDisconnect, Poll2);
  RUN_TEST_CASE(RemoteDisconnect, Poll2Delay);
}

