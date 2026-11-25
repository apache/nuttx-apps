/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp.c
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

#include "test_tcp.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest tcp_tests[] =
    {
#ifdef CONFIG_NET_IPv4
#  ifdef CONFIG_NET_SOCKOPTS
      /* First: after the other cases, the devif callbacks that this case
       * needs at once may not be free (NET_PREALLOC_DEVIF_CALLBACKS).
       */

      cmocka_unit_test_setup_teardown(test_tcp_loopback_bulk,
                                      test_tcp_bulk_setup,
                                      test_tcp_bulk_teardown),
#  endif
      cmocka_unit_test_setup_teardown(test_tcp_closewait_accept_data,
                                      test_tcp_closewait_setup,
                                      test_tcp_closewait_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_closewait_accept,
                                      test_tcp_closewait_setup,
                                      test_tcp_closewait_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_closewait_accept_half,
                                      test_tcp_closewait_setup,
                                      test_tcp_closewait_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_closewait_connect,
                                      test_tcp_closewait_setup,
                                      test_tcp_closewait_teardown),
#  ifdef CONFIG_NET_SOLINGER
      cmocka_unit_test_setup_teardown(test_tcp_closewait_rst_accept,
                                      test_tcp_closewait_setup,
                                      test_tcp_closewait_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_closewait_rst_accept_data,
                                      test_tcp_closewait_setup,
                                      test_tcp_closewait_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_closewait_rst_connect,
                                      test_tcp_closewait_setup,
                                      test_tcp_closewait_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_closewait_rst_race,
                                      test_tcp_closewait_setup,
                                      test_tcp_closewait_teardown),
#  endif
#endif
#ifdef CONFIG_NET_IPv4
      cmocka_unit_test_setup_teardown(test_tcp_connect_ipv4,
                                      test_tcp_connect_ipv4_setup,
                                      test_tcp_common_teardown),
#  ifdef CONFIG_NET_SOCKOPTS
      cmocka_unit_test_setup_teardown(test_tcp_send_limit_wakeup,
                                      test_tcp_sndlimit_setup,
                                      test_tcp_sndlimit_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_bind_reuseaddr_timewait,
                                      test_tcp_bind_listen_setup,
                                      test_tcp_bind_listen_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_bind_reuseaddr_connected,
                                      test_tcp_bind_listen_setup,
                                      test_tcp_bind_listen_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_bind_reuseaddr_listener,
                                      test_tcp_bind_listen_setup,
                                      test_tcp_bind_listen_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_accept_inherit,
                                      test_tcp_bind_listen_setup,
                                      test_tcp_bind_listen_teardown),
#    ifdef CONFIG_NET_TCP_WRITE_BUFFERS
      cmocka_unit_test_setup_teardown(test_tcp_sndtimeo_iob,
                                      test_tcp_sndtimeo_setup,
                                      test_tcp_sndtimeo_teardown),
#    endif
#  endif
      cmocka_unit_test_setup_teardown(test_tcp_listen_again,
                                      test_tcp_bind_listen_setup,
                                      test_tcp_bind_listen_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_listen_recv,
                                      test_tcp_bind_listen_setup,
                                      test_tcp_bind_listen_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_listen_connected,
                                      test_tcp_bind_listen_setup,
                                      test_tcp_bind_listen_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_close_shutdown_queued,
                                      test_tcp_close_setup,
                                      test_tcp_close_teardown),
#  ifdef CONFIG_NET_SOLINGER
      cmocka_unit_test_setup_teardown(test_tcp_close_shutdown_close,
                                      test_tcp_close_setup,
                                      test_tcp_close_teardown),
#  endif
#  if defined(CONFIG_NET_LOOPBACK) && defined(CONFIG_NET_SOLINGER)
      cmocka_unit_test_setup_teardown(test_tcp_close_netdown,
                                      test_tcp_close_setup,
                                      test_tcp_close_teardown),
#  endif
      cmocka_unit_test_setup_teardown(test_tcp_close_unread,
                                      test_tcp_close_setup,
                                      test_tcp_close_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_close_timewait,
                                      test_tcp_close_setup,
                                      test_tcp_close_teardown),
#endif
#ifdef CONFIG_NET_IPv6
      cmocka_unit_test_setup_teardown(test_tcp_connect_ipv6,
                                      test_tcp_connect_ipv6_setup,
                                      test_tcp_common_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_peername_ipv6,
                                      test_tcp_peername_setup,
                                      test_tcp_peername_teardown),
#endif
#ifdef CONFIG_NET_IPv4
      cmocka_unit_test_setup_teardown(test_tcp_reset_recv,
                                      test_tcp_reset_setup,
                                      test_tcp_reset_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_reset_recv_wait,
                                      test_tcp_reset_setup,
                                      test_tcp_reset_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_reset_recv_waitall,
                                      test_tcp_reset_setup,
                                      test_tcp_reset_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_reset_send,
                                      test_tcp_reset_setup,
                                      test_tcp_reset_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_reset_poll,
                                      test_tcp_reset_setup,
                                      test_tcp_reset_teardown),
#  ifdef CONFIG_NET_SOCKOPTS
      cmocka_unit_test_setup_teardown(test_tcp_reset_refused,
                                      test_tcp_reset_setup,
                                      test_tcp_reset_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_halfclose_answer,
                                      test_tcp_halfclose_setup,
                                      test_tcp_halfclose_teardown),
#  endif
      cmocka_unit_test_setup_teardown(test_tcp_recv_iov_empty,
                                      test_tcp_iov_setup,
                                      test_tcp_iov_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_recv_iov_nowait,
                                      test_tcp_iov_setup,
                                      test_tcp_iov_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_epollet_in,
                                      test_tcp_epoll_setup,
                                      test_tcp_epoll_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_epollet_out,
                                      test_tcp_epoll_setup,
                                      test_tcp_epoll_teardown),
#  if defined(CONFIG_NET_TCP_WRITE_BUFFERS) && defined(CONFIG_NET_SOCKOPTS)
      cmocka_unit_test_setup_teardown(test_tcp_epollet_eagain,
                                      test_tcp_epoll_setup,
                                      test_tcp_epoll_teardown),
#  endif
#  ifdef CONFIG_NET_TCPBACKLOG
#    ifdef CONFIG_NET_SOLINGER
      cmocka_unit_test_setup_teardown(test_tcp_backlog_relisten_rst,
                                      test_tcp_backlog_setup,
                                      test_tcp_backlog_teardown),
#    endif
      cmocka_unit_test_setup_teardown(test_tcp_backlog_relisten_race,
                                      test_tcp_backlog_setup,
                                      test_tcp_backlog_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_backlog_close_race,
                                      test_tcp_backlog_setup,
                                      test_tcp_backlog_teardown),
#  endif
#endif
#ifdef TEST_TCP_PROCFS
      cmocka_unit_test_setup_teardown(test_tcp_procfs_many,
                                      test_tcp_procfs_setup,
                                      test_tcp_procfs_teardown),
#  ifdef CONFIG_NET_SOLINGER
      cmocka_unit_test_setup_teardown(test_tcp_procfs_close,
                                      test_tcp_procfs_setup,
                                      test_tcp_procfs_teardown),
#  endif
#endif
    };

  return cmocka_run_group_tests(tcp_tests, test_tcp_group_setup,
                                test_tcp_group_teardown);
}
