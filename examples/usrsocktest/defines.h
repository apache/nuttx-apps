/****************************************************************************
 * apps/examples/usrsocktest/defines.h
 * Common defines for all usrsock testcases
 *
 *   Copyright (C) 2015, 2017 Haltian Ltd. All rights reserved.
 *   Author: Roman Saveljev <roman.saveljev@haltian.com>
 *           Jussi Kivilinna <jussi.kivilinna@haltian.com>
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

#ifndef __EXAMPLES_USRSOCKTEST_DEFINES_H
#define __EXAMPLES_USRSOCKTEST_DEFINES_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <assert.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define USRSOCK_NODE "/dev/usrsock"

#define USRSOCKTEST_DAEMON_CONF_DEFAULTS \
  { \
    .max_sockets = UINT_MAX, \
    .supported_domain = AF_INET, \
    .supported_type = SOCK_STREAM, \
    .supported_protocol = 0, \
    .delay_all_responses = false, \
    .endpoint_addr = "127.0.0.1", \
    .endpoint_port = 255, \
    .endpoint_block_connect = false, \
    .endpoint_block_send = false, \
    .endpoint_recv_avail_from_start = true, \
    .endpoint_recv_avail = 4, \
  }

/* Test case macros */

#define utest_match(type, should_be_value, test_value) do { \
    type tmp_utest_test_value = (type)(test_value); \
    type tmp_utest_should_value = (type)(should_be_value); \
    if (!usrsocktest_assert_print_value(__func__, __LINE__, \
                                        "(" #type ")(" #test_value ") == " \
                                        "(" #type ")(" #should_be_value ")", \
                                        tmp_utest_test_value, \
                                        tmp_utest_should_value)) \
      { \
        usrsocktest_test_failed = true; \
        return; \
      } \
  } while(false)

#define utest_match_buf(should_be_valuebuf, test_valuebuf, test_valuebuflen) do { \
    if (!usrsocktest_assert_print_buf(__func__, __LINE__, \
                                      "memcmp(" #should_be_valuebuf ", " \
                                                #test_valuebuf ", " \
                                                #test_valuebuflen ") == 0", \
                                      should_be_valuebuf, \
                                      test_valuebuf, \
                                      test_valuebuflen)) \
      { \
        usrsocktest_test_failed = true; \
        return; \
      } \
  } while(false)

#define RUN_TEST_CASE(g,t) do { \
    if (usrsocktest_test_failed) \
      { \
        return; \
      } \
    usrsocktest_group_##g##_setup(); \
    usrsocktest_test_##g##_##t(); \
    usrsocktest_group_##g##_teardown(); \
    if (usrsocktest_test_failed) \
      { \
        return; \
      } \
  } while (false)

#define RUN_TEST_GROUP(g) do { \
    extern const char *usrsocktest_group_##g; \
    void usrsocktest_group_##g##_run(void); \
    run_tests(usrsocktest_group_##g, usrsocktest_group_##g##_run); \
  } while (false)

#define TEST_GROUP(g)        const char *usrsocktest_group_##g = #g; \
                             void usrsocktest_group_##g##_run(void)
#define TEST_SETUP(g)        static void usrsocktest_group_##g##_setup(void)
#define TEST_TEAR_DOWN(g)    static void usrsocktest_group_##g##_teardown(void)
#define TEST(g,t)            static void usrsocktest_test_##g##_##t(void)

#define TEST_ASSERT_TRUE(v)    utest_match(bool, true, (v))
#define TEST_ASSERT_FALSE(v)   utest_match(bool, false, (v))
#define TEST_ASSERT_EQUAL(e,v) utest_match(ssize_t, (e), (v))
#define TEST_ASSERT_EQUAL_UINT8_ARRAY(e,v,s) utest_match_buf((e), (v), (s))

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct usrsocktest_daemon_conf_s
{
  unsigned int max_sockets;
  int8_t supported_domain:8;
  int8_t supported_type:8;
  int8_t supported_protocol:8;
  bool delay_all_responses:1;
  bool endpoint_block_connect:1;
  bool endpoint_block_send:1;
  bool endpoint_recv_avail_from_start:1;
  uint8_t endpoint_recv_avail:8;
  const char *endpoint_addr;
  uint16_t endpoint_port;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int usrsocktest_endp_malloc_cnt;
extern int usrsocktest_dcmd_malloc_cnt;
extern const struct usrsocktest_daemon_conf_s usrsocktest_daemon_defconf;
extern struct usrsocktest_daemon_conf_s usrsocktest_daemon_config;

extern bool usrsocktest_test_failed;

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

bool usrsocktest_assert_print_value(FAR const char *func,
                                    const int line,
                                    FAR const char *check_str,
                                    long int test_value,
                                    long int should_be);

bool usrsocktest_assert_print_buf(FAR const char *func,
                                  const int line,
                                  FAR const char *check_str,
                                  FAR const void *test_buf,
                                  FAR const void *expect_buf,
                                  size_t buflen);

int usrsocktest_daemon_start(FAR const struct usrsocktest_daemon_conf_s *conf);

int usrsocktest_daemon_stop(void);

int usrsocktest_daemon_get_num_active_sockets(void);

int usrsocktest_daemon_get_num_connected_sockets(void);

int usrsocktest_daemon_get_num_waiting_connect_sockets(void);

int usrsocktest_daemon_get_num_recv_empty_sockets(void);

ssize_t usrsocktest_daemon_get_send_bytes(void);

ssize_t usrsocktest_daemon_get_recv_bytes(void);

int usrsocktest_daemon_get_num_unreachable_sockets(void);

int usrsocktest_daemon_get_num_remote_disconnected_sockets(void);

bool usrsocktest_daemon_establish_waiting_connections(void);

bool usrsocktest_send_delayed_command(char cmd, unsigned int delay_msec);

int usrsocktest_daemon_pause_usrsock_handling(bool pause);

#endif /* __EXAMPLES_USRSOCKTEST_DEFINES_H */
