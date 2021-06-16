/****************************************************************************
 * apps/examples/usrsocktest/usrsocktest_basic_getsockname.c
 * Basic getsockname tests
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

static bool started;
static int sd;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: basic_getsockname_open
 *
 * Description:
 *   Open and get socket options
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

static
void basic_getsockname_open(FAR struct usrsocktest_daemon_conf_s *dconf)
{
  int ret;
  socklen_t addrlen;
  union {
    struct sockaddr_storage storage;
    struct sockaddr_in in;
  } addr;

  /* Start test daemon. */

  dconf->endpoint_addr = "127.0.0.1";
  dconf->endpoint_port = 255;
  TEST_ASSERT_EQUAL(OK, usrsocktest_daemon_start(dconf));
  started = true;
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_active_sockets());

  /* Open socket */

  sd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT_TRUE(sd >= 0);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Get socket name */

  memset(&addr, 0xff, sizeof(addr));
  addrlen = sizeof(addr.in);
  ret = getsockname(sd, (FAR void *)&addr.in, &addrlen);
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(sizeof(addr.in), addrlen);
  TEST_ASSERT_EQUAL(AF_INET, addr.in.sin_family);
  TEST_ASSERT_EQUAL(htons(12345), addr.in.sin_port);
  TEST_ASSERT_EQUAL(htonl(0x7f000001), addr.in.sin_addr.s_addr);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Get socket name, short buffer */

  memset(&addr, 0xff, sizeof(addr));
  addrlen = 1;
  ret = getsockname(sd, (FAR void *)&addr.in, &addrlen);
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(sizeof(addr.in), addrlen);
  TEST_ASSERT_EQUAL(AF_INET, ((FAR uint8_t *)&addr.in.sin_family)[0]);
  TEST_ASSERT_EQUAL(0xff,    ((FAR uint8_t *)&addr.in.sin_family)[1]);
  TEST_ASSERT_EQUAL(htons(0xffff), addr.in.sin_port);
  TEST_ASSERT_EQUAL(htonl(0xffffffff), addr.in.sin_addr.s_addr);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Get socket name, larger buffer */

  memset(&addr, 0xff, sizeof(addr));
  addrlen = sizeof(addr.storage);
  ret = getsockname(sd, (FAR void *)&addr.in, &addrlen);
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(sizeof(addr.in), addrlen);
  TEST_ASSERT_EQUAL(AF_INET, addr.in.sin_family);
  TEST_ASSERT_EQUAL(htons(12345), addr.in.sin_port);
  TEST_ASSERT_EQUAL(htonl(0x7f000001), addr.in.sin_addr.s_addr);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Get socket name, NULL addr */

  memset(&addr, 0xff, sizeof(addr));
  addrlen = sizeof(addr.in);
  ret = getsockname(sd, NULL, &addrlen);
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EINVAL, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Get socket name, NULL addrlen */

  memset(&addr, 0xff, sizeof(addr));
  addrlen = sizeof(addr.in);
  ret = getsockname(sd, (FAR void *)&addr.in, NULL);
  TEST_ASSERT_EQUAL(-1, ret);
  TEST_ASSERT_EQUAL(EINVAL, errno);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

  /* Get socket name, zero length buffer */

  memset(&addr, 0xff, sizeof(addr));
  addrlen = 0;
  ret = getsockname(sd, (FAR void *)&addr.in, &addrlen);
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL(sizeof(addr.in), addrlen);
  TEST_ASSERT_EQUAL(0xffff, addr.in.sin_family);
  TEST_ASSERT_EQUAL(htons(0xffff), addr.in.sin_port);
  TEST_ASSERT_EQUAL(htonl(0xffffffff), addr.in.sin_addr.s_addr);
  TEST_ASSERT_EQUAL(1, usrsocktest_daemon_get_num_active_sockets());
  TEST_ASSERT_EQUAL(0, usrsocktest_daemon_get_num_connected_sockets());

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
 * Name: basic_getsockname test group setup
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

TEST_SETUP(basic_getsockname)
{
  sd = -1;
  started = false;
}

/****************************************************************************
 * Name: basic_getsockname test group teardown
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

TEST_TEAR_DOWN(basic_getsockname)
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

TEST(basic_getsockname, basic_getsockname_open)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  basic_getsockname_open(&usrsocktest_daemon_config);
}

TEST(basic_getsockname, basic_getsockname_open_delay)
{
  usrsocktest_daemon_config = usrsocktest_daemon_defconf;
  usrsocktest_daemon_config.delay_all_responses = true;
  basic_getsockname_open(&usrsocktest_daemon_config);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(basic_getsockname)
{
  RUN_TEST_CASE(basic_getsockname, basic_getsockname_open);
  RUN_TEST_CASE(basic_getsockname, basic_getsockname_open_delay);
}
