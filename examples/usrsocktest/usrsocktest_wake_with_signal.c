/****************************************************************************
 * apps/examples/usrsocktest/usrsocktest_wake_with_signal.c
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

struct thread_func
{
  pthread_startroutine_t fn;
  bool stop_only_on_hang;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static FAR void *usrsock_blocking_socket_thread(FAR void *param);
static FAR void *usrsock_blocking_close_thread(FAR void *param);
static FAR void *usrsock_blocking_connect_thread(FAR void *param);
static FAR void *usrsock_blocking_setsockopt_thread(FAR void *param);
static FAR void *usrsock_blocking_getsockopt_thread(FAR void *param);
static FAR void *usrsock_blocking_recv_thread(FAR void *param);
static FAR void *usrsock_blocking_send_thread(FAR void *param);
static FAR void *usrsock_blocking_poll_thread(FAR void *param);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_t tid[MAX_THREADS];
static sem_t tid_startsem;
static sem_t tid_releasesem;
static int test_sd[MAX_THREADS];
static enum e_test_type test_type;
static int test_flags;

static struct thread_func thread_funcs[__TEST_TYPE_MAX] =
{
  [TEST_TYPE_SOCKET]      =
  {
    usrsock_blocking_socket_thread, false
  },
  [TEST_TYPE_CLOSE]       =
  {
    usrsock_blocking_close_thread, false
  },
  [TEST_TYPE_CONNECT]     =
  {
    usrsock_blocking_connect_thread, true
  },
  [TEST_TYPE_SETSOCKOPT]  =
  {
    usrsock_blocking_setsockopt_thread, false
  },
  [TEST_TYPE_GETSOCKOPT]  =
  {
    usrsock_blocking_getsockopt_thread, false
  },
  [TEST_TYPE_RECV]        =
  {
    usrsock_blocking_recv_thread, true
  },
  [TEST_TYPE_SEND]        =
  {
    usrsock_blocking_send_thread, true
  },
  [TEST_TYPE_POLL]        =
  {
    usrsock_blocking_poll_thread, true
  },
};

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
  ret = getsockopt(test_sd[tidx], SOL_SOCKET, SO_REUSEADDR, &value,
                   &valuelen);
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

  ret = connect(test_sd[tidx], (FAR const struct sockaddr *)&addr,
                sizeof(addr));
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

  ret = connect(test_sd[tidx], (FAR const struct sockaddr *)&addr,
                sizeof(addr));
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
  struct pollfd pfd = {
  };

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

  ret = connect(test_sd[tidx], (FAR const struct sockaddr *)&addr,
                sizeof(addr));
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
  int ret;
  int nthreads = (flags & TEST_FLAG_MULTI_THREAD) ? MAX_THREADS : 1;
  int tidx;
  bool test_abort = !!(flags & TEST_FLAG_DAEMON_ABORT);
  bool test_hang = !!(flags & TEST_FLAG_PAUSE_USRSOCK_HANDLING);

  /* Start test daemon. */

  TEST_ASSERT_EQUAL(OK,
                    usrsocktest_daemon_start(&usrsocktest_daemon_config));
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
 * Name: wake_with_signal test group setup
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

TEST_SETUP(wake_with_signal)
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
 * Name: wake_with_signal test group teardown
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

TEST_TEAR_DOWN(wake_with_signal)
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
 * Name: wake_blocking_connect
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

TEST(wake_with_signal, wake_blocking_connect)
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
 * Name: wake_blocking_connect_multithread
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

TEST(wake_with_signal, wake_blocking_connect_multithread)
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
 * Name: wake_blocking_send
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

TEST(wake_with_signal, wake_blocking_send)
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
 * Name: wake_blocking_send_multithread
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

TEST(wake_with_signal, wake_blocking_send_multithread)
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
 * Name: wake_blocking_recv
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

TEST(wake_with_signal, wake_blocking_recv)
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
 * Name: wake_blocking_recv_multithread
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

TEST(wake_with_signal, wake_blocking_recv_multithread)
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
 * Name: abort_blocking_connect
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

TEST(wake_with_signal, abort_blocking_connect)
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
 * Name: abort_blocking_connect_multithread
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

TEST(wake_with_signal, abort_blocking_connect_multithread)
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
 * Name: abort_blocking_send
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

TEST(wake_with_signal, abort_blocking_send)
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
 * Name: abort_blocking_send_multithread
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

TEST(wake_with_signal, abort_blocking_send_multithread)
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
 * Name: abort_blocking_recv
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

TEST(wake_with_signal, abort_blocking_recv)
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
 * Name: abort_blocking_recv_multithread
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

TEST(wake_with_signal, abort_blocking_recv_multithread)
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
 * Name: pending_request_blocking_connect
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

TEST(wake_with_signal, pending_request_blocking_connect)
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
 * Name: pending_request_blocking_connect_multithread
 *
 * Description:
 *   Wake multiple blocking connect with daemon abort (and daemon not
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

TEST(wake_with_signal, pending_request_blocking_connect_multithread)
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
 * Name: pending_request_blocking_send
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

TEST(wake_with_signal, pending_request_blocking_send)
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
 * Name: pending_request_blocking_send_multithread
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

TEST(wake_with_signal, pending_request_blocking_send_multithread)
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
 * Name: pending_request_blocking_recv
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

TEST(wake_with_signal, pending_request_blocking_recv)
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
 * Name: pending_request_blocking_recv_multithread
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

TEST(wake_with_signal, pending_request_blocking_recv_multithread)
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
 * Name: pending_request_blocking_open
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

TEST(wake_with_signal, pending_request_blocking_open)
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
 * Name: pending_request_blocking_open_multithread
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

TEST(wake_with_signal, pending_request_blocking_open_multithread)
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
 * Name: pending_request_blocking_close
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

TEST(wake_with_signal, pending_request_blocking_close)
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
 * Name: pending_request_blocking_close_multithread
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

TEST(wake_with_signal, pending_request_blocking_close_multithread)
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
 * Name: pending_request_blocking_poll
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

TEST(wake_with_signal, pending_request_blocking_poll)
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
 * Name: pending_request_blocking_poll_multithread
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

TEST(wake_with_signal, pending_request_blocking_poll_multithread)
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
 * Name: pending_request_blocking_setsockopt
 *
 * Description:
 *   Wake blocking setsockopt with daemon abort (and daemon not handling
 *   pending request before abort)
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

TEST(wake_with_signal, pending_request_blocking_setsockopt)
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
 * Name: pending_request_blocking_setsockopt_multithread
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

TEST(wake_with_signal, pending_request_blocking_setsockopt_multithread)
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
 * Name: pending_request_blocking_getsockopt
 *
 * Description:
 *   Wake blocking getsockopt with daemon abort (and daemon not handling
 *   pending request before abort)
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

TEST(wake_with_signal, pending_request_blocking_getsockopt)
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
 * Name: pending_request_blocking_getsockopt_multithread
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

TEST(wake_with_signal, pending_request_blocking_getsockopt_multithread)
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

TEST_GROUP(wake_with_signal)
{
  RUN_TEST_CASE(wake_with_signal, wake_blocking_connect);
  RUN_TEST_CASE(wake_with_signal, wake_blocking_connect_multithread);
  RUN_TEST_CASE(wake_with_signal, wake_blocking_send);
  RUN_TEST_CASE(wake_with_signal, wake_blocking_send_multithread);
  RUN_TEST_CASE(wake_with_signal, wake_blocking_recv);
  RUN_TEST_CASE(wake_with_signal, wake_blocking_recv_multithread);
  RUN_TEST_CASE(wake_with_signal, abort_blocking_connect);
  RUN_TEST_CASE(wake_with_signal, abort_blocking_connect_multithread);
  RUN_TEST_CASE(wake_with_signal, abort_blocking_send);
  RUN_TEST_CASE(wake_with_signal, abort_blocking_send_multithread);
  RUN_TEST_CASE(wake_with_signal, abort_blocking_recv);
  RUN_TEST_CASE(wake_with_signal, abort_blocking_recv_multithread);
  RUN_TEST_CASE(wake_with_signal, pending_request_blocking_connect);
  RUN_TEST_CASE(wake_with_signal,
                pending_request_blocking_connect_multithread);
  RUN_TEST_CASE(wake_with_signal, pending_request_blocking_send);
  RUN_TEST_CASE(wake_with_signal,
                pending_request_blocking_send_multithread);
  RUN_TEST_CASE(wake_with_signal, pending_request_blocking_recv);
  RUN_TEST_CASE(wake_with_signal,
                pending_request_blocking_recv_multithread);
  RUN_TEST_CASE(wake_with_signal, pending_request_blocking_open);
  RUN_TEST_CASE(wake_with_signal,
                pending_request_blocking_open_multithread);
  RUN_TEST_CASE(wake_with_signal, pending_request_blocking_close);
  RUN_TEST_CASE(wake_with_signal,
                pending_request_blocking_close_multithread);
  RUN_TEST_CASE(wake_with_signal, pending_request_blocking_poll);
  RUN_TEST_CASE(wake_with_signal,
                pending_request_blocking_poll_multithread);
  RUN_TEST_CASE(wake_with_signal, pending_request_blocking_setsockopt);
  RUN_TEST_CASE(wake_with_signal,
                pending_request_blocking_setsockopt_multithread);
  RUN_TEST_CASE(wake_with_signal, pending_request_blocking_getsockopt);
  RUN_TEST_CASE(wake_with_signal,
                pending_request_blocking_getsockopt_multithread);
}
