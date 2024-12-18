/****************************************************************************
 * apps/testing/testsuites/kernel/socket/cases/net_socket_test_011.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 *The ASF licenses this file to you under the Apache License, Version 2.0
 *(the "License"); you may not use this file except in compliance with
 *the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 *implied.  See the License for the specific language governing
 *permissions and limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "SocketTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SERVER_PORT 7777
#define INVALID_SOCKET -1

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static int gfds[FD_SETSIZE];
static int gbye;

static void initfds(void)
{
  for (int i = 0; i < FD_SETSIZE; ++i)
    {
      gfds[i] = INVALID_SOCKET;
    }
}

static void getreadfds(fd_set *fds, int *nfd)
{
  for (int i = 0; i < FD_SETSIZE; i++)
    {
      if (gfds[i] == INVALID_SOCKET)
        {
          continue;
        }

      FD_SET(gfds[i], fds);
      if (*nfd < gfds[i])
        {
          *nfd = gfds[i];
        }
    }
}

static int addfd(int fd)
{
  for (int i = 0; i < FD_SETSIZE; ++i)
    {
      if (gfds[i] == INVALID_SOCKET)
        {
          gfds[i] = fd;
          return 0;
        }
    }

  return -1;
}

static void delfd(int fd)
{
  for (int i = 0; i < FD_SETSIZE; ++i)
    {
      if (gfds[i] == fd)
        {
          gfds[i] = INVALID_SOCKET;
        }
    }

  (void)close(fd);
}

static int closeallfd(void)
{
  for (int i = 0; i < FD_SETSIZE; ++i)
    {
      if (gfds[i] != INVALID_SOCKET)
        {
          (void)close(gfds[i]);
          gfds[i] = INVALID_SOCKET;
        }
    }

  return 0;
}

static int handlerecv(int fd)
{
  char buf[128];
  int ret = recv(fd, buf, sizeof(buf) - 1, 0);
  if (ret < 0)
    {
      syslog(LOG_INFO, "[%d]Error: %s", fd, strerror(errno));
      delfd(fd);
    }

  else if (ret == 0)
    {
      syslog(LOG_INFO, "[%d]Closed", fd);
      delfd(fd);
    }

  else
    {
      buf[ret] = 0;
      syslog(LOG_INFO, "[%d]Received: %s", fd, buf);
      if (strstr(buf, "Bye") != NULL)
        {
          gbye++;
        }
    }

  return -(ret < 0);
}

static int handlereadfds(fd_set *fds, int lsfd)
{
  int ret = 0;
  for (int i = 0; i < FD_SETSIZE; ++i)
    {
      if (gfds[i] == INVALID_SOCKET || !FD_ISSET(gfds[i], fds))
        {
          continue;
        }

      ret += handlerecv(gfds[i]);
    }

  return ret;
}

static void *clientsthread(void *param)
{
  int fd;
  int thrno = (int)(intptr_t)param;

  syslog(LOG_INFO, "<%d>socket client thread started", thrno);
  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd == INVALID_SOCKET)
    {
      perror("socket");
      return NULL;
    }

  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  sa.sin_port = htons(SERVER_PORT);
  if (connect(fd, (struct sockaddr *)(&sa), sizeof(sa)) == -1)
    {
      perror("connect");
      return NULL;
    }

  syslog(LOG_INFO, "[%d]<%d>connected to udp://%s:%d successful", fd,
         thrno, inet_ntoa(sa.sin_addr), SERVER_PORT);

  const char *msg[] =
  {
      "ohos, ",
      "hello, ",
      "my name is net_socket_test_011, ",
      "see u next time, ",
      "Bye!",
  };

  for (int i = 0; i < sizeof(msg) / sizeof(msg[0]); ++i)
    {
      if (send(fd, msg[i], strlen(msg[i]), 0) < 0)
        {
          syslog(LOG_INFO, "[%d]<%d>send msg [%s] fail", fd, thrno,
                 msg[i]);
        }
    }

  (void)shutdown(fd, SHUT_RDWR);
  (void)close(fd);
  return param;
}

static int startclients(pthread_t *cli, int clinum)
{
  int ret;
  pthread_attr_t attr =
    {
      0
    };

  struct sched_param param =
  {
    0
  };

  int policy;
  ret = pthread_getschedparam(pthread_self(), &policy, &param);
  assert_int_equal(ret, 0);

  for (int i = 0; i < clinum; ++i)
    {
      ret = pthread_attr_init(&attr);
      param.sched_priority = param.sched_priority + 1;
      pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
      pthread_attr_setschedparam(&attr, &param);

      ret = pthread_create(&cli[i], &attr, clientsthread,
                           (void *)(intptr_t)i);
      assert_int_equal(ret, 0);
      ret = pthread_attr_destroy(&attr);
      assert_int_equal(ret, 0);
    }

  return 0;
}

/****************************************************************************
 * Name: test_nuttx_net_socket_test11
 ****************************************************************************/

void test_nuttx_net_socket_test11(FAR void **state)
{
  struct sockaddr_in sa =
  {
    0
  };

  int lsfd;
  int ret;

  lsfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  assert_int_not_equal(lsfd, -1);

  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons(SERVER_PORT);
  ret = bind(lsfd, (struct sockaddr *)(&sa), sizeof(sa));
  assert_int_not_equal(ret, -1);

  initfds();
  addfd(lsfd);
  syslog(LOG_INFO, "[%d]Waiting for client to connect on UDP port %d",
         lsfd, SERVER_PORT);

  pthread_t clients[CLIENT_NUM];

  ret = startclients(clients, CLIENT_NUM);
  assert_int_equal(ret, 0);

  for (; ; )
    {
      int nfd;
      fd_set readfds;
      struct timeval timeout;

      timeout.tv_sec = 3;
      timeout.tv_usec = 0;

      nfd = 0;
      FD_ZERO(&readfds);

      getreadfds(&readfds, &nfd);

      ret = select(nfd + 1, &readfds, NULL, NULL, &timeout);
      syslog(LOG_INFO, "select %d", ret);
      if (ret == -1)
        {
          perror("select");
          break; /* error occurred */
        }

      else if (ret == 0)
        {
          break; /* timed out */
        }

      if (handlereadfds(&readfds, lsfd) < 0)
        {
          break;
        }
    }

  for (int i = 0; i < CLIENT_NUM; ++i)
    {
      ret = pthread_join(clients[i], NULL);
      assert_int_equal(ret, 0);
    }

#ifdef CONFIG_NET_UDP_NOTIFIER
  assert_int_equal(gbye, CLIENT_NUM);
#endif
  (void)closeallfd();
}
