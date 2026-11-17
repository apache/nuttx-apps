/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_process.c
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

/* Data that goes over TCP on the loopback address from one process to
 * another, with write() and writev() at the sender and read() and readv()
 * at the receiver: the receiver must get every byte.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <cmocka.h>

#include "test_tcp.h"
#include "utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_DATA_SIZE  (1024 * 1024)
#define TEST_CHUNK_SIZE 4096
#define TEST_READ_DELAY 100000 /* us: the receive buffer fills first */
#define TEST_TIMEOUT    30     /* s: a transfer that stops fails */

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Only the child uses g_rxbuf, only the parent g_txbuf.  Not on the stack
 * or the heap: a wrong copy to the address of g_rxbuf in the parent must
 * not break the parent.
 */

static unsigned char g_rxbuf[TEST_CHUNK_SIZE];
static unsigned char g_txbuf[TEST_CHUNK_SIZE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_process_pattern
 ****************************************************************************/

static unsigned char tcp_process_pattern(size_t i)
{
  return (i * 7 + (i >> 12)) % 251;
}

/****************************************************************************
 * Name: tcp_process_alarm
 ****************************************************************************/

static void tcp_process_alarm(int signo)
{
  UNUSED(signo);
}

/****************************************************************************
 * Name: tcp_process_timeout
 ****************************************************************************/

static void tcp_process_timeout(int fd, int option)
{
  struct timeval tv;

  tv.tv_sec  = TEST_TIMEOUT;
  tv.tv_usec = 0;
  setsockopt(fd, SOL_SOCKET, option, &tv, sizeof(tv));
}

/****************************************************************************
 * Name: tcp_process_receive
 *
 * Description:
 *   The child: connect, read everything with read() and readv() in turn
 *   and compare it with the pattern.  Return the exit status: 0 if the
 *   data is complete and correct.
 *
 ****************************************************************************/

static int tcp_process_receive(FAR struct sockaddr_in *addr,
                               socklen_t addrlen)
{
  struct iovec iov[2];
  size_t got = 0;
  ssize_t nread;
  ssize_t i;
  int calls = 0;
  int bad = 0;
  int fd;

  fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0 || connect(fd, (FAR struct sockaddr *)addr, addrlen) < 0)
    {
      return 2;
    }

  tcp_process_timeout(fd, SO_RCVTIMEO);
  usleep(TEST_READ_DELAY);
  for (; ; )
    {
      if (calls++ % 2 == 0)
        {
          nread = read(fd, g_rxbuf, sizeof(g_rxbuf));
        }
      else
        {
          iov[0].iov_base = g_rxbuf;
          iov[0].iov_len  = sizeof(g_rxbuf) / 2;
          iov[1].iov_base = g_rxbuf + sizeof(g_rxbuf) / 2;
          iov[1].iov_len  = sizeof(g_rxbuf) / 2;
          nread = readv(fd, iov, 2);
        }

      if (nread <= 0)
        {
          break;
        }

      /* Read on after a difference, so that the sender can finish */

      for (i = 0; i < nread; i++)
        {
          if (got + i >= TEST_DATA_SIZE ||
              g_rxbuf[i] != tcp_process_pattern(got + i))
            {
              bad = 1;
            }
        }

      got += nread;
    }

  close(fd);
  return nread == 0 && got == TEST_DATA_SIZE && bad == 0 ? 0 : 3;
}

/****************************************************************************
 * Name: tcp_process_send
 *
 * Description:
 *   The parent: send the pattern with write() and writev() in turn.
 *   Return true if all data was sent.
 *
 ****************************************************************************/

static bool tcp_process_send(int fd)
{
  struct iovec iov[2];
  size_t sent = 0;
  int calls = 0;

  while (sent < TEST_DATA_SIZE)
    {
      size_t len = sizeof(g_txbuf);
      size_t done = 0;
      size_t i;

      for (i = 0; i < len; i++)
        {
          g_txbuf[i] = tcp_process_pattern(sent + i);
        }

      while (done < len)
        {
          ssize_t nwritten;

          if (calls++ % 2 == 0 || done > 0)
            {
              nwritten = write(fd, g_txbuf + done, len - done);
            }
          else
            {
              iov[0].iov_base = g_txbuf;
              iov[0].iov_len  = sizeof(g_txbuf) / 2;
              iov[1].iov_base = g_txbuf + sizeof(g_txbuf) / 2;
              iov[1].iov_len  = sizeof(g_txbuf) / 2;
              nwritten = writev(fd, iov, 2);
            }

          if (nwritten <= 0)
            {
              return false;
            }

          done += nwritten;
        }

      sent += len;
    }

  return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_process_data
 ****************************************************************************/

void test_tcp_process_data(FAR void **state)
{
  struct sockaddr_in addr;
  socklen_t addrlen = nettest_lo_addr((FAR struct sockaddr *)&addr,
                                      AF_INET);
  struct sigaction act;
  struct sigaction oldalrm;
  struct sigaction oldpipe;
  bool sent = false;
  int status = -1;
  pid_t pid;
  int lfd;
  int fd;

  UNUSED(state);

  /* A port that bind() selects: the group server has the test port */

  addr.sin_port = 0;
  lfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  assert_true(lfd >= 0);
  assert_return_code(bind(lfd, (FAR struct sockaddr *)&addr, addrlen),
                     errno);
  assert_return_code(listen(lfd, 1), errno);
  assert_return_code(getsockname(lfd, (FAR struct sockaddr *)&addr,
                                 &addrlen), errno);

  pid = fork();
  if (pid == 0)
    {
      close(lfd);
      _exit(tcp_process_receive(&addr, addrlen));
    }

  assert_true(pid > 0);

  /* SIGALRM ends an accept() that waits for a child that cannot connect,
   * and a write() to a child that ended gives an error, not SIGPIPE.
   */

  memset(&act, 0, sizeof(act));
  act.sa_handler = tcp_process_alarm;
  sigaction(SIGALRM, &act, &oldalrm);
  act.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &act, &oldpipe);

  alarm(TEST_TIMEOUT);
  fd = accept4(lfd, NULL, NULL, SOCK_CLOEXEC);
  alarm(0);
  close(lfd);
  if (fd >= 0)
    {
      tcp_process_timeout(fd, SO_SNDTIMEO);
      sent = tcp_process_send(fd);
      close(fd);
    }

  sigaction(SIGPIPE, &oldpipe, NULL);
  sigaction(SIGALRM, &oldalrm, NULL);

  if (!sent)
    {
      kill(pid, SIGKILL);
    }

  assert_int_equal(waitpid(pid, &status, 0), pid);
  assert_true(sent);
  assert_true(WIFEXITED(status));
  assert_int_equal(WEXITSTATUS(status), 0);
}
