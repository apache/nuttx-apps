/****************************************************************************
 * examples/usrsocktest/usrsocktest_wake_with_signal.c
 * Wake blocked IO with signal or daemon abort
 *
 *   Copyright (C) 2015, 2017 Haltian Ltd. All rights reserved.
 *   Authors: Jussi Kivilinna <jussi.kivilinna@haltian.com>
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
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>

#include "defines.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define TEST_FLAG_PAUSE_USRSOCK_HANDLING (1 << 0)
#define TEST_FLAG_DAEMON_ABORT           (1 << 1)
#define TEST_FLAG_MULTI_THREAD           (1 << 2)

#define MAX_THREADS                      4

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum e_test_type
{
  TEST_TYPE_SOCKET = 0,
  TEST_TYPE_CLOSE,
  TEST_TYPE_CONNECT,
  TEST_TYPE_SETSOCKOPT,
  TEST_TYPE_GETSOCKOPT,
  TEST_TYPE_SEND,
  TEST_TYPE_RECV,
  TEST_TYPE_POLL,
  __TEST_TYPE_MAX,
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
static pthread_t tid[MAX_THREADS];
static sem_t tid_startsem;
static sem_t tid_releasesem;
static int test_sd[MAX_THREADS];
static enum e_test_type test_type;
static int test_flags;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void do_usrsock_blocking_socket_thread(FAR void *param)
{
  intptr_t tidx = (intptr_t)param;
  bool test_abort = !!(test_flags & TEST_FLAG_DAEMON_ABORT);
  bool test_hang = !!(test_flags & TEST_FLAG_PAUSE_USRSOCK_HANDLING);

  TEST_ASSERT_TRUE(test_hang);
  TEST_ASSERT_TRUE(test_abort);

  /* Allow main thread to hang usrsock daemon at this point. */

  sem_post(&tid_startsem);
  sem_wait(&tid_releasesem);

  /* Attempt hanging open socket. */

  sem_post(&tid_startsem);
  test_sd[tidx] = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_EQUAL(-1, test_sd[tidx]);
  TEST_ASSERT_EQUAL(ENETDOWN, errno);
}

static FAR void * usrsock_blocking_socket_thread(FAR void *param)
{
  do_usrsock_blocking_socket_thread(param);
  return NULL;
}

static void do_usrsock_blocking_close_thread(FAR void *param)
{
  intptr_t tidx = (intptr_t)param;
  struct sockaddr_in addr;
  int ret;
  bool test_abort = !!(test_flags & TEST_FLAG_DAEMON_ABORT);
  bool test_hang = !!(test_flags & TEST_FLAG_PAUSE_USRSOCK_HANDLING);

  TEST_ASSERT_TRUE(test_hang);
  TEST_ASSERT_TRUE(test_abort);

  /* Open socket. */

  test_sd[tidx] = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(test_sd[tidx] >= 0);

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);

  /* Allow main thread to hang usrsock daemon at this point. */

  sem_post(&tid_startsem);
  sem_wait(&tid_releasesem);

  /* Attempt hanging close socket. */

  sem_post(&tid_startsem);
  ret = close(test_sd[tidx]);
  TEST_ASSERT_EQUAL(0, ret);
  test_sd[tidx] = -1;
}

static FAR void * usrsock_blocking_close_thread(FAR void *param)
{
  do_usrsock_blocking_close_thread(param);
  return NULL;
}

static void do_usrsock_blocking_connect_thread(FAR void *param)
{
  intptr_t tidx = (intptr_t)param;
  struct sockaddr_in addr;
  int ret;
  bool test_abort = !!(test_flags & TEST_FLAG_DAEMON_ABORT);
  bool test_hang = !!(test_flags & TEST_FLAG_PAUSE_USRSOCK_HANDLING);

  TEST_ASSERT_TRUE(test_hang || !test_hang);

  /* Open socket. */

  test_sd[tidx] = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(test_sd[tidx] >= 0);

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);

  /* Allow main thread to hang usrsock daemon at this point. */

  sem_post(&tid_startsem);
  sem_wait(&tid_releasesem);

  /* Attempt blocking connect. */

  sem_post(&tid_startsem);
  ret = connect(test_sd[tidx], (FAR const struct sockaddr *)&addr,
                sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(test_abort ? ECONNABORTED : EINTR, errno);

  /* Close socket */

  TEST_ASSERT_TRUE(close(test_sd[tidx]) >= 0);
  test_sd[tidx] = -1;
}

static FAR void * usrsock_blocking_connect_thread(FAR void *param)
{
  do_usrsock_blocking_connect_thread(param);
  return NULL;
}

static void do_usrsock_blocking_setsockopt_thread(FAR void *param)
{
  intptr_t tidx = (intptr_t)param;
  struct sockaddr_in addr;
  int ret;
  int value;
  bool test_abort = !!(test_flags & TEST_FLAG_DAEMON_ABORT);
  bool test_hang = !!(test_flags & TEST_FLAG_PAUSE_USRSOCK_HANDLING);

  /* Open socket. */

  test_sd[tidx] = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(test_sd[tidx] >= 0);

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);

  TEST_ASSERT_TRUE(test_hang);
  TEST_ASSERT_TRUE(test_abort);

  /* Allow main thread to hang usrsock daemon at this point. */

  sem_post(&tid_startsem);
  sem_wait(&tid_releasesem);

  /* Attempt hanging setsockopt. */

  sem_post(&tid_startsem);
  value = 1;
  ret = setsockopt(test_sd[tidx], SOL_SOCKET, SO_REUSEADDR, &value,
                   sizeof(value));
  TEST_ASSERT_EQUAL(-1, ret);

  /* Close socket */

  TEST_ASSERT_TRUE(close(test_sd[tidx]) >= 0);
  test_sd[tidx] = -1;
}

static FAR void * usrsock_blocking_setsockopt_thread(FAR void *param)
{
  do_usrsock_blocking_setsockopt_thread(param);
  return NULL;
}

static void do_usrsock_blocking_getsockopt_thread(FAR void *param)
{
  intptr_t tidx = (intptr_t)param;
  struct sockaddr_in addr;
  int ret;
  int value;
  socklen_t valuelen;
  bool test_abort = !!(test_flags & TEST_FLAG_DAEMON_ABORT);
  bool test_hang = !!(test_flags & TEST_FLAG_PAUSE_USRSOCK_HANDLING);

  /* Open socket. */

  test_sd[tidx] = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(test_sd[tidx] >= 0);

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);

  TEST_ASSERT_TRUE(test_hang);
  TEST_ASSERT_TRUE(test_abort);

  /* Allow main thread to hang usrsock daemon at this point. */

  sem_post(&tid_startsem);
  sem_wait(&tid_releasesem);

  /* Attempt hanging getsockopt. */

  sem_post(&tid_startsem);
  value = -1;
  valuelen = sizeof(value);
  ret = getsockopt(test_sd[tidx], SOL_SOCKET, SO_REUSEADDR, &value, &valuelen);
  TEST_ASSERT_EQUAL(-1, ret);

  /* Close socket */

  TEST_ASSERT_TRUE(close(test_sd[tidx]) >= 0);
  test_sd[tidx] = -1;
}

static FAR void * usrsock_blocking_getsockopt_thread(FAR void *param)
{
  do_usrsock_blocking_getsockopt_thread(param);
  return NULL;
}

static void do_usrsock_blocking_send_thread(FAR void *param)
{
  intptr_t tidx = (intptr_t)param;
  struct sockaddr_in addr;
  int ret;
  bool test_abort = !!(test_flags & TEST_FLAG_DAEMON_ABORT);
  bool test_hang = !!(test_flags & TEST_FLAG_PAUSE_USRSOCK_HANDLING);

  TEST_ASSERT_TRUE(test_hang || !test_hang);

  /* Open socket. */

  test_sd[tidx] = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(test_sd[tidx] >= 0);

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);

  /* Connect socket. */

  ret = connect(test_sd[tidx], (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(0, ret);

  /* Allow main thread to hang usrsock daemon at this point. */

  sem_post(&tid_startsem);
  sem_wait(&tid_releasesem);

  /* Attempt blocking send. */

  sem_post(&tid_startsem);
  ret = send(test_sd[tidx], &addr, sizeof(addr), 0);
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(test_abort ? EPIPE : EINTR, errno);

  /* Close socket */

  TEST_ASSERT_TRUE(close(test_sd[tidx]) >= 0);
  test_sd[tidx] = -1;
}

static FAR void * usrsock_blocking_send_thread(FAR void *param)
{
  do_usrsock_blocking_send_thread(param);
  return NULL;
}

static void do_usrsock_blocking_recv_thread(FAR void *param)
{
  intptr_t tidx = (intptr_t)param;
  struct sockaddr_in addr;
  int ret;
  bool test_abort = !!(test_flags & TEST_FLAG_DAEMON_ABORT);
  bool test_hang = !!(test_flags & TEST_FLAG_PAUSE_USRSOCK_HANDLING);

  TEST_ASSERT_TRUE(test_hang || !test_hang);

  /* Open socket. */

  test_sd[tidx] = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(test_sd[tidx] >= 0);

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);

  /* Connect socket. */

  ret = connect(test_sd[tidx], (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(0, ret);

  /* Allow main thread to hang usrsock daemon at this point. */

  sem_post(&tid_startsem);
  sem_wait(&tid_releasesem);

  /* Attempt blocking recv. */

  sem_post(&tid_startsem);
  ret = recv(test_sd[tidx], &addr, sizeof(addr), 0);
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(test_abort ? EPIPE : EINTR, errno);

  /* Close socket */

  TEST_ASSERT_TRUE(close(test_sd[tidx]) >= 0);
  test_sd[tidx] = -1;
}

static FAR void * usrsock_blocking_recv_thread(FAR void *param)
{
  do_usrsock_blocking_recv_thread(param);
  return NULL;
}

static void do_usrsock_blocking_poll_thread(FAR void *param)
{
  intptr_t tidx = (intptr_t)param;
  struct sockaddr_in addr;
  int ret;
  struct pollfd pfd = {};
  bool test_abort = !!(test_flags & TEST_FLAG_DAEMON_ABORT);
  bool test_hang = !!(test_flags & TEST_FLAG_PAUSE_USRSOCK_HANDLING);

  TEST_ASSERT_TRUE(test_abort);
  TEST_ASSERT_TRUE(test_hang || !test_hang);

  /* Open socket. */

  test_sd[tidx] = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(test_sd[tidx] >= 0);

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);

  /* Connect socket. */

  ret = connect(test_sd[tidx], (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(0, ret);

  /* Allow main thread to hang usrsock daemon at this point. */

  sem_post(&tid_startsem);
  sem_wait(&tid_releasesem);

  /* Attempt poll. */

  pfd.fd = test_sd[tidx];
  pfd.events = POLLIN;

  sem_post(&tid_startsem);
  ret = poll(&pfd, 1, -1);
  TEST_ASSERT_EQUAL(1, ret);
  TEST_ASSERT_EQUAL(POLLERR | POLLHUP, pfd.revents);

  /* Attempt read from aborted socket */

  ret = recv(test_sd[tidx], &addr, sizeof(addr), 0);
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EPIPE, errno);

  /* Close socket */

  TEST_ASSERT_TRUE(close(test_sd[tidx]) >= 0);
  test_sd[tidx] = -1;
}

static FAR void * usrsock_blocking_poll_thread(FAR void *param)
{
  do_usrsock_blocking_poll_thread(param);
  return NULL;
}

static void do_wake_test(enum e_test_type type, int flags)
{
  static const struct
  {
    pthread_startroutine_t fn;
    bool stop_only_on_hang;
  } thread_funcs[__TEST_TYPE_MAX] =
    {
      [TEST_TYPE_SOCKET]      = { usrsock_blocking_socket_thread, false },
      [TEST_TYPE_CLOSE]       = { usrsock_blocking_close_thread, false },
      [TEST_TYPE_CONNECT]     = { usrsock_blocking_connect_thread, true },
      [TEST_TYPE_SETSOCKOPT]  = { usrsock_blocking_setsockopt_thread, false },
      [TEST_TYPE_GETSOCKOPT]  = { usrsock_blocking_getsockopt_thread, false },
      [TEST_TYPE_RECV]        = { usrsock_blocking_recv_thread, true },
      [TEST_TYPE_SEND]        = { usrsock_blocking_send_thread, true },
      [TEST_TYPE_POLL]        = { usrsock_blocking_poll_thread, true },
    };
  int ret;
  int nthreads = (flags & TEST_FLAG_MULTI_THREAD) ? MAX_THREADS : 1;
  int tidx;
  bool test_abort = !!(flags & TEST_FLAG_DAEMON_ABORT);
  bool test_hang = !!(flags & TEST_FLAG_PAUSE_USRSOCK_HANDLING);

  /* Start test daemon. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(&usrsocktest_daemon_config));
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Launch worker threads. */

  test_type = type;
  test_flags = flags;
  for (tidx = 0; tidx < nthreads; tidx++)
    {
      ret = pthread_create(&tid[tidx], NULL, thread_funcs[type].fn,
                           (pthread_addr_t)(intptr_t)tidx);
      TEST_ASSERT_EQUAL(OK, ret);
    }

/* Let workers to start. */

  for (tidx = 0; tidx < nthreads; tidx++)
    {
      sem_wait(&tid_startsem);
    }

  if (test_hang || !thread_funcs[type].stop_only_on_hang)
    {
      TEST_ASSERT_EQUAL(0, usrsocktest_daemon_pause_usrsock_handling(true));
    }

  for (tidx = 0; tidx < nthreads; tidx++)
    {
      sem_post(&tid_releasesem);
    }
  for (tidx = 0; tidx < nthreads; tidx++)
    {
      sem_wait(&tid_startsem);
    }

  usleep(100 * USEC_PER_MSEC); /* Let worker thread proceed to blocking
                                * function. */

  if (!test_abort)
    {
      /* Wake waiting thread with signal. */

      /* Send signal to task to break out from blocking send. */

      for (tidx = 0; tidx < nthreads; tidx++)
        {
          pthread_kill(tid[tidx], 1);

          /* Wait threads to complete work. */

          ret = pthread_join(tid[tidx], NULL);
          TEST_ASSERT_EQUAL(OK, ret);
          tid[tidx] = -1;
        }
      TEST_ASSERT_FALSE(usrsocktest_test_failed);

      /* Stopping daemon should succeed. */

      TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
      TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
      TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
    }
  else
    {
      /* Wake waiting thread with daemon abort. */

      /* Stopping daemon should succeed. */

      TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
      TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
      TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);

      /* Wait threads to complete work. */

      for (tidx = 0; tidx < nthreads; tidx++)
        {
          ret = pthread_join(tid[tidx], NULL);
          TEST_ASSERT_EQUAL(OK, ret);
          tid[tidx] = -1;
        }
      TEST_ASSERT_FALSE(usrsocktest_test_failed);
    }
}

/****************************************************************************
 * Name: WakeWithSignal test group setup
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

TEST_SETUP(WakeWithSignal)
{
  int i;

  for (i = 0; i < MAX_THREADS; i++)
    {
      tid[i] = -1;
      test_sd[i] = -1;
    }
  sem_init(&tid_startsem, 0, 0);
  sem_init(&tid_releasesem, 0, 0);
}

/****************************************************************************
 * Name: WakeWithSignal test group teardown
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

TEST_TEAR_DOWN(WakeWithSignal)
{
  int ret;
  int i;

  for (i = 0; i < MAX_THREADS; i++)
    {
      if (tid[i] != -1)
        {
          ret = pthread_cancel(tid[i]);
          assert(ret == OK);
          ret = pthread_join(tid[i], NULL);
          assert(ret == OK);
        }
      if (test_sd[i] != -1)
        {
          close(test_sd[i]);
          test_sd[i] = -1;
        }
    }
  sem_destroy(&tid_startsem);
  sem_destroy(&tid_releasesem);
}

/****************************************************************************
 * Name: WakeBlockingConnect
 *
 * Description:
 *   Wake blocking connect with signal
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

TEST(WakeWithSignal, WakeBlockingConnect)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_CONNECT, 0);
}

/****************************************************************************
 * Name: WakeBlockingConnectMultiThread
 *
 * Description:
 *   Wake multiple blocking connect with signal
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

TEST(WakeWithSignal, WakeBlockingConnectMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_CONNECT, TEST_FLAG_MULTI_THREAD);
}

/****************************************************************************
 * Name: WakeBlockingSend
 *
 * Description:
 *   Wake blocking send with signal
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

TEST(WakeWithSignal, WakeBlockingSend)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_SEND, 0);
}

/****************************************************************************
 * Name: WakeBlockingSendMultiThread
 *
 * Description:
 *   Wake multiple blocking send with signal
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

TEST(WakeWithSignal, WakeBlockingSendMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_SEND, TEST_FLAG_MULTI_THREAD);
}
/****************************************************************************
 * Name: WakeBlockingRecv
 *
 * Description:
 *   Wake blocking recv with signal
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

TEST(WakeWithSignal, WakeBlockingRecv)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = false;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_RECV, 0);
}

/****************************************************************************
 * Name: WakeBlockingRecvMultiThread
 *
 * Description:
 *   Wake multiple blocking recv with signal
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

TEST(WakeWithSignal, WakeBlockingRecvMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = false;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_RECV, TEST_FLAG_MULTI_THREAD);
}

/****************************************************************************
 * Name: AbortBlockingConnect
 *
 * Description:
 *   Wake blocking connect with daemon abort
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

TEST(WakeWithSignal, AbortBlockingConnect)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_CONNECT, TEST_FLAG_DAEMON_ABORT);
}

/****************************************************************************
 * Name: AbortBlockingConnectMultiThread
 *
 * Description:
 *   Wake multiple blocking connect with daemon abort
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

TEST(WakeWithSignal, AbortBlockingConnectMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_CONNECT,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_MULTI_THREAD);
}

/****************************************************************************
 * Name: AbortBlockingSend
 *
 * Description:
 *   Wake blocking send with daemon abort
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

TEST(WakeWithSignal, AbortBlockingSend)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_SEND, TEST_FLAG_DAEMON_ABORT);
}

/****************************************************************************
 * Name: AbortBlockingSendMultiThread
 *
 * Description:
 *   Wake multiple blocking send with daemon abort
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

TEST(WakeWithSignal, AbortBlockingSendMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_SEND,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_MULTI_THREAD);
}

/****************************************************************************
 * Name: AbortBlockingRecv
 *
 * Description:
 *   Wake blocking recv with daemon abort
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

TEST(WakeWithSignal, AbortBlockingRecv)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = false;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_RECV, TEST_FLAG_DAEMON_ABORT);
}

/****************************************************************************
 * Name: AbortBlockingRecvMultiThread
 *
 * Description:
 *   Wake multiple blocking recv with daemon abort
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

TEST(WakeWithSignal, AbortBlockingRecvMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = false;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_RECV,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_MULTI_THREAD);
}

/****************************************************************************
 * Name: PendingRequestBlockingConnect
 *
 * Description:
 *   Wake blocking connect with daemon abort (and daemon not handling pending
 *   request before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingConnect)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_CONNECT,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING);
}

/****************************************************************************
 * Name: PendingRequestBlockingConnectMultiThread
 *
 * Description:
 *   Wake multiple blocking connect with daemon abort (and daemon not handling
 *   pending requests before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingConnectMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_CONNECT,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING |
               TEST_FLAG_MULTI_THREAD);
}

/****************************************************************************
 * Name: PendingRequestBlockingSend
 *
 * Description:
 *   Wake blocking send with daemon abort (and daemon not handling pending
 *   request before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingSend)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_SEND,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING);
}

/****************************************************************************
 * Name: PendingRequestBlockingSendMultiThread
 *
 * Description:
 *   Wake multiple blocking send with daemon abort (and daemon not handling
 *   pending requests before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingSendMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_SEND,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING |
               TEST_FLAG_MULTI_THREAD);
}

/****************************************************************************
 * Name: PendingRequestBlockingRecv
 *
 * Description:
 *   Wake blocking recv with daemon abort (and daemon not handling pending
 *   request before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingRecv)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = false;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_RECV,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING);
}

/****************************************************************************
 * Name: PendingRequestBlockingRecvMultiThread
 *
 * Description:
 *   Wake multiple blocking recv with daemon abort (and daemon not handling
 *   pending requests before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingRecvMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = false;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_RECV,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING |
               TEST_FLAG_MULTI_THREAD);
}

/****************************************************************************
 * Name: PendingRequestBlockingOpen
 *
 * Description:
 *   Wake blocking open with daemon abort (and daemon not handling pending
 *   request before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingOpen)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_SOCKET,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING);
}

/****************************************************************************
 * Name: PendingRequestBlockingOpenMultiThread
 *
 * Description:
 *   Wake multiple blocking open with daemon abort (and daemon not handling
 *   pending requests before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingOpenMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_SOCKET,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING |
               TEST_FLAG_MULTI_THREAD);
}

/****************************************************************************
 * Name: PendingRequestBlockingClose
 *
 * Description:
 *   Wake blocking close with daemon abort (and daemon not handling pending
 *   request before abort)
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
TEST(WakeWithSignal, PendingRequestBlockingClose)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_CLOSE,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING);
}

/****************************************************************************
 * Name: PendingRequestBlockingCloseMultiThread
 *
 * Description:
 *   Wake multiple blocking close with daemon abort (and daemon not handling
 *   pending requests before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingCloseMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_CLOSE,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING |
               TEST_FLAG_MULTI_THREAD);
}

/****************************************************************************
 * Name: PendingRequestBlockingPoll
 *
 * Description:
 *   Wake blocking poll with daemon abort (and daemon not handling pending
 *   request before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingPoll)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_POLL,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING);
}

/****************************************************************************
 * Name: PendingRequestBlockingPollMultiThread
 *
 * Description:
 *   Wake multiple blocking poll with daemon abort (and daemon not handling
 *   pending requests before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingPollMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_POLL,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING |
               TEST_FLAG_MULTI_THREAD);
}

/****************************************************************************
 * Name: PendingRequestBlockingSetSockOpt
 *
 * Description:
 *   Wake blocking setsockopt with daemon abort (and daemon not handling pending
 *   request before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingSetSockOpt)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_SETSOCKOPT,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING);
}

/****************************************************************************
 * Name: PendingRequestBlockingSetSockOptMultiThread
 *
 * Description:
 *   Wake multiple blocking setsockopt with daemon abort (and daemon not
 *   handling pending requests before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingSetSockOptMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_SETSOCKOPT,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING |
               TEST_FLAG_MULTI_THREAD);
}

/****************************************************************************
 * Name: PendingRequestBlockingGetSockOpt
 *
 * Description:
 *   Wake blocking getsockopt with daemon abort (and daemon not handling pending
 *   request before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingGetSockOpt)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_GETSOCKOPT,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING);
}

/****************************************************************************
 * Name: PendingRequestBlockingGetSockOptMultiThread
 *
 * Description:
 *   Wake multiple blocking getsockopt with daemon abort (and daemon not
 *   handling pending requests before abort)
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

TEST(WakeWithSignal, PendingRequestBlockingGetSockOptMultiThread)
{
  /* Configure test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = false;
  usrsocktest_daemon_config.endpoint_block_send = true;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_recv_avail = 0;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;

  /* Run test. */

  do_wake_test(TEST_TYPE_GETSOCKOPT,
               TEST_FLAG_DAEMON_ABORT | TEST_FLAG_PAUSE_USRSOCK_HANDLING |
               TEST_FLAG_MULTI_THREAD);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(WakeWithSignal)
{
  RUN_TEST_CASE(WakeWithSignal, WakeBlockingConnect);
  RUN_TEST_CASE(WakeWithSignal, WakeBlockingConnectMultiThread);
  RUN_TEST_CASE(WakeWithSignal, WakeBlockingSend);
  RUN_TEST_CASE(WakeWithSignal, WakeBlockingSendMultiThread);
  RUN_TEST_CASE(WakeWithSignal, WakeBlockingRecv);
  RUN_TEST_CASE(WakeWithSignal, WakeBlockingRecvMultiThread);
  RUN_TEST_CASE(WakeWithSignal, AbortBlockingConnect);
  RUN_TEST_CASE(WakeWithSignal, AbortBlockingConnectMultiThread);
  RUN_TEST_CASE(WakeWithSignal, AbortBlockingSend);
  RUN_TEST_CASE(WakeWithSignal, AbortBlockingSendMultiThread);
  RUN_TEST_CASE(WakeWithSignal, AbortBlockingRecv);
  RUN_TEST_CASE(WakeWithSignal, AbortBlockingRecvMultiThread);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingConnect);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingConnectMultiThread);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingSend);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingSendMultiThread);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingRecv);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingRecvMultiThread);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingOpen);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingOpenMultiThread);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingClose);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingCloseMultiThread);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingPoll);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingPollMultiThread);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingSetSockOpt);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingSetSockOptMultiThread);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingGetSockOpt);
  RUN_TEST_CASE(WakeWithSignal, PendingRequestBlockingGetSockOptMultiThread);
}

