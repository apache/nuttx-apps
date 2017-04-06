/****************************************************************************
 * examples/usrsocktest/usrsocktest_basic_daemon.c
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
static int sd, sd2, sd3;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: NoActiveSockets
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

static void NoActiveSockets(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;

  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());
}

/****************************************************************************
 * Name: OpenClose
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

static void OpenClose(FAR struct usrsocktest_daemon_conf_s *dconf)
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
 * Name: UnsupportedType
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

static void UnsupportedType(FAR struct usrsocktest_daemon_conf_s *dconf)
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
 * Name: UnsupportedProto
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

static void UnsupportedProto(FAR struct usrsocktest_daemon_conf_s *dconf)
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
 * Name: OpenThree
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

static void OpenThree(FAR struct usrsocktest_daemon_conf_s *dconf)
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
 * Name: Dup
 *
 * Description:
 *   Dup opened socket
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

static void Dup(FAR struct usrsocktest_daemon_conf_s *dconf)
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
 * Name: Dup2
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

static void Dup2(FAR struct usrsocktest_daemon_conf_s *dconf)
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
 * Name: Stops
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

static void Stops(FAR struct usrsocktest_daemon_conf_s *dconf)
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
 * Name: StopsStarts
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

static void StopsStarts(FAR struct usrsocktest_daemon_conf_s *dconf)
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
 * Name: BasicDaemon test group setup
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

TEST_SETUP(BasicDaemon)
{
  sd = -1;
  sd2 = -1;
  sd3 = -1;
  started = false;
}

/****************************************************************************
 * Name: BasicDaemon test group teardown
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

TEST_TEAR_DOWN(BasicDaemon)
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

TEST(BasicDaemon, NoActiveSockets)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  NoActiveSockets(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, NoActiveSocketsDelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  NoActiveSockets(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, OpenClose)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  OpenClose(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, OpenCloseDelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  OpenClose(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, UnsupportedType)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  UnsupportedType(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, UnsupportedTypeDelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  UnsupportedType(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, UnsupportedProto)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  UnsupportedProto(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, UnsupportedProtoDelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  UnsupportedProto(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, OpenThree)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  OpenThree(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, OpenThreeDelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  OpenThree(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, Dup)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  Dup(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, DupDelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  Dup(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, Dup2)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  Dup2(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, Dup2Delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  Dup2(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, Stops)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  Stops(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, StopsDelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  Stops(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, StopsStarts)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  StopsStarts(&usrsocktest_daemon_config);
}

TEST(BasicDaemon, StopsStartsDelay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  StopsStarts(&usrsocktest_daemon_config);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(BasicDaemon)
{
  RUN_TEST_CASE(BasicDaemon, NoActiveSockets);
  RUN_TEST_CASE(BasicDaemon, NoActiveSocketsDelay);
  RUN_TEST_CASE(BasicDaemon, OpenClose);
  RUN_TEST_CASE(BasicDaemon, OpenCloseDelay);
  RUN_TEST_CASE(BasicDaemon, UnsupportedType);
  RUN_TEST_CASE(BasicDaemon, UnsupportedTypeDelay);
  RUN_TEST_CASE(BasicDaemon, UnsupportedProto);
  RUN_TEST_CASE(BasicDaemon, UnsupportedProtoDelay);
  RUN_TEST_CASE(BasicDaemon, OpenThree);
  RUN_TEST_CASE(BasicDaemon, OpenThreeDelay);
  RUN_TEST_CASE(BasicDaemon, Dup);
  RUN_TEST_CASE(BasicDaemon, DupDelay);
  RUN_TEST_CASE(BasicDaemon, Dup2);
  RUN_TEST_CASE(BasicDaemon, Dup2Delay);
  RUN_TEST_CASE(BasicDaemon, Stops);
  RUN_TEST_CASE(BasicDaemon, StopsDelay);
  RUN_TEST_CASE(BasicDaemon, StopsStarts);
  RUN_TEST_CASE(BasicDaemon, StopsStartsDelay);
}

