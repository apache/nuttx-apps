/****************************************************************************
 * apps/examples/usrsocktest/usrsocktest_main.c
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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <assert.h>
#include <debug.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "defines.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

#ifndef dbg
  #define dbg _warn
#endif

#define usrsocktest_dbg(...) ((void)0)

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct
{
  unsigned int ok;
  unsigned int failed;

  unsigned int nchecks;
} overall;

/****************************************************************************
 * Public Data
 ****************************************************************************/

int usrsocktest_endp_malloc_cnt = 0;
int usrsocktest_dcmd_malloc_cnt = 0;
bool usrsocktest_test_failed = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void get_mallinfo(struct mallinfo *mem)
{
  *mem = mallinfo();
}

static void print_mallinfo(const struct mallinfo *mem, const char *title)
{
  if (title)
    {
      printf("%s:\n", title);
    }

  printf("       %11s%11s%11s%11s\n", "total", "used", "free", "largest");
  printf("Mem:   %11d%11d%11d%11d\n",
         mem->arena, mem->uordblks, mem->fordblks, mem->mxordblk);
}

static void utest_assert_print_head(FAR const char *func, const int line,
                                    FAR const char *check_str)
{
  printf("\t[TEST ASSERT FAILED!]\n"
         "\t\tIn function \"%s\":\n"
         "\t\tline %d: Assertion `%s' failed.\n", func, line, check_str);
}

static void run_tests(FAR const char *name, void (CODE *test_fn)(void))
{
  printf("Testing group \"%s\" =>\n", name);
  fflush(stdout);
  fflush(stderr);

  usrsocktest_test_failed = false;
  test_fn();
  if (!usrsocktest_test_failed)
    {
      printf("\tGroup \"%s\": [OK]\n", name);
      overall.ok++;
    }
  else
    {
      printf("\tGroup \"%s\": [FAILED]\n", name);
      overall.failed++;
    }

  fflush(stdout);
  fflush(stderr);
}

/****************************************************************************
 * Name: run_all_tests
 *
 * Description:
 *   Sequentially runs all included test groups
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

static void run_all_tests(void)
{
  RUN_TEST_GROUP(char_dev);
  RUN_TEST_GROUP(no_daemon);
  RUN_TEST_GROUP(basic_daemon);
  RUN_TEST_GROUP(basic_connect);
  RUN_TEST_GROUP(basic_connect_delay);
  RUN_TEST_GROUP(no_block_connect);
  RUN_TEST_GROUP(basic_send);
  RUN_TEST_GROUP(no_block_send);
  RUN_TEST_GROUP(block_send);
  RUN_TEST_GROUP(no_block_recv);
  RUN_TEST_GROUP(block_recv);
  RUN_TEST_GROUP(remote_disconnect);
  RUN_TEST_GROUP(basic_setsockopt);
  RUN_TEST_GROUP(basic_getsockopt);
  RUN_TEST_GROUP(basic_getsockname);
  RUN_TEST_GROUP(wake_with_signal);
  RUN_TEST_GROUP(multithread);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool usrsocktest_assert_print_value(FAR const char *func,
                                    const int line,
                                    FAR const char *check_str,
                                    long int test_value,
                                    long int should_be)
{
  ++overall.nchecks;

  if (test_value == should_be)
    {
      int keep_errno = errno;

      usrsocktest_dbg("%d => OK.\n", line);

      errno = keep_errno;
      return true;
    }

  utest_assert_print_head(func, line, check_str);

  printf("\t\t\tgot value: %ld\n", test_value);
  printf("\t\t\tshould be: %ld\n", should_be);

  fflush(stdout);
  fflush(stderr);

  return false;
}

bool usrsocktest_assert_print_buf(FAR const char *func,
                                  const int line,
                                  FAR const char *check_str,
                                  FAR const void *test_buf,
                                  FAR const void *expect_buf,
                                  size_t buflen)
{
  ++overall.nchecks;

  if (memcmp(test_buf, expect_buf, buflen) == 0)
    {
      int keep_errno = errno;

      usrsocktest_dbg("%d => OK.\n", line);

      errno = keep_errno;
      return true;
    }

  utest_assert_print_head(func, line, check_str);

  fflush(stdout);
  fflush(stderr);

  return false;
}

/****************************************************************************
 * usrsocktest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct mallinfo mem_before;
  struct mallinfo mem_after;

  memset(&overall, 0, sizeof(overall));

  printf("Starting unit-tests...\n");
  fflush(stdout);
  fflush(stderr);

  get_mallinfo(&mem_before);

  run_all_tests();

  printf("Unit-test groups done... OK:%d, FAILED:%d, TOTAL:%d\n",
         overall.ok, overall.failed, overall.ok + overall.failed);
  printf(" -- number of checks made: %d\n", overall.nchecks);
  fflush(stdout);
  fflush(stderr);

  get_mallinfo(&mem_after);

  print_mallinfo(&mem_before, "HEAP BEFORE TESTS");
  print_mallinfo(&mem_after, "HEAP AFTER TESTS");

  fflush(stdout);
  fflush(stderr);
  exit(0);

  return 0;
}
