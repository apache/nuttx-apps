/****************************************************************************
 * apps/testing/libc/printfengine/printfengine_main.c
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
#include <float.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CHECK(want, fmt, ...) \
  check_str(want, fmt, nofmt(g_buf, sizeof(g_buf), fmt, ##__VA_ARGS__))

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_buf[128];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* It has no format attribute: the compiler warns about some formats */

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
      printf("printfengine: ERROR: \"%s\" returned %d: \"%s\", expected "
             "\"%s\"\n", fmt, ret, g_buf, want);
      ASSERT(false);
    }
}

/* A value from its bits.  An operation such as 0.0 / 0.0 gives a NaN with
 * the sign that the CPU selects, and a CPU can flush a subnormal result.
 */

static double make_double(uint64_t bits64, uint32_t bits32)
{
  double value;

#if DBL_MANT_DIG == 53
  UNUSED(bits32);
  memcpy(&value, &bits64, sizeof(value));
#else
  UNUSED(bits64);
  memcpy(&value, &bits32, sizeof(value));
#endif
  return value;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Ties of values that are exact in binary, and %.0f rounds */

  CHECK("0", "%.0f", 0.5);
  CHECK("2", "%.0f", 1.5);
  CHECK("2", "%.0f", 2.5);
  CHECK("4", "%.0f", 3.5);
  CHECK("14", "%.0f", 13.5);
  CHECK("2.2", "%.1f", 2.25);
  CHECK("0.1", "%.1f", 0.125);
  CHECK("1.0e+02", "%.1e", 99.5);
  CHECK("2e+06", "%.0e", 2.5e6);

  /* Rounding and the form of %g */

  CHECK("0.00", "%.2f", 0.001);
  CHECK("0.01", "%.2f", 0.006);
  CHECK("10.00", "%.2f", 9.999);
  CHECK("1e+20", "%g", 1e20);
  CHECK("0.0001", "%g", 0.0001);
  CHECK("1e-05", "%g", 0.00001);
  CHECK("100000", "%g", 100000.0);
  CHECK("1e+06", "%g", 1000000.0);
  CHECK("3.14159", "%g", 3.14159265358979);

  /* Sign, infinity and NaN */

  CHECK("-0.000000", "%f", -0.0);
  CHECK("inf|-inf", "%f|%f",
        make_double(0x7ff0000000000000ull, 0x7f800000u),
        make_double(0xfff0000000000000ull, 0xff800000u));
  CHECK("nan", "%f", make_double(0x7ff8000000000000ull, 0x7fc00000u));

#if DBL_MANT_DIG == 53
  /* The limits of binary64, and 17 digits are correct in all builds */

  CHECK("1.797693e+308", "%e", DBL_MAX);
  CHECK("2.225074e-308", "%e", DBL_MIN);
  CHECK("4.940656e-324", "%e", make_double(1, 1));
  CHECK("1.0000000000000001e-01", "%.16e", 0.1);
  CHECK("3.3333333333333331e-01", "%.16e", 1.0 / 3.0);

#  ifdef CONFIG_LIBC_PRINTF_EXACT
  /* All digits */

  CHECK("0.10000000000000000555", "%.20f", 0.1);
  CHECK("0.1000000000000000055511151231257827021181583404541015625",
        "%.55f", 0.1);
  CHECK("9223372036854775808", "%.0f", 9223372036854775808.0);
  CHECK("10000000000000000000000.000000", "%f", 1e22);
  CHECK("99999999999999991611392.000000", "%f", 1e23);
  CHECK("9.99999999999999916e+22", "%.17e", 1e23);
  CHECK("1.79769313486231570815e+308", "%.20e", DBL_MAX);
  CHECK("0.1000000000000000055511151231257827", "%.34g", 0.1);
  CHECK("1.5", "%.40g", 1.5);
#  endif
#endif

#if defined(CONFIG_LIBC_PRINTF_EXACT_LDOUBLE) && \
    (LDBL_MANT_DIG == 113 || LDBL_MANT_DIG == 64)
  /* The range of long double, and more digits than a double has */

  CHECK("1.5", "%Lg", 1.5L);
  CHECK("1.189731e+4932", "%Le", LDBL_MAX);
  CHECK("3.362103e-4932", "%Le", LDBL_MIN);
#  if LDBL_MANT_DIG == 113
  CHECK("1.00000000000000000000000000000000019", "%.36Lg",
        1.0L + LDBL_EPSILON);
#  endif
#endif

  printf("printfengine: PASS\n");
  return 0;
}
