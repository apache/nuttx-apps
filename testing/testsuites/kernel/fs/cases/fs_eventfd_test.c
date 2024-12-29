/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_eventfd_test.c
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
#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <sys/eventfd.h>
#include "fstest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: threadfunc
 ****************************************************************************/

__attribute__((unused)) static void *threadfunc(void *args)
{
  eventfd_t eventfd01_buffer;
  int fd = *(int *)args;

  for (int i = 1; i < 6; i++)
    {
      read(fd, &eventfd01_buffer, sizeof(eventfd_t));
      sleep(1);
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_eventfd
 ****************************************************************************/

void test_nuttx_fs_eventfd(FAR void **state)
{
#ifdef CONFIG_EVENT_FD
  eventfd_t eventfd01_buf = 1;
  int eventfd01_ret;
  int eventfd01_efd;
  pthread_t eventfd01_tid;
  struct fs_testsuites_state_s *test_state;
  test_state = (struct fs_testsuites_state_s *)*state;

  eventfd01_efd = eventfd(0, 0);
  assert_int_not_equal(eventfd01_efd, -1);
  test_state->fd1 = eventfd01_efd;
  assert_true(pthread_create(&eventfd01_tid, NULL, threadfunc,
                             &eventfd01_efd) >= 0);

  for (int i = 1; i < 5; i++)
    {
      eventfd01_ret =
          write(eventfd01_efd, &eventfd01_buf, sizeof(eventfd_t));
      assert_int_equal(eventfd01_ret, sizeof(eventfd_t));
      eventfd01_buf++;
      sleep(1);
    }

  sleep(2);
#endif
}
