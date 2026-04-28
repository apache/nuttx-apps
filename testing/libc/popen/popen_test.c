/****************************************************************************
 * apps/testing/libc/popen/popen_test.c
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int test_popen_read(void)
{
  FILE *fp;
  char buf[256];
  int found = 0;

  printf("[TEST 1] popen read...\n");
  fp = popen("popen_test --echo hello_popen", "r");
  if (fp == NULL)
    {
      printf("[FAIL] popen returned NULL\n");
      return 1;
    }

  while (fgets(buf, sizeof(buf), fp) != NULL)
    {
      printf("  > %s", buf);
      if (strstr(buf, "hello_popen"))
        {
          found = 1;
        }
    }

  pclose(fp);
  printf("%s\n", found ? "[PASS]" : "[FAIL] missing output");
  return !found;
}

static int test_popen_multiarg(void)
{
  FILE *fp;
  char buf[256];
  int found = 0;

  printf("[TEST 2] popen multi-arg...\n");
  fp = popen("popen_test --echo aaa bbb ccc", "r");
  if (fp == NULL)
    {
      printf("[FAIL] popen returned NULL\n");
      return 1;
    }

  while (fgets(buf, sizeof(buf), fp) != NULL)
    {
      printf("  > %s", buf);
      if (strstr(buf, "aaa bbb ccc"))
        {
          found = 1;
        }
    }

  pclose(fp);
  printf("%s\n", found ? "[PASS]" : "[FAIL] wrong output");
  return !found;
}

static int test_pclose_complete(void)
{
  FILE *fp;
  char buf[256];

  printf("[TEST 3] pclose completes...\n");
  fp = popen("popen_test --echo ok", "r");
  if (fp == NULL)
    {
      printf("[FAIL] popen returned NULL\n");
      return 1;
    }

  while (fgets(buf, sizeof(buf), fp) != NULL);
  pclose(fp);
  printf("[PASS]\n");
  return 0;
}

static int test_popen_invalid_mode(void)
{
  FILE *fp;

  printf("[TEST 4] popen invalid mode...\n");
  fp = popen("popen_test", "x");
  if (fp == NULL)
    {
      printf("[PASS]\n");
      return 0;
    }

  pclose(fp);
  printf("[FAIL] should return NULL\n");
  return 1;
}

static int test_dpopen_read(void)
{
  char buf[256];
  ssize_t n;
  pid_t pid;
  int found = 0;
  int fd;

  printf("[TEST 5] dpopen read...\n");
  fd = dpopen("popen_test --echo hello_dpopen", O_RDONLY, &pid);
  if (fd < 0)
    {
      printf("[FAIL] dpopen returned %d\n", fd);
      return 1;
    }

  while ((n = read(fd, buf, sizeof(buf) - 1)) > 0)
    {
      buf[n] = '\0';
      printf("  > %s", buf);
      if (strstr(buf, "hello_dpopen"))
        {
          found = 1;
        }
    }

  dpclose(fd, pid);
  printf("%s\n", found ? "[PASS]" : "[FAIL] missing output");
  return !found;
}

static int test_dpopen_multiarg(void)
{
  char buf[256];
  ssize_t n;
  pid_t pid;
  int found = 0;
  int fd;

  printf("[TEST 6] dpopen multi-arg...\n");
  fd = dpopen("popen_test --echo xxx yyy zzz", O_RDONLY, &pid);
  if (fd < 0)
    {
      printf("[FAIL] dpopen returned %d\n", fd);
      return 1;
    }

  while ((n = read(fd, buf, sizeof(buf) - 1)) > 0)
    {
      buf[n] = '\0';
      printf("  > %s", buf);
      if (strstr(buf, "xxx yyy zzz"))
        {
          found = 1;
        }
    }

  dpclose(fd, pid);
  printf("%s\n", found ? "[PASS]" : "[FAIL] wrong output");
  return !found;
}

static int test_dpclose_complete(void)
{
  char buf[256];
  pid_t pid;
  int fd;

  printf("[TEST 7] dpclose completes...\n");
  fd = dpopen("popen_test --echo ok", O_RDONLY, &pid);
  if (fd < 0)
    {
      printf("[FAIL] dpopen returned %d\n", fd);
      return 1;
    }

  while (read(fd, buf, sizeof(buf)) > 0);
  dpclose(fd, pid);
  printf("[PASS]\n");
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fail = 0;

  /* Sub-command: echo args back, used as popen/dpopen target */

  if (argc > 1 && strcmp(argv[1], "--echo") == 0)
    {
      int i;
      for (i = 2; i < argc; i++)
        {
          printf("%s%s", i > 2 ? " " : "", argv[i]);
        }

      printf("\n");
      return 0;
    }

  printf("=== popen/dpopen test ===\n\n");

  /* popen tests */

  fail += test_popen_read();
  fail += test_popen_multiarg();
  fail += test_pclose_complete();
  fail += test_popen_invalid_mode();

  /* dpopen tests */

  fail += test_dpopen_read();
  fail += test_dpopen_multiarg();
  fail += test_dpclose_complete();

  printf("\n=== Results: %d failed ===\n", fail);
  return fail;
}
