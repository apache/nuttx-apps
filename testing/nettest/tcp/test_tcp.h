/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp.h
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

#ifndef __APPS_TESTING_NETTEST_TCP_TEST_TCP_H
#define __APPS_TESTING_NETTEST_TCP_TEST_TCP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <pthread.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct nettest_tcp_state_s
{
  int       client_fd;
  pthread_t server_tid;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_group_setup
 ****************************************************************************/

int test_tcp_group_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_group_teardown
 ****************************************************************************/

int test_tcp_group_teardown(FAR void **state);

/****************************************************************************
 * Name: test_tcp_common_teardown
 ****************************************************************************/

int test_tcp_common_teardown(FAR void **state);

#ifdef CONFIG_NET_IPv4
/****************************************************************************
 * Name: test_tcp_closewait_setup
 ****************************************************************************/

int test_tcp_closewait_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_closewait_teardown
 ****************************************************************************/

int test_tcp_closewait_teardown(FAR void **state);

/****************************************************************************
 * Name: test_tcp_closewait_accept_data
 ****************************************************************************/

void test_tcp_closewait_accept_data(FAR void **state);

/****************************************************************************
 * Name: test_tcp_closewait_accept
 ****************************************************************************/

void test_tcp_closewait_accept(FAR void **state);

/****************************************************************************
 * Name: test_tcp_closewait_accept_half
 ****************************************************************************/

void test_tcp_closewait_accept_half(FAR void **state);

/****************************************************************************
 * Name: test_tcp_closewait_connect
 ****************************************************************************/

void test_tcp_closewait_connect(FAR void **state);

#  ifdef CONFIG_NET_SOLINGER
/****************************************************************************
 * Name: test_tcp_closewait_rst_accept
 ****************************************************************************/

void test_tcp_closewait_rst_accept(FAR void **state);

/****************************************************************************
 * Name: test_tcp_closewait_rst_accept_data
 ****************************************************************************/

void test_tcp_closewait_rst_accept_data(FAR void **state);

/****************************************************************************
 * Name: test_tcp_closewait_rst_connect
 ****************************************************************************/

void test_tcp_closewait_rst_connect(FAR void **state);

/****************************************************************************
 * Name: test_tcp_closewait_rst_race
 ****************************************************************************/

void test_tcp_closewait_rst_race(FAR void **state);
#  endif
#endif

/****************************************************************************
 * Name: test_tcp_connect_ipv4_setup
 ****************************************************************************/

#ifdef CONFIG_NET_IPv4
int test_tcp_connect_ipv4_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_connect_ipv4
 ****************************************************************************/

void test_tcp_connect_ipv4(FAR void **state);

/****************************************************************************
 * Name: test_tcp_bind_listen_setup
 ****************************************************************************/

int test_tcp_bind_listen_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_bind_listen_teardown
 ****************************************************************************/

int test_tcp_bind_listen_teardown(FAR void **state);

/****************************************************************************
 * Name: test_tcp_sndlimit_setup
 ****************************************************************************/

#  ifdef CONFIG_NET_SOCKOPTS
int test_tcp_sndlimit_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_sndlimit_teardown
 ****************************************************************************/

int test_tcp_sndlimit_teardown(FAR void **state);

/****************************************************************************
 * Name: test_tcp_send_limit_wakeup
 ****************************************************************************/

void test_tcp_send_limit_wakeup(FAR void **state);
#  endif

/****************************************************************************
 * Name: test_tcp_bind_reuseaddr_timewait
 ****************************************************************************/

#  ifdef CONFIG_NET_SOCKOPTS
void test_tcp_bind_reuseaddr_timewait(FAR void **state);

/****************************************************************************
 * Name: test_tcp_bind_reuseaddr_connected
 ****************************************************************************/

void test_tcp_bind_reuseaddr_connected(FAR void **state);

/****************************************************************************
 * Name: test_tcp_bind_reuseaddr_listener
 ****************************************************************************/

void test_tcp_bind_reuseaddr_listener(FAR void **state);

/****************************************************************************
 * Name: test_tcp_accept_inherit
 ****************************************************************************/

void test_tcp_accept_inherit(FAR void **state);

#    ifdef CONFIG_NET_TCP_WRITE_BUFFERS
/****************************************************************************
 * Name: test_tcp_sndtimeo_setup
 ****************************************************************************/

int test_tcp_sndtimeo_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_sndtimeo_teardown
 ****************************************************************************/

int test_tcp_sndtimeo_teardown(FAR void **state);

/****************************************************************************
 * Name: test_tcp_sndtimeo_iob
 ****************************************************************************/

void test_tcp_sndtimeo_iob(FAR void **state);
#    endif
#  endif

/****************************************************************************
 * Name: test_tcp_listen_again
 ****************************************************************************/

void test_tcp_listen_again(FAR void **state);

/****************************************************************************
 * Name: test_tcp_listen_recv
 ****************************************************************************/

void test_tcp_listen_recv(FAR void **state);

/****************************************************************************
 * Name: test_tcp_listen_connected
 ****************************************************************************/

void test_tcp_listen_connected(FAR void **state);

/****************************************************************************
 * Name: test_tcp_close_setup
 ****************************************************************************/

int test_tcp_close_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_close_teardown
 ****************************************************************************/

int test_tcp_close_teardown(FAR void **state);

/****************************************************************************
 * Name: test_tcp_close_shutdown_queued
 ****************************************************************************/

void test_tcp_close_shutdown_queued(FAR void **state);

/****************************************************************************
 * Name: test_tcp_close_shutdown_close
 ****************************************************************************/

#  ifdef CONFIG_NET_SOLINGER
void test_tcp_close_shutdown_close(FAR void **state);
#  endif

/****************************************************************************
 * Name: test_tcp_close_netdown
 ****************************************************************************/

#  if defined(CONFIG_NET_LOOPBACK) && defined(CONFIG_NET_SOLINGER)
void test_tcp_close_netdown(FAR void **state);
#  endif

/****************************************************************************
 * Name: test_tcp_close_unread
 ****************************************************************************/

void test_tcp_close_unread(FAR void **state);

/****************************************************************************
 * Name: test_tcp_close_timewait
 ****************************************************************************/

void test_tcp_close_timewait(FAR void **state);
#endif

/****************************************************************************
 * Name: test_tcp_connect_ipv6_setup
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
int test_tcp_connect_ipv6_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_connect_ipv6
 ****************************************************************************/

void test_tcp_connect_ipv6(FAR void **state);

/****************************************************************************
 * Name: test_tcp_peername_setup
 ****************************************************************************/

int test_tcp_peername_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_peername_teardown
 ****************************************************************************/

int test_tcp_peername_teardown(FAR void **state);

/****************************************************************************
 * Name: test_tcp_peername_ipv6
 ****************************************************************************/

void test_tcp_peername_ipv6(FAR void **state);
#endif

#ifdef CONFIG_NET_IPv4
/****************************************************************************
 * Name: test_tcp_reset_setup
 ****************************************************************************/

int test_tcp_reset_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_reset_teardown
 ****************************************************************************/

int test_tcp_reset_teardown(FAR void **state);

/****************************************************************************
 * Name: test_tcp_reset_recv
 ****************************************************************************/

void test_tcp_reset_recv(FAR void **state);

/****************************************************************************
 * Name: test_tcp_reset_recv_wait
 ****************************************************************************/

void test_tcp_reset_recv_wait(FAR void **state);

/****************************************************************************
 * Name: test_tcp_reset_recv_waitall
 ****************************************************************************/

void test_tcp_reset_recv_waitall(FAR void **state);

/****************************************************************************
 * Name: test_tcp_reset_send
 ****************************************************************************/

void test_tcp_reset_send(FAR void **state);

/****************************************************************************
 * Name: test_tcp_reset_poll
 ****************************************************************************/

void test_tcp_reset_poll(FAR void **state);

/****************************************************************************
 * Name: test_tcp_reset_refused
 ****************************************************************************/

#  ifdef CONFIG_NET_SOCKOPTS
void test_tcp_reset_refused(FAR void **state);

/****************************************************************************
 * Name: test_tcp_halfclose_setup
 ****************************************************************************/

int test_tcp_halfclose_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_halfclose_teardown
 ****************************************************************************/

int test_tcp_halfclose_teardown(FAR void **state);

/****************************************************************************
 * Name: test_tcp_halfclose_answer
 ****************************************************************************/

void test_tcp_halfclose_answer(FAR void **state);
#  endif

/****************************************************************************
 * Name: test_tcp_iov_setup
 ****************************************************************************/

int test_tcp_iov_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_iov_teardown
 ****************************************************************************/

int test_tcp_iov_teardown(FAR void **state);

/****************************************************************************
 * Name: test_tcp_recv_iov_empty
 ****************************************************************************/

void test_tcp_recv_iov_empty(FAR void **state);

/****************************************************************************
 * Name: test_tcp_recv_iov_nowait
 ****************************************************************************/

void test_tcp_recv_iov_nowait(FAR void **state);
#endif
#endif /* __APPS_TESTING_NETTEST_TCP_TEST_TCP_H */
