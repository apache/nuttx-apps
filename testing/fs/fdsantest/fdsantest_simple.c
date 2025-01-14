/****************************************************************************
 * apps/testing/fs/fdsantest/fdsantest_simple.c
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

#include <android/fdsan.h>
#include <fcntl.h>
#include <stdio.h>
#include <cmocka.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void test_case_fdsan_unowned_untagged_close(void **state)
{
  int fd;

  fd = open("/dev/null", O_RDONLY);
  assert_int_equal(0, close(fd));
}

static void test_case_unowned_tagged_close(void **state)
{
  int fd;

  fd = open("/dev/null", O_RDONLY);
  assert_int_equal(0, android_fdsan_close_with_tag(fd, 0));
}

static void test_case_owned_tagged_close(void **state)
{
  int fd;

  fd = open("/dev/null", O_RDONLY);

  android_fdsan_exchange_owner_tag(fd, 0, 0xdeadbeef);
  assert_int_equal(0, android_fdsan_close_with_tag(fd, 0xdeadbeef));
}

static void test_case_overflow(void **state)
{
  uint64_t fds[1000];
  int i;
  int fd;
  int open_count;
  int close_count;
  uint64_t tag;

  open_count = 240;
  close_count = 0;
  memset(fds, 0, sizeof(fds));
  for (i = 0; i < open_count; ++i)
    {
      fd = open("/dev/null", O_RDONLY);
      tag = 0xdead00000000ull | i;
      android_fdsan_exchange_owner_tag(fd, 0, tag);
      assert_in_range(fd, 3, 999);
      fds[fd] = tag;
    }

  for (fd = 3; fd < 1000; fd++)
    {
      if (fds[fd] != 0)
        {
          android_fdsan_close_with_tag(fd, fds[fd]);
          close_count++;
        }
    }

  assert_int_equal(open_count, close_count);
}

static void test_case_vfork(void **state)
{
  int fd = open("/dev/null", O_RDONLY);
  android_fdsan_exchange_owner_tag(fd, 0, 0xbadc0de);

  pid_t rc = vfork();
  assert_int_not_equal(-1, rc);

  if (rc == 0)
    {
      close(fd);
      _exit(0);
    }

  android_fdsan_close_with_tag(fd, 0xbadc0de);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * fdsantest_simple_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test(test_case_fdsan_unowned_untagged_close),
    cmocka_unit_test(test_case_unowned_tagged_close),
    cmocka_unit_test(test_case_owned_tagged_close),
    cmocka_unit_test(test_case_overflow),
    cmocka_unit_test(test_case_vfork),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
