/****************************************************************************
 * apps/testing/nettest/local/test_local_dgram.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>

#include "test_local_dgram.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DGRAM_TEST(f) \
  cmocka_unit_test_setup_teardown(f, test_local_dgram_setup, \
                                  test_local_dgram_teardown)
#define DGRAM_TEST_CONNECTED(f) \
  cmocka_unit_test_setup_teardown(f, test_local_dgram_connected_setup, \
                                  test_local_dgram_teardown)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 *   The first optional argument is a writable directory for the socket
 *   names.  The second is a cmocka filter, to run some of the cases.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest local_dgram_tests[] =
    {
      DGRAM_TEST(test_local_dgram_connect),
      DGRAM_TEST_CONNECTED(test_local_dgram_peername),
      DGRAM_TEST_CONNECTED(test_local_dgram_send),
      DGRAM_TEST_CONNECTED(test_local_dgram_write),
      DGRAM_TEST_CONNECTED(test_local_dgram_sendto_null),
      DGRAM_TEST(test_local_dgram_pollin0),
      DGRAM_TEST_CONNECTED(test_local_dgram_pollin),
      DGRAM_TEST_CONNECTED(test_local_dgram_nohup),
      DGRAM_TEST_CONNECTED(test_local_dgram_pollout),
      DGRAM_TEST_CONNECTED(test_local_dgram_pollwait),
      DGRAM_TEST_CONNECTED(test_local_dgram_select),
      DGRAM_TEST(test_local_dgram_dontwait),
      DGRAM_TEST(test_local_dgram_nonblock),
      DGRAM_TEST(test_local_dgram_peeronly),
      DGRAM_TEST_CONNECTED(test_local_dgram_unspec),
      DGRAM_TEST_CONNECTED(test_local_dgram_reconnect),
      DGRAM_TEST(test_local_dgram_missing),
      DGRAM_TEST(test_local_dgram_notsock),
      DGRAM_TEST_CONNECTED(test_local_dgram_closed),
      DGRAM_TEST(test_local_dgram_abstract),
      DGRAM_TEST(test_local_dgram_addrlen),
      DGRAM_TEST(test_local_dgram_connect_empty),
      DGRAM_TEST(test_local_dgram_poll_unbound),
      DGRAM_TEST(test_local_dgram_poll_bound),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_level),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_trunc),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_peek),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_mask),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_fds),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_same),
      DGRAM_TEST(test_local_dgram_poll_sendto),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_many),
      DGRAM_TEST(test_local_dgram_poll_timeout),
      DGRAM_TEST(test_local_dgram_poll_eintr),
      DGRAM_TEST(test_local_dgram_ppoll),
      DGRAM_TEST_CONNECTED(test_local_dgram_select_rw),
      DGRAM_TEST_CONNECTED(test_local_dgram_epoll),
      DGRAM_TEST(test_local_dgram_epoll_bind),
      DGRAM_TEST(test_local_dgram_epoll_close),
      DGRAM_TEST(test_local_dgram_poll_bindwait),
      DGRAM_TEST(test_local_dgram_poll_connwait),
      DGRAM_TEST(test_local_dgram_poll_unspecwait),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_closepeer),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_closesend),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_threads),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_slots),
      DGRAM_TEST(test_local_dgram_poll_cycle),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_big),
      DGRAM_TEST(test_local_dgram_poll_abstract),
      DGRAM_TEST(test_local_dgram_poll_self),
      DGRAM_TEST(test_local_dgram_poll_nonblock),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_dup),
      DGRAM_TEST_CONNECTED(test_local_dgram_poll_sendmsg),
      DGRAM_TEST(test_local_dgram_poll_stress),
      DGRAM_TEST(test_local_dgram_fifo_policy),
      DGRAM_TEST(test_local_dgram_reg_inuse),
      DGRAM_TEST(test_local_dgram_reg_names),
      DGRAM_TEST(test_local_dgram_reg_sendrecv),
      DGRAM_TEST(test_local_dgram_reg_source),
      DGRAM_TEST(test_local_dgram_reg_order),
      DGRAM_TEST(test_local_dgram_reg_trunc),
      DGRAM_TEST(test_local_dgram_reg_zero),
      DGRAM_TEST(test_local_dgram_reg_big),
      DGRAM_TEST(test_local_dgram_reg_dontwait),
      DGRAM_TEST(test_local_dgram_reg_peek),
      DGRAM_TEST(test_local_dgram_reg_missing),
      DGRAM_TEST(test_local_dgram_reg_unbound),
      DGRAM_TEST(test_local_dgram_reg_self),
      DGRAM_TEST(test_local_dgram_reg_msg),
      DGRAM_TEST(test_local_dgram_reg_cycle),
      DGRAM_TEST(test_local_dgram_stream_names),
      DGRAM_TEST(test_local_dgram_stream_unspec),
      DGRAM_TEST(test_local_dgram_stream_threads),
      DGRAM_TEST(test_local_dgram_stream_hup),
      DGRAM_TEST(test_local_dgram_stream_listenclose),
      DGRAM_TEST(test_local_dgram_pipe_hup),
      DGRAM_TEST(test_local_dgram_fifo_hup),
      DGRAM_TEST(test_local_dgram_pair_send),
      DGRAM_TEST(test_local_dgram_pair_full),
      DGRAM_TEST(test_local_dgram_pair_close),
      DGRAM_TEST(test_local_dgram_poll_pair),
      DGRAM_TEST(test_local_dgram_recv_intr),
      DGRAM_TEST(test_local_dgram_stream_intr),
    };

  if (argc > 1)
    {
      g_local_dgram_dir = argv[1];
    }

  if (argc > 2)
    {
      cmocka_set_test_filter(argv[2]);
    }

  return cmocka_run_group_tests(local_dgram_tests,
                                test_local_dgram_group_setup,
                                test_local_dgram_group_teardown);
}
