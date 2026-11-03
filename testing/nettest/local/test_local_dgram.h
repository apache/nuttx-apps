/****************************************************************************
 * apps/testing/nettest/local/test_local_dgram.h
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

#ifndef __APPS_TESTING_NETTEST_LOCAL_TEST_LOCAL_DGRAM_H
#define __APPS_TESTING_NETTEST_LOCAL_TEST_LOCAL_DGRAM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/compiler.h>

#include <stdbool.h>
#include <sys/socket.h>
#include <sys/un.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LOCAL_DGRAM_PATHLEN 96
#define LOCAL_DGRAM_BIGLEN  60000

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct nettest_local_dgram_state_s
{
  char p1[LOCAL_DGRAM_PATHLEN];  /* Name of r1 */
  char p2[LOCAL_DGRAM_PATHLEN];  /* Name of r2 */
  char p3[LOCAL_DGRAM_PATHLEN];  /* Name of r3 */
  char pf[LOCAL_DGRAM_PATHLEN];  /* Name of a regular file */
  char abs[LOCAL_DGRAM_PATHLEN]; /* An abstract name */
  FAR char *big;                 /* LOCAL_DGRAM_BIGLEN bytes */
  int r1;                        /* Receiver bound to p1 */
  int r2;
  int r3;
  int s;                         /* Sender */
  int u;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The directory of the socket names (default P_tmpdir) */

extern FAR const char *g_local_dgram_dir;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: local_dgram_bind
 ****************************************************************************/

int local_dgram_bind(FAR const char *path, bool abstract);

/****************************************************************************
 * Name: local_dgram_connect_to
 ****************************************************************************/

int local_dgram_connect_to(int fd, FAR const char *path, bool abstract);

/****************************************************************************
 * Name: local_dgram_addr
 ****************************************************************************/

socklen_t local_dgram_addr(FAR struct sockaddr_un *addr,
                           FAR const char *path, bool abstract);

/****************************************************************************
 * Name: local_dgram_readable
 ****************************************************************************/

bool local_dgram_readable(int fd, int ms);

/****************************************************************************
 * Name: local_dgram_expect
 ****************************************************************************/

void local_dgram_expect(int fd, FAR const char *want);

/****************************************************************************
 * Name: local_dgram_drain
 ****************************************************************************/

void local_dgram_drain(int fd, FAR char *buf, size_t size);

/****************************************************************************
 * Name: test_local_dgram_group_setup
 ****************************************************************************/

int test_local_dgram_group_setup(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_group_teardown
 ****************************************************************************/

int test_local_dgram_group_teardown(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_setup
 ****************************************************************************/

int test_local_dgram_setup(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_connected_setup
 ****************************************************************************/

int test_local_dgram_connected_setup(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_teardown
 ****************************************************************************/

int test_local_dgram_teardown(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_connect
 ****************************************************************************/

void test_local_dgram_connect(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_peername
 ****************************************************************************/

void test_local_dgram_peername(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_send
 ****************************************************************************/

void test_local_dgram_send(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_write
 ****************************************************************************/

void test_local_dgram_write(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_sendto_null
 ****************************************************************************/

void test_local_dgram_sendto_null(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_peeronly
 ****************************************************************************/

void test_local_dgram_peeronly(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_unspec
 ****************************************************************************/

void test_local_dgram_unspec(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_reconnect
 ****************************************************************************/

void test_local_dgram_reconnect(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_missing
 ****************************************************************************/

void test_local_dgram_missing(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_notsock
 ****************************************************************************/

void test_local_dgram_notsock(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_closed
 ****************************************************************************/

void test_local_dgram_closed(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_abstract
 ****************************************************************************/

void test_local_dgram_abstract(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_pollin0
 ****************************************************************************/

void test_local_dgram_pollin0(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_pollin
 ****************************************************************************/

void test_local_dgram_pollin(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_nohup
 ****************************************************************************/

void test_local_dgram_nohup(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_pollout
 ****************************************************************************/

void test_local_dgram_pollout(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_pollwait
 ****************************************************************************/

void test_local_dgram_pollwait(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_select
 ****************************************************************************/

void test_local_dgram_select(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_dontwait
 ****************************************************************************/

void test_local_dgram_dontwait(FAR void **state);

/****************************************************************************
 * Name: test_local_dgram_nonblock
 ****************************************************************************/

void test_local_dgram_nonblock(FAR void **state);

#endif /* __APPS_TESTING_NETTEST_LOCAL_TEST_LOCAL_DGRAM_H */
