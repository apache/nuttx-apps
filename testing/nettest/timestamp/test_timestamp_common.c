/****************************************************************************
 * apps/testing/nettest/timestamp/test_timestamp_common.c
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

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <poll.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <cmocka.h>

#include "test_timestamp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TX_PKT_SIZE    46
#define RX_RETRY_MAX   10
#define RX_RETRY_USEC  50000
#define POLL_TIMEOUT   1000

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_timestamp_group_setup
 ****************************************************************************/

int test_timestamp_group_setup(FAR void **state)
{
  FAR struct nettest_timestamp_state_s *ts_state;

  ts_state = zalloc(sizeof(*ts_state));
  assert_non_null(ts_state);

  *state = ts_state;

  ts_state->ifindex = if_nametoindex("eth0");

  return 0;
}

/****************************************************************************
 * Name: test_timestamp_group_teardown
 ****************************************************************************/

int test_timestamp_group_teardown(FAR void **state)
{
  FAR struct nettest_timestamp_state_s *ts_state = *state;

  free(ts_state);

  return 0;
}

/****************************************************************************
 * Name: test_timestamp_setsockopt
 *
 * Description:
 *   Test 1: Verify SO_TIMESTAMPING setsockopt/getsockopt.
 *
 ****************************************************************************/

void test_timestamp_setsockopt(FAR void **state)
{
  socklen_t len;
  int val;
  int ret;
  int fd;

  fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  assert_return_code(fd, errno);

  val = SOF_TIMESTAMPING_TX_SOFTWARE;
  ret = setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING,
                   &val, sizeof(val));
  assert_return_code(ret, errno);

  val = 0;
  len = sizeof(val);
  ret = getsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING,
                   &val, &len);
  assert_return_code(ret, errno);

  assert_true(val != 0);

  close(fd);
}

/****************************************************************************
 * Name: test_timestamp_compat
 *
 * Description:
 *   Test 2: SO_TIMESTAMP/SO_TIMESTAMPNS backward compatibility.
 *
 ****************************************************************************/

void test_timestamp_compat(FAR void **state)
{
  int val;
  int ret;
  int fd;

  fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  assert_return_code(fd, errno);

  val = 1;
  ret = setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP,
                   &val, sizeof(val));
  assert_return_code(ret, errno);

  val = 1;
  ret = setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPNS,
                   &val, sizeof(val));
  assert_return_code(ret, errno);

  close(fd);
}

/****************************************************************************
 * Name: test_timestamp_tx
 *
 * Description:
 *   Test 3: TX timestamping + MSG_ERRQUEUE delivery.
 *   Matches PR #20161 test_tx_timestamp() exactly.
 *
 ****************************************************************************/

void test_timestamp_tx(FAR void **state)
{
  FAR struct nettest_timestamp_state_s *ts_state = *state;
  FAR struct cmsghdr *cmsg;
  FAR struct timespec *ts;
  struct sockaddr_ll addr;
  char cmsgbuf[256];
  char buf[64];
  struct msghdr msg;
  struct iovec iov;
  ssize_t n;
  int val;
  int ret;
  int fd;

  if (ts_state->ifindex == 0)
    {
      skip();
    }

  fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
  assert_return_code(fd, errno);

  val = SOF_TIMESTAMPING_TX_SOFTWARE;
  ret = setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING,
                   &val, sizeof(val));
  assert_return_code(ret, errno);

  memset(&addr, 0, sizeof(addr));
  addr.sll_family   = AF_PACKET;
  addr.sll_protocol = htons(ETH_P_ALL);
  addr.sll_ifindex  = ts_state->ifindex;

  ret = bind(fd, (FAR struct sockaddr *)&addr, sizeof(addr));
  assert_return_code(ret, errno);

  memset(buf, 0xaa, TX_PKT_SIZE);
  addr.sll_halen = 6;
  memset(addr.sll_addr, 0xff, 6);

  n = sendto(fd, buf, TX_PKT_SIZE, 0,
             (FAR struct sockaddr *)&addr, sizeof(addr));
  assert_true(n > 0);

  usleep(10000);

  memset(&msg, 0, sizeof(msg));
  iov.iov_base       = buf;
  iov.iov_len        = sizeof(buf);
  msg.msg_iov        = &iov;
  msg.msg_iovlen     = 1;
  msg.msg_control    = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);

  n = recvmsg(fd, &msg, MSG_ERRQUEUE | MSG_DONTWAIT);
  assert_true(n >= 0);

  ts = NULL;
  for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
       cmsg = CMSG_NXTHDR(&msg, cmsg))
    {
      if (cmsg->cmsg_level == SOL_SOCKET &&
          cmsg->cmsg_type == SO_TIMESTAMPING)
        {
          ts = (FAR struct timespec *)CMSG_DATA(cmsg);
          break;
        }
    }

  assert_non_null(ts);
  assert_true(ts[0].tv_sec > 0);

  close(fd);
}

/****************************************************************************
 * Name: test_timestamp_rx
 *
 * Description:
 *   Test 4: RX timestamp via SO_TIMESTAMP.
 *   Matches PR #20161 test_rx_timestamp() exactly.
 *
 ****************************************************************************/

void test_timestamp_rx(FAR void **state)
{
  FAR struct nettest_timestamp_state_s *ts_state = *state;
  FAR struct cmsghdr *cmsg;
  FAR struct timeval *tv;
  struct sockaddr_ll addr;
  char cmsgbuf[256];
  char buf[128];
  struct msghdr msg;
  struct iovec iov;
  ssize_t n;
  int retry;
  int val;
  int ret;
  int fd;

  if (ts_state->ifindex == 0)
    {
      skip();
    }

  fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  assert_return_code(fd, errno);

  val = 1;
  ret = setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP,
                   &val, sizeof(val));
  assert_return_code(ret, errno);

  memset(&addr, 0, sizeof(addr));
  addr.sll_family   = AF_PACKET;
  addr.sll_protocol = htons(ETH_P_ALL);
  addr.sll_ifindex  = ts_state->ifindex;

  ret = bind(fd, (FAR struct sockaddr *)&addr, sizeof(addr));
  assert_return_code(ret, errno);

  /* Send broadcast via separate SOCK_DGRAM to trigger RX */

  {
    struct sockaddr_ll sa;
    int sfd;

    sfd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
    assert_return_code(sfd, errno);

    memset(&sa, 0, sizeof(sa));
    sa.sll_family   = AF_PACKET;
    sa.sll_protocol = htons(ETH_P_IP);
    sa.sll_ifindex  = ts_state->ifindex;
    sa.sll_halen    = 6;
    memset(sa.sll_addr, 0xff, 6);
    memset(buf, 0xcc, TX_PKT_SIZE);

    sendto(sfd, buf, TX_PKT_SIZE, 0,
           (FAR struct sockaddr *)&sa, sizeof(sa));
    close(sfd);
  }

  memset(&msg, 0, sizeof(msg));
  iov.iov_base       = buf;
  iov.iov_len        = sizeof(buf);
  msg.msg_iov        = &iov;
  msg.msg_iovlen     = 1;
  msg.msg_control    = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);

  n = -1;
  for (retry = 0; retry < RX_RETRY_MAX; retry++)
    {
      usleep(RX_RETRY_USEC);
      n = recvmsg(fd, &msg, MSG_DONTWAIT);
      if (n > 0)
        {
          break;
        }
    }

  assert_true(n > 0);

  tv = NULL;
  for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
       cmsg = CMSG_NXTHDR(&msg, cmsg))
    {
      if (cmsg->cmsg_level == SOL_SOCKET &&
          cmsg->cmsg_type == SO_TIMESTAMP)
        {
          tv = (FAR struct timeval *)CMSG_DATA(cmsg);
          break;
        }
    }

  assert_non_null(tv);
  assert_true(tv->tv_sec > 0);

  close(fd);
}

/****************************************************************************
 * Name: test_timestamp_poll
 *
 * Description:
 *   Test 5: TX timestamping with poll(POLLPRI) notification.
 *   Matches PR #20161 test_tx_timestamp_poll() exactly.
 *
 ****************************************************************************/

void test_timestamp_poll(FAR void **state)
{
  FAR struct nettest_timestamp_state_s *ts_state = *state;
  FAR struct cmsghdr *cmsg;
  FAR struct timespec *ts;
  struct sockaddr_ll addr;
  struct pollfd pfd;
  char cmsgbuf[256];
  char buf[64];
  struct msghdr msg;
  struct iovec iov;
  ssize_t n;
  int val;
  int ret;
  int fd;

  if (ts_state->ifindex == 0)
    {
      skip();
    }

  fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
  assert_return_code(fd, errno);

  val = SOF_TIMESTAMPING_TX_SOFTWARE;
  ret = setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING,
                   &val, sizeof(val));
  assert_return_code(ret, errno);

  memset(&addr, 0, sizeof(addr));
  addr.sll_family   = AF_PACKET;
  addr.sll_protocol = htons(ETH_P_ALL);
  addr.sll_ifindex  = ts_state->ifindex;

  ret = bind(fd, (FAR struct sockaddr *)&addr, sizeof(addr));
  assert_return_code(ret, errno);

  addr.sll_halen = 6;
  memset(addr.sll_addr, 0xff, 6);

  memset(buf, 0xaa, TX_PKT_SIZE);
  n = sendto(fd, buf, TX_PKT_SIZE, 0,
             (FAR struct sockaddr *)&addr, sizeof(addr));
  assert_true(n > 0);

  pfd.fd      = fd;
  pfd.events  = POLLPRI;
  pfd.revents = 0;

  n = poll(&pfd, 1, POLL_TIMEOUT);
  assert_true(n > 0);
  assert_true((pfd.revents & POLLPRI) != 0);

  memset(&msg, 0, sizeof(msg));
  iov.iov_base       = buf;
  iov.iov_len        = sizeof(buf);
  msg.msg_iov        = &iov;
  msg.msg_iovlen     = 1;
  msg.msg_control    = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);

  n = recvmsg(fd, &msg, MSG_ERRQUEUE);
  assert_true(n >= 0);

  ts = NULL;
  for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
       cmsg = CMSG_NXTHDR(&msg, cmsg))
    {
      if (cmsg->cmsg_level == SOL_SOCKET &&
          cmsg->cmsg_type == SO_TIMESTAMPING)
        {
          ts = (FAR struct timespec *)CMSG_DATA(cmsg);
          break;
        }
    }

  assert_non_null(ts);
  assert_true(ts[0].tv_sec > 0);

  close(fd);
}
