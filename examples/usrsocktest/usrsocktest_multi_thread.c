/****************************************************************************
 * examples/usrsocktest/usrsocktest_multi_thread.c
 * Multi-threaded access to sockets
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
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "defines.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_t tids[4];
static int sds[4];
static bool started;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void usrsock_socket_multitask_do_work(int *sd)
{
  struct sockaddr_in addr;
  int ret;
  int i;

  for (i = 0; i < 10; i++)
    {
      /* Simple test for opening socket with usrsock daemon running. */

      *sd = socket(AF_INET, SOCK_STREAM, 0);
      TEST_ASSERT_TRUE(*sd >= 0);

      inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
      addr.sin_family = AF_INET;
      addr.sin_port = htons(255);
      ret = connect(*sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
      TEST_ASSERT_EQUAL(0, ret);

      /* Close socket */

      TEST_ASSERT_TRUE(close(*sd) >= 0);
      *sd = -1;
    }
}

static FAR void *usrsock_socket_multitask_thread(FAR void *param)
{
  usrsock_socket_multitask_do_work((int *)param);
  return NULL;
}

/****************************************************************************
 * Name: multithread test group setup
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

TEST_SETUP(multithread)
{
  int i;

  for (i = 0; i < ARRAY_SIZE(sds); i++)
    {
      sds[i] = -1;
    }

  for (i = 0; i < ARRAY_SIZE(tids); i++)
    {
      tids[i] = -1;
    }

  started = false;
}

/****************************************************************************
 * Name: multithread test group teardown
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

TEST_TEAR_DOWN(multithread)
{
  int ret;
  int i;

  for (i = 0; i < ARRAY_SIZE(tids); i++)
    {
      if (tids[i] != -1)
        {
          ret = pthread_cancel(tids[i]);
          assert(ret == OK);
          ret = pthread_join(tids[i], NULL);
          assert(ret == OK);
        }
    }

  for (i = 0; i < ARRAY_SIZE(sds); i++)
    {
      if (sds[i] != -1)
        {
          ret = close(sds[i]);
          assert(ret >= 0);
        }
    }

  if (started)
    {
      ret = usrsocktest_daemon_stop();
      assert(ret == OK);
    }
}

/****************************************************************************
 * Name: open_close
 *
 * Description:
 *   Open and close socket with multiple threads
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

TEST(multithread, open_close)
{
  int ret;
  int i;

  /* Start test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  usrsocktest_daemon_config.endpoint_block_send = false;
  usrsocktest_daemon_config.endpoint_block_connect = false;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;
  TEST_ASSERT_EQUAL(OK,
                    usrsocktest_daemon_start(&usrsocktest_daemon_config));
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Launch worker threads. */

  for (i = 0; i < ARRAY_SIZE(tids); i++)
    {
      ret = pthread_create(&tids[i], NULL, usrsock_socket_multitask_thread,
                           sds + i);
      TEST_ASSERT_EQUAL(OK, ret);
    }

  /* Wait threads to complete work. */

  while (--i > -1)
    {
      pthread_addr_t tparam;

      ret = pthread_join(tids[i], &tparam);
      TEST_ASSERT_EQUAL(OK, ret);
      tids[i] = -1;

      /* This flag is set whenever a test fails, otherwise it is not touched
       * No need for synchronization. Here we bail from main test thread on
       * first failure in any thread.
       */

      TEST_ASSERT_FALSE(usrsocktest_test_failed);
    }

  /* Stopping daemon should succeed. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(multithread)
{
  RUN_TEST_CASE(multithread, open_close);
}
