/****************************************************************************
 * apps/testing/libc/printftest/printftest_main.c
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

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Each string case ends with a string argument: an argument that printf
 * misreads shows in the text, or crashes the test.
 */

#define CHECK(want, fmt, ...) \
  check_str(want, fmt, nofmt(g_buf, sizeof(g_buf), fmt, ##__VA_ARGS__))

#define CHECKLEN(want, fmt, ...) \
  check_ret(want, fmt, nofmt(g_buf, sizeof(g_buf), fmt, ##__VA_ARGS__))

#ifdef CONFIG_LIBC_FLOATINGPOINT
#  define FLT(on, off) on
#else
#  define FLT(on, off) off
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_buf[512];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* It has no format attribute: the compiler warns about some formats below. */

static int nofmt(FAR char *buf, size_t size, FAR const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start(ap, fmt);
  ret = vsnprintf(buf, size, fmt, ap);
  va_end(ap);
  return ret;
}

static void check_str(FAR const char *want, FAR const char *fmt, int ret)
{
  if (ret != (int)strlen(want) || strcmp(g_buf, want) != 0)
    {
      printf("printftest: ERROR: \"%s\" returned %d: \"%s\", expected "
             "\"%s\"\n", fmt, ret, g_buf, want);
      ASSERT(false);
    }
}

static void check_ret(int want, FAR const char *fmt, int ret)
{
  if (ret != want)
    {
      printf("printftest: ERROR: \"%s\" returned %d, expected %d\n",
             fmt, ret, want);
      ASSERT(false);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  long long ll = 77;
  signed char hh = 77;
  int n = 77;

  /* A conversion that is not available takes its argument */

  CHECK("?|str", "%a|%s", 1.5, "str");
  CHECK("?|str", "%A|%s", 1.5, "str");
  CHECK("    ?|?    |str", "%5a|%-5A|%s", 1.5, 2.5, "str");
  CHECK("?|2|s|3|?|5", "%a|%ld|%s|%ld|%a|%ld", 1.0, 2L, "s", 3L, 4.0, 5L);
  CHECK("?|str", "%La|%s", (long double)1.5, "str");
  CHECK(FLT("1.5|str", "?|str"), "%Lg|%s", (long double)1.5, "str");
  CHECK(FLT("1.500000|str", "?|str"), "%f|%s", 1.5, "str");

  /* %n of any length takes its pointer, stores nothing, prints "?" */

  CHECK("ab?|str", "ab%n|%s", &n, "str");
  CHECK("ab?|str", "ab%lln|%s", &ll, "str");
  CHECK("ab?|str", "ab%hhn|%s", &hh, "str");
  ASSERT(n == 77 && ll == 77 && hh == 77);

  /* An unknown conversion takes no argument: the rest is text.  A newer
   * format that printf does not know yet is the same case.
   */

  CHECK("%y|%d|%s", "%y|%d|%s", 1, "str");
  CHECK("1%Y%s", "%d%Y%s", 1, "str");
  CHECK("%w64d|%s", "%w64d|%s", 1LL, "str");
  CHECK("%w32d|%s", "%w32d|%s", 1, "str");
  CHECK("%Hf|%s", "%Hf|%s", 1.5, "str");

  /* The end of the format */

  CHECK("abc%", "abc%");
  CHECK("abc%", "abc%5");
  CHECK("abc%", "abc%l");

  /* Width and precision */

  CHECKLEN(300, "%300d", 1);
  CHECKLEN(4095, "%4095d", 1);
  CHECKLEN(300, "%.300d", 1);
  CHECK("abc|", "%.99999999999s|", "abc");
  CHECK("1||", "%.*d|%.*s|", INT_MIN, 1, INT_MIN, "ab");

  errno = 0;
  CHECKLEN(-1, "%#.2147483647x", 1);
  ASSERT(errno == EOVERFLOW);

#ifdef CONFIG_LIBC_FLOATINGPOINT
  CHECKLEN(-1, "%.2147483647f", 1.5);
  CHECK("2", "%.0g", 1.5);
  CHECK("1.5", "%.256g", 1.5);
  CHECK("    1.0e+100|", "%12.1e|", 1e100);
  CHECKLEN(261, "%.255e", 1.0);
  ASSERT(g_buf[0] == '1');
#endif

  /* No digits */

  CHECK("||", "%#.0x|%#.0X|", 0, 0);
  CHECK("0|", "%#.0o|", 0);

#ifdef CONFIG_LIBC_NUMBERED_ARGS
  /* Numbered arguments */

  CHECK("str?", "%2$s%1$n", &n, "str");
  CHECK("str ff FF 10", "%3$s %1$x %2$X %4$o", 255, 255, "str", 8);
  CHECK("str ?", "%2$s %1$a", 1.5, "str");
  CHECK("str ?", "%2$s %1$La", (long double)1.5, "str");
  CHECK("1    |", "%2$*1$d|", -5, 1);
  CHECK("[", "[%3$s %1$d]", 1, 2, "str");
  CHECK("[", "[%99999999999$d]", 1);
  ASSERT(n == 77);
#endif

  printf("printftest: PASS\n");
  return 0;
}
