/****************************************************************************
 * apps/testing/nettest/radio/test_radio_common.c
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

#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cmocka.h>

#include "test_radio.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_radio_recv_nodev
 ****************************************************************************/

void test_radio_recv_nodev(int fd)
{
  struct sockaddr_storage from;
  socklen_t fromlen;
  char buf[16];
  int i;

  /* The socket lock must stay usable after each failed call */

  for (i = 0; i < 3; i++)
    {
      fromlen = sizeof(from);
      assert_int_equal(recvfrom(fd, buf, sizeof(buf), 0,
                                (FAR struct sockaddr *)&from, &fromlen),
                       -1);
      assert_int_equal(errno, ENODEV);
    }

  assert_return_code(close(fd), errno);
}
