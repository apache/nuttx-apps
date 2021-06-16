/****************************************************************************
 * apps/examples/usrsocktest/usrsocktest_basic_daemon.c
 * Basic tests with socket daemon
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
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <assert.h>
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
static int sd3;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: no_active_sockets
 *
 * Description:
 *   Checks there is no active sockets on daemon startup
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

static void no_active_sockets(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;

  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
}

/****************************************************************************
 * Name: open_close
 *
 * Description:
 *   Open and close AF_INET socket, check active socket counter updates
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

static void open_close(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());

  TEST_ASSERT_TRUE(close(sd) >= 0);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Name: unsupported_type
 *
 * Description:
 *   Try open socket for unsupported type
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

static void unsupported_type(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;

  sd = socket(AF_INET, SOCK_RDM, 0);
  TEST_ASSERT_TRUE(sd < 0);
  TEST_ASSERT_EQUAL(EPROTONOSUPPORT, errno);
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Name: unsupported_proto
 *
 * Description:
 *   Try open socket for unsupported protocol
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

static void unsupported_proto(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;

  sd = socket(AF_INET, SOCK_STREAM, 1);
  TEST_ASSERT_TRUE(sd < 0);
  TEST_ASSERT_EQUAL(EPROTONOSUPPORT, errno);
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Name: open_three
 *
 * Description:
 *   Open multiple sockets
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

static void open_three(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  int ret;

  dconf->max_sockets = 3;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  sd2 = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd2 >= 0);
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
  sd3 = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd3 >= 0);
  TEST_ASSERT_EQUAL(3, usrsocktest_daemon_get_num_active_sockets());
  ret = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_FALSE(ret >= 0);
  TEST_ASSERT_EQUAL(3, usrsocktest_daemon_get_num_active_sockets());

  ret = close(sd2);
  TEST_ASSERT_EQUAL(0, ret);
  sd2 = -1;
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());
  ret = close(sd);
  TEST_ASSERT_EQUAL(0, ret);
  sd = -1;
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  ret = close(sd3);
  TEST_ASSERT_EQUAL(0, ret);
  sd3 = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Name: basic_daemon_dup
 *
 * Description:
 *   basic_daemon_dup opened socket
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

static void basic_daemon_dup(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  int ret;

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());

  sd2 = dup(sd);
  TEST_ASSERT_TRUE(sd2 >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());

  sd3 = dup(sd);
  TEST_ASSERT_TRUE(sd3 >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());

  ret = close(sd2);
  TEST_ASSERT_EQUAL(0, ret);
  sd2 = -1;
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  ret = close(sd);
  TEST_ASSERT_EQUAL(0, ret);
  sd = -1;
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  ret = close(sd3);
  TEST_ASSERT_EQUAL(0, ret);
  sd3 = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Name: basic_daemon_dup2
 *
 * Description:
 *   Clone opened socket with dup2
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

static void basic_daemon_dup2(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  int ret;

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  sd2 = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());

  ret = dup2(sd2, sd);
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());

  ret = close(sd2);
  TEST_ASSERT_EQUAL(0, ret);
  sd2 = -1;
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  ret = close(sd);
  TEST_ASSERT_EQUAL(0, ret);
  sd = -1;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Name: stops
 *
 * Description:
 *   Daemon stops unexpectedly
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

static void stops(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  int ret;

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  sd2 = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd2 >= 0);
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());

  ret = usrsocktest_daemon_stop();
  TEST_ASSERT_EQUAL(OK, ret);
  started = false;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);

  TEST_ASSERT_EQUAL(0, close(sd));
  sd = -1;
  TEST_ASSERT_EQUAL(0, close(sd2));
  sd2 = -1;
}

/****************************************************************************
 * Name: stops_starts
 *
 * Description:
 *   Daemon stops and restarts unexpectedly
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

static void stops_starts(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  int ret;
  struct sockaddr_in addr;

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  sd2 = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd2 >= 0);
  TEST_ASSERT_EQUAL(2, usrsocktest_daemon_get_num_active_sockets());

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(-ENODEV, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;

  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = 255;
  ret = connect(sd, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EPIPE, errno);

  ret = connect(sd2, (FAR const struct sockaddr *)&addr, sizeof(addr));
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EPIPE, errno);

  TEST_ASSERT_EQUAL(0, close(sd));
  sd = -1;
  TEST_ASSERT_EQUAL(0, close(sd2));
  sd2 = -1;

  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_stop());
  started = false;
  TEST_ASSERT_EQUAL(0, usrsocktest_endp_malloc_cnt);
  TEST_ASSERT_EQUAL(0, usrsocktest_dcmd_malloc_cnt);
}

/****************************************************************************
 * Name: basic_daemon test group setup
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

TEST_SETUP(basic_daemon)
{
  sd = -1;
  sd2 = -1;
  sd3 = -1;
  started = false;
}

/****************************************************************************
 * Name: basic_daemon test group teardown
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

TEST_TEAR_DOWN(basic_daemon)
{
  int ret;
  if (sd >= 0)
    {
      ret = close(sd);
      assert(ret >= 0);
    }

  if (sd2 >= 0)
    {
      ret = close(sd2);
      assert(ret >= 0);
    }

  if (sd3 >= 0)
    {
      ret = close(sd3);
      assert(ret >= 0);
    }

  if (started)
    {
      ret = usrsocktest_daemon_stop();
      assert(ret == OK);
    }
}

TEST(basic_daemon, no_active_sockets)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  no_active_sockets(&usrsocktest_daemon_config);
}

TEST(basic_daemon, no_active_sockets_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  no_active_sockets(&usrsocktest_daemon_config);
}

TEST(basic_daemon, open_close)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  open_close(&usrsocktest_daemon_config);
}

TEST(basic_daemon, open_close_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  open_close(&usrsocktest_daemon_config);
}

TEST(basic_daemon, unsupported_type)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  unsupported_type(&usrsocktest_daemon_config);
}

TEST(basic_daemon, unsupported_type_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  unsupported_type(&usrsocktest_daemon_config);
}

TEST(basic_daemon, unsupported_proto)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  unsupported_proto(&usrsocktest_daemon_config);
}

TEST(basic_daemon, unsupported_proto_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  unsupported_proto(&usrsocktest_daemon_config);
}

TEST(basic_daemon, open_three)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  open_three(&usrsocktest_daemon_config);
}

TEST(basic_daemon, open_three_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  open_three(&usrsocktest_daemon_config);
}

TEST(basic_daemon, basic_daemon_dup)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  basic_daemon_dup(&usrsocktest_daemon_config);
}

TEST(basic_daemon, basic_daemon_dup_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  basic_daemon_dup(&usrsocktest_daemon_config);
}

TEST(basic_daemon, basic_daemon_dup2)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  basic_daemon_dup2(&usrsocktest_daemon_config);
}

TEST(basic_daemon, basic_daemon_dup2_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  basic_daemon_dup2(&usrsocktest_daemon_config);
}

TEST(basic_daemon, stops)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  stops(&usrsocktest_daemon_config);
}

TEST(basic_daemon, stops_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  stops(&usrsocktest_daemon_config);
}

TEST(basic_daemon, stops_starts)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  stops_starts(&usrsocktest_daemon_config);
}

TEST(basic_daemon, stops_starts_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  stops_starts(&usrsocktest_daemon_config);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(basic_daemon)
{
  RUN_TEST_CASE(basic_daemon, no_active_sockets);
  RUN_TEST_CASE(basic_daemon, no_active_sockets_delay);
  RUN_TEST_CASE(basic_daemon, open_close);
  RUN_TEST_CASE(basic_daemon, open_close_delay);
  RUN_TEST_CASE(basic_daemon, unsupported_type);
  RUN_TEST_CASE(basic_daemon, unsupported_type_delay);
  RUN_TEST_CASE(basic_daemon, unsupported_proto);
  RUN_TEST_CASE(basic_daemon, unsupported_proto_delay);
  RUN_TEST_CASE(basic_daemon, open_three);
  RUN_TEST_CASE(basic_daemon, open_three_delay);
  RUN_TEST_CASE(basic_daemon, basic_daemon_dup);
  RUN_TEST_CASE(basic_daemon, basic_daemon_dup_delay);
  RUN_TEST_CASE(basic_daemon, basic_daemon_dup2);
  RUN_TEST_CASE(basic_daemon, basic_daemon_dup2_delay);
  RUN_TEST_CASE(basic_daemon, stops);
  RUN_TEST_CASE(basic_daemon, stops_delay);
  RUN_TEST_CASE(basic_daemon, stops_starts);
  RUN_TEST_CASE(basic_daemon, stops_starts_delay);
}
