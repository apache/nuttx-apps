/****************************************************************************
 * examples/usrsocktest/usrsocktest_main.c
 *
 *   Copyright (C) 2015, 2017 Haltian Ltd. All rights reserved.
 *    Author: Jussi Kivilinna <jussi.kivilinna@haltian.com>
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

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
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
#ifdef CONFIG_CAN_PASS_STRUCTS
  *mem = mallinfo();
#else
  (void)mallinfo(mem);
#endif
}

static void print_mallinfo(const struct mallinfo *mem, const char *title)
{
  if (title)
    printf("%s:\n", title);
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
 * Name: runAllTests
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

static void runAllTests(void)
{
  RUN_TEST_GROUP(CharDev);
  RUN_TEST_GROUP(NoDaemon);
  RUN_TEST_GROUP(BasicDaemon);
  RUN_TEST_GROUP(BasicConnect);
  RUN_TEST_GROUP(BasicConnectDelay);
  RUN_TEST_GROUP(NoBlockConnect);
  RUN_TEST_GROUP(BasicSend);
  RUN_TEST_GROUP(NoBlockSend);
  RUN_TEST_GROUP(BlockSend);
  RUN_TEST_GROUP(NoBlockRecv);
  RUN_TEST_GROUP(BlockRecv);
  RUN_TEST_GROUP(RemoteDisconnect);
  RUN_TEST_GROUP(BasicSetSockOpt);
  RUN_TEST_GROUP(BasicGetSockOpt);
  RUN_TEST_GROUP(BasicGetSockName);
  RUN_TEST_GROUP(WakeWithSignal);
  RUN_TEST_GROUP(MultiThread);
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

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int usrsocktest_main(int argc, char *argv[])
#endif
{
  struct mallinfo mem_before, mem_after;

  memset(&overall, 0, sizeof(overall));

  printf("Starting unit-tests...\n");
  fflush(stdout);
  fflush(stderr);

  get_mallinfo(&mem_before);

  runAllTests();

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

