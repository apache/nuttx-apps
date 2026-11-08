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
#include <float.h>
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

/* What a correct call prints depends on the options of printf */

#ifdef CONFIG_LIBC_FLOATINGPOINT
#  define FLT(on, off) on
#else
#  define FLT(on, off) off
#endif

#ifdef CONFIG_LIBC_PRINTF_PERCENT_N
#  define PCN(on, off) on
#else
#  define PCN(on, off) off
#endif

#ifdef CONFIG_LIBC_PRINTF_HEXFLOAT
#  define HXF(on, off) on
#else
#  define HXF(on, off) off
#endif

#ifdef CONFIG_LIBC_PRINTF_C23
#  define C23(on, off) on
#else
#  define C23(on, off) off
#endif

#ifdef CONFIG_LIBC_PRINTF_ALT_FORM
#  define ALT(on, off) on
#else
#  define ALT(on, off) off
#endif

#ifdef CONFIG_LIBC_PRINTF_OCTAL
#  define OCT(on, off) on
#else
#  define OCT(on, off) off
#endif

#ifdef CONFIG_LIBC_PRINTF_LEFT_JUSTIFY
#  define LJT(on, off) on
#else
#  define LJT(on, off) off
#endif

#ifdef CONFIG_LIBC_PRINTF_INT_PRECISION
#  define PRC(on, off) on
#else
#  define PRC(on, off) off
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

  CHECK(HXF("0x1.8p+0", "?") "|str", "%a|%s", 1.5, "str");
  CHECK(HXF("0X1.8P+0", "?") "|str", "%A|%s", 1.5, "str");
  CHECK(HXF("0x1.8p+0|0X1.4P+1", LJT("    ?|?    ", "    ?|    ?")) "|str",
        "%5a|%-5A|%s", 1.5, 2.5, "str");
  CHECK(HXF("0x1p+0|2|s|3|0x1p+2|5", "?|2|s|3|?|5"),
        "%a|%ld|%s|%ld|%a|%ld", 1.0, 2L, "s", 3L, 4.0, 5L);
  CHECK(HXF("0x1.8p+0", "?") "|str", "%La|%s", (long double)1.5, "str");
  CHECK(FLT("1.5|str", "?|str"), "%Lg|%s", (long double)1.5, "str");
  CHECK(FLT("1.500000|str", "?|str"), "%f|%s", 1.5, "str");

  /* %n of any length takes its pointer and prints nothing.  Off: it stores
   * nothing.  On: it stores the count at its size.
   */

  CHECK("ab|str", "ab%n|%s", &n, "str");
  ASSERT(n == PCN(2, 77));
  CHECK("abc|str", "abc%lln|%s", &ll, "str");
  ASSERT(ll == PCN(3, 77));
  CHECK("abcd|str", "abcd%hhn|%s", &hh, "str");
  ASSERT(hh == PCN(4, 77));
  n = 77;
  check_ret(6, "ab%n|%s", nofmt(NULL, 0, "ab%n|%s", &n, "str"));
  ASSERT(n == PCN(2, 77));

  /* An unknown conversion takes no argument: the rest is text.  A format
   * of a feature that is off is the same case.
   */

  CHECK("%y|%d|%s", "%y|%d|%s", 1, "str");
  CHECK("1%Y%s", "%d%Y%s", 1, "str");
  CHECK("%w7d|%d", "%w7d|%d", 1, 2);
  CHECK(C23("5|str", "%w32d|%s"), "%w32d|%s", 5, "str");
  CHECK(C23("101|str", "%b|%s"), "%b|%s", 5, "str");
  CHECK("%Hf|%s", "%Hf|%s", 1.5, "str");

  /* The end of the format */

  CHECK("abc%", "abc%");
  CHECK("abc%", "abc%5");
  CHECK("abc%", "abc%l");

  /* Width and precision */

  CHECKLEN(300, "%300d", 1);
  CHECKLEN(4095, "%4095d", 1);
  CHECKLEN(PRC(300, 1), "%.300d", 1);
  CHECK("abc|", "%.99999999999s|", "abc");
  CHECK("1|ab|", "%.*d|%.*s|", INT_MIN, 1, INT_MIN, "ab");
  CHECK("0|abcdef|", "%.*d|%.*s|", -1, 0, -2, "abcdef");
  CHECK("000042|", "%0*.*d|", 6, -1, 42);

#ifdef CONFIG_LIBC_PRINTF_INT_PRECISION
  errno = 0;
  CHECKLEN(-1, "%#.2147483647x", 1);
  ASSERT(errno == EOVERFLOW);
#endif

#ifdef CONFIG_LIBC_FLOATINGPOINT
  CHECKLEN(-1, "%.2147483647f", 1.5);
  CHECK("2", "%.0g", 1.5);
  CHECK("1.5", "%.256g", 1.5);
  CHECK("    1.0e+100|", "%12.1e|", 1e100);
  CHECKLEN(261, "%.255e", 1.0);
  ASSERT(g_buf[0] == '1');

  /* The # flag of %g keeps the zeros.  A negative precision is none. */

  CHECK("-3.750000e+00|", "%*.*e|", 0, -1, -3.75);
  CHECK(ALT("1.00000|1.00000e+06|1.|100000.", "1|1e+06|1|100000"),
        "%#g|%#g|%#.0g|%#g", 1.0, 1e6, 1.0, 1e5);

  /* The sign of a negative zero */

  CHECK("-0.000000|-0.000000e+00|-0|str", "%f|%e|%g|%s", -0.0, -0.0, -0.0,
        "str");
#endif

#ifdef CONFIG_LIBC_PRINTF_HEXFLOAT
  /* %a */

  CHECK("0x0p+0|-0x0p+0|0x1.999999999999ap-4", "%a|%a|%a", 0.0, -0.0, 0.1);
  CHECK("0x2p+0|0x1p+0|0x1.2p+0|0x1.0p+0", "%.0a|%.0a|%.1a|%.1a", 1.5, 1.25,
        1.09375, 1.03125);
  CHECK("  0x1.8p+0|" LJT("0x1.8p+0  ", "  0x1.8p+0")
        "|0x0001.8p+0|+0x1.8p+0",
        "%10a|%-10a|%011a|%+a", 1.5, 1.5, 1.5, 1.5);
  CHECK("0x1.fffffffffffffp+1023|0x0.0000000000001p-1022", "%a|%a",
        DBL_MAX, DBL_MIN * DBL_EPSILON);
  CHECK("inf|-INF", "%a|%A", 1.0 / 0.0, -1.0 / 0.0);
#endif

#ifdef CONFIG_LIBC_PRINTF_C23
  /* %b, %B, wN and wfN.  A length with an unknown size stays as text. */

  CHECK("101|0|       101|" LJT("101       ", "       101")
        "|0000000101|str",
        "%b|%b|%10b|%-10b|%010b|%s", 5, 0, 5, 5, 5, "str");
  CHECK(ALT("0b101|0B101", "101|101") "|str", "%#b|%#B|%s", 5, 5, "str");
  CHECK("4294967295|str", "%w32u|%s", -1, "str");
  CHECK("abc%w", "abc%w");
#endif

  /* No digits */

  CHECK(PRC("||", "?|?|"), "%#.0x|%#.0X|", 0, 0);
  CHECK(OCT(PRC(ALT("0|", "|"), "?|"), "?|"), "%#.0o|", 0);

#ifdef CONFIG_LIBC_NUMBERED_ARGS
  /* Numbered arguments */

  CHECK("str", "%2$s%1$n", &n, "str");
  CHECK("str ff FF " OCT("10", "?"), "%3$s %1$x %2$X %4$o",
        255, 255, "str", 8);
  CHECK(HXF("str 0x1.8p+0", "str ?"), "%2$s %1$a", 1.5, "str");
  CHECK(HXF("str 0x1.8p+0", "str ?"), "%2$s %1$La", (long double)1.5, "str");
  CHECK(LJT("1    |", "    1|"), "%2$*1$d|", -5, 1);
  CHECK("[", "[%3$s %1$d]", 1, 2, "str");
  CHECK("[", "[%99999999999$d]", 1);
  ASSERT(n == PCN(3, 77));
#endif

  printf("printftest: PASS\n");
  return 0;
}
