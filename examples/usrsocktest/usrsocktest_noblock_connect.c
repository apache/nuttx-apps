/****************************************************************************
 * apps/examples/usrsocktest/usrsocktest_noblock_connect.c
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
static int sd2;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

TEST_SETUP(no_block_connect)
{
  sd = -1;
  sd2 = -1;
  started = false;
}

TEST_TEAR_DOWN(no_block_connect)
{
  int ret;

  if (sd >= 0)
    {
      ret = close(sd);
      assert(ret == 0);
    }

  if (sd2 >= 0)
    {
      ret = close(sd2);
      assert(ret == 0);
    }

  if (started)
    {
      ret = usrsocktest_daemon_stop();
      assert(ret == OK);
    }
}

TEST(no_block_connect, instant_connect)
{
  int flags;
  int ret;
  struct sockaddr_in addr;

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;
  TEST_ASSERT_EQUAL(OK,
                    usrsocktest_daemon_start(&usrsocktest_daemon_config));
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
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

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

TEST(no_block_connect, delayed_connect)
{
  int flags;
  int ret;
  int count;
  struct sockaddr_in addr;

  /* Start test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;
  usrsocktest_daemon_config.delay_all_responses = true;
  TEST_ASSERT_EQUAL(OK,
                    usrsocktest_daemon_start(&usrsocktest_daemon_config));
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

  /* Another connect attempt results EALREADY. */

  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EALREADY, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Release delayed connect. */

  TEST_ASSERT_TRUE(usrsocktest_daemon_establish_waiting_connections());
  for (count = 0;
       usrsocktest_daemon_get_num_waiting_connect_sockets() > 0; count++)
    {
      TEST_ASSERT_TRUE(count <= 5);
      usleep(10 * 1000);
    }

  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Another connect attempt results EISCONN. */

  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EISCONN, errno);

  /* Close socket. */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Stopping daemon should succeed. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(-ENODEV,
                    usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

TEST(no_block_connect, close_not_connected)
{
  int flags;
  int ret;
  struct sockaddr_in addr;

  /* Start test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;
  usrsocktest_daemon_config.delay_all_responses = true;
  TEST_ASSERT_EQUAL(OK,
                    usrsocktest_daemon_start(&usrsocktest_daemon_config));
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

  /* Close socket. */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Stopping daemon should succeed. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(-ENODEV,
                    usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

TEST(no_block_connect, early_drop)
{
  int flags;
  int ret;
  struct sockaddr_in addr;

  /* Start test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;
  usrsocktest_daemon_config.delay_all_responses = false;
  TEST_ASSERT_EQUAL(OK,
                    usrsocktest_daemon_start(&usrsocktest_daemon_config));
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

  /* Stopping daemon should succeed. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(-ENODEV,
                    usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);

  /* Close socket. */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(-ENODEV,
                    usrsocktest_daemon_get_num_waiting_connect_sockets());
}

TEST(no_block_connect, multiple)
{
  int flags;
  int ret;
  int count;
  struct sockaddr_in addr;

  /* Start test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;
  usrsocktest_daemon_config.delay_all_responses = false;
  TEST_ASSERT_EQUAL(OK,
                    usrsocktest_daemon_start(&usrsocktest_daemon_config));
  started = true;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Open sockets */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  sd2 = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd2 >= 0);
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
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

  flags = fcntl(sd2, F_GETFL, 0);
  TEST_ASSERT_TRUE(flags >= 0);
  TEST_ASSERT_EQUAL(O_RDWR, flags & O_RDWR);
  TEST_ASSERT_EQUAL(0, flags & O_NONBLOCK);
  ret = fcntl(sd2, F_SETFL, flags | O_NONBLOCK);
  TEST_ASSERT_EQUAL(0, ret);
  flags = fcntl(sd2, F_GETFL, 0);
  TEST_ASSERT_TRUE(flags >= 0);
  TEST_ASSERT_EQUAL(O_RDWR, flags & O_RDWR);
  TEST_ASSERT_EQUAL(O_NONBLOCK, flags & O_NONBLOCK);

  /* Launch connect attempts, daemon delays actual connection until
   * triggered.
   */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd2, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EINPROGRESS, errno);
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Release delayed connections. */

  TEST_ASSERT_TRUE(usrsocktest_daemon_establish_waiting_connections());
  for (count = 0;
       usrsocktest_daemon_get_num_waiting_connect_sockets() > 0; count++)
    {
      TEST_ASSERT_TRUE(count <= 5);
      usleep(10 * 1000);
    }

  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd2, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EISCONN, errno);

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EINPROGRESS, errno);
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Release delayed connections. */

  TEST_ASSERT_TRUE(usrsocktest_daemon_establish_waiting_connections());
  for (count = 0; usrsocktest_daemon_get_num_waiting_connect_sockets() > 0;
       count++)
    {
      TEST_ASSERT_TRUE(count <= 5);
      usleep(10 * 1000);
    }

  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Close sockets. */

  TEST_ASSERT_TRUE(close(sd2) >= 0);
  sd2 = -1;
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Open sockets */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  sd2 = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd2 >= 0);
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
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

  flags = fcntl(sd2, F_GETFL, 0);
  TEST_ASSERT_TRUE(flags >= 0);
  TEST_ASSERT_EQUAL(O_RDWR, flags & O_RDWR);
  TEST_ASSERT_EQUAL(0, flags & O_NONBLOCK);
  ret = fcntl(sd2, F_SETFL, flags | O_NONBLOCK);
  TEST_ASSERT_EQUAL(0, ret);
  flags = fcntl(sd2, F_GETFL, 0);
  TEST_ASSERT_TRUE(flags >= 0);
  TEST_ASSERT_EQUAL(O_RDWR, flags & O_RDWR);
  TEST_ASSERT_EQUAL(O_NONBLOCK, flags & O_NONBLOCK);

  /* Launch connect attempts, daemon delays actual connection until
   * triggered.
   */

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(255);
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EINPROGRESS, errno);
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_waiting_connect_sockets());

  ret = connect(sd2, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EINPROGRESS, errno);
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Release delayed connections. */

  TEST_ASSERT_TRUE(usrsocktest_daemon_establish_waiting_connections());
  for (count = 0;
       usrsocktest_daemon_get_num_waiting_connect_sockets() > 0; count++)
    {
      TEST_ASSERT_TRUE(count <= 5);
      usleep(10 * 1000);
    }

  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Close sockets. */

  TEST_ASSERT_TRUE(close(sd2) >= 0);
  sd2 = -1;
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Open sockets */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());
  sd2 = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd2 >= 0);
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
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

  flags = fcntl(sd2, F_GETFL, 0);
  TEST_ASSERT_TRUE(flags >= 0);
  TEST_ASSERT_EQUAL(O_RDWR, flags & O_RDWR);
  TEST_ASSERT_EQUAL(0, flags & O_NONBLOCK);
  ret = fcntl(sd2, F_SETFL, flags | O_NONBLOCK);
  TEST_ASSERT_EQUAL(0, ret);
  flags = fcntl(sd2, F_GETFL, 0);
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
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Release delayed connections. */

  TEST_ASSERT_TRUE(usrsocktest_daemon_establish_waiting_connections());
  for (count = 0;
       usrsocktest_daemon_get_num_waiting_connect_sockets() > 0; count++)
    {
      TEST_ASSERT_TRUE(count <= 5);
      usleep(10 * 1000);
    }

  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Launch another connect attempt, daemon delays actual connection until
   * triggered.
   */

  ret = connect(sd2, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EINPROGRESS, errno);
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Close sockets. */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_TRUE(close(sd2) >= 0);
  sd2 = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Stopping daemon should succeed. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(-ENODEV,
                    usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

TEST(no_block_connect, basic_daemon_dup2)
{
  int flags;
  int ret;
  int count;
  struct sockaddr_in addr;

  /* Start test daemon. */

  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.endpoint_block_connect = true;
  usrsocktest_daemon_config.endpoint_addr = "127.0.0.1";
  usrsocktest_daemon_config.endpoint_port = 255;
  usrsocktest_daemon_config.delay_all_responses = true;
  TEST_ASSERT_EQUAL(OK,
                    usrsocktest_daemon_start(&usrsocktest_daemon_config));
  started = true;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Duplicate socket */

  sd2 = dup(sd);
  TEST_ASSERT_TRUE(sd2 >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());

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

  /* Another connect attempt results EALREADY. */

  ret = connect(sd2, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EALREADY, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Release delayed connect. */

  TEST_ASSERT_TRUE(usrsocktest_daemon_establish_waiting_connections());
  for (count = 0;
       usrsocktest_daemon_get_num_waiting_connect_sockets() > 0; count++)
    {
      TEST_ASSERT_TRUE(count <= 5);
      usleep(10 * 1000);
    }

  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Another connect attempt results EISCONN. */

  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EISCONN, errno);

  /* Close sockets. */

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  TEST_ASSERT_TRUE(close(sd2) >= 0);
  sd2 = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_waiting_connect_sockets());

  /* Stopping daemon should succeed. */

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_connected_sockets());
  TEST_ASSERT_EQUAL(-ENODEV,
                    usrsocktest_daemon_get_num_waiting_connect_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(no_block_connect)
{
  RUN_TEST_CASE(no_block_connect, instant_connect);
  RUN_TEST_CASE(no_block_connect, delayed_connect);
  RUN_TEST_CASE(no_block_connect, close_not_connected);
  RUN_TEST_CASE(no_block_connect, early_drop);
  RUN_TEST_CASE(no_block_connect, multiple);
  RUN_TEST_CASE(no_block_connect, basic_daemon_dup2);
}
