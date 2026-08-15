/****************************************************************************
 * apps/testing/libc/arch_libc/arch_libc_bench.c
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

/* Throughput for the string and memory functions a machine directory may
 * override, across a size sweep and every source/destination alignment
 * pair.  A machine implementation usually takes its wide path only when
 * the pointers satisfy some alignment condition, so a single aligned size
 * reports the best case and hides what the rest of the input space costs.
 *
 * Each measurement reports MB/s, which compares across machines, and
 * cycles per byte from perf_gettime(), which does not depend on the
 * timebase being calibrated.  Both come from one timed loop.
 *
 * Two ways a benchmark of these functions measures nothing are defeated
 * here: the compiler treating a pure call with unchanged arguments as loop
 * invariant, countered by laundering the pointers through an asm that
 * claims to change them, and comparison inputs an earlier measurement
 * scribbled on, countered by preparing the buffers before each timed loop.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <nuttx/clock.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BENCH_BUF    (CONFIG_TESTING_ARCH_LIBC_BENCH_BUFSIZE)
#define BENCH_MINMS  (CONFIG_TESTING_ARCH_LIBC_BENCH_MINMS)
#define BENCH_GUARD  64
#define BENCH_BATCH  16

/* perf_gettime() reaches an application only where the C library builds
 * its own copy, or where the application and the kernel are one image.
 * Elsewhere the cycle count is left out and the throughput stands alone.
 */

#if defined(CONFIG_ARCH_HAVE_PERF_EVENTS_USER_ACCESS) || \
    defined(CONFIG_BUILD_FLAT)
#  define BENCH_CYCLES 1
#endif

/* Operations.  A writer stores through the destination pointer, so its
 * destination offset is worth sweeping; a reader only takes one.
 */

#define OP_MEMCPY    0
#define OP_MEMMOVE   1
#define OP_MEMSET    2
#define OP_MEMCMP    3
#define OP_MEMCHR    4
#define OP_STRLEN    5
#define OP_STRCMP    6
#define OP_STRNCMP   7
#define OP_STRCPY    8
#define OP_STRLCPY   9
#define OP_STRCHR    10
#define OP_STRRCHR   11
#define OP_STRCHRNUL 12
#define OP_STRNLEN   13
#define OP_STPCPY    14
#define OP_STRNCPY   15
#define OP_STRCAT    16
#define OP_MEMCCPY   17
#define OP_STPNCPY   18

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct bench_op_s
{
  FAR const char *name;
  uint8_t         op;
  bool            dst;       /* Touches the destination pointer, so its
                              * offset is worth sweeping too */
  bool            str;       /* Operand is a string, so it needs a
                              * terminator at the measured length */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR char *g_src;
static FAR char *g_dst;
static volatile unsigned long g_sink;
static clockid_t g_clock = CLOCK_MONOTONIC;

static const struct bench_op_s g_ops[] =
{
  {"memcpy",  OP_MEMCPY,  true,  false},
  {"memmove", OP_MEMMOVE, true,  false},
  {"memset",  OP_MEMSET,  true,  false},
  {"memcmp",  OP_MEMCMP,  true,  false},
  {"memchr",  OP_MEMCHR,  false, false},
  {"strlen",  OP_STRLEN,  false, true},
  {"strcmp",  OP_STRCMP,  true,  true},
  {"strncmp", OP_STRNCMP, true,  true},
  {"strcpy",  OP_STRCPY,  true,  true},
  {"strlcpy", OP_STRLCPY, true,  true},
  {"strchr",  OP_STRCHR,  false, true},
  {"strrchr", OP_STRRCHR, false, true},
  {"strchrnul", OP_STRCHRNUL, false, true},
  {"strnlen", OP_STRNLEN, false, true},
  {"stpcpy",  OP_STPCPY,  true,  true},
  {"strncpy", OP_STRNCPY, true,  true},
  {"strcat",  OP_STRCAT,  true,  true},
  {"memccpy", OP_MEMCCPY, true,  false},
  {"stpncpy", OP_STPNCPY, true,  true},
};

static const size_t g_sizes[] =
{
  64, 512, 4096, 32768
};

/* Source and destination offsets within a register.  Aligned, equally
 * misaligned, misaligned by different amounts, and a source shift against
 * an aligned destination.
 */

static const uint8_t g_aligns[][2] =
{
  {
    0, 0
  },
  {
    1, 1
  },
  {
    1, 2
  },
  {
    3, 0
  }
};

#define NOPS    (sizeof(g_ops) / sizeof(g_ops[0]))
#define NSIZES  (sizeof(g_sizes) / sizeof(g_sizes[0]))
#define NALIGNS (sizeof(g_aligns) / sizeof(g_aligns[0]))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: bench_now
 ****************************************************************************/

static double bench_now(void)
{
  struct timespec t;

  clock_gettime(g_clock, &t);
  return t.tv_sec + t.tv_nsec / 1e9;
}

/****************************************************************************
 * Name: bench_pick_clock
 *
 * Description:
 *   Settle on a clock that runs.  Every measurement below repeats until a
 *   stated interval has passed, so a clock that reads the same value twice
 *   would spin forever rather than report anything.  CLOCK_MONOTONIC is
 *   preferred and does not advance on every target.
 *
 ****************************************************************************/

static bool bench_pick_clock(void)
{
  static const clockid_t tries[] =
  {
    CLOCK_MONOTONIC, CLOCK_REALTIME
  };

  struct timespec a;
  struct timespec b;
  volatile int i;
  size_t k;

  for (k = 0; k < sizeof(tries) / sizeof(tries[0]); k++)
    {
      if (clock_gettime(tries[k], &a) < 0)
        {
          continue;
        }

      for (i = 0; i < 1000000; i++);

      if (clock_gettime(tries[k], &b) < 0)
        {
          continue;
        }

      if (b.tv_sec != a.tv_sec || b.tv_nsec != a.tv_nsec)
        {
          g_clock = tries[k];
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: bench_seen_src
 *
 * Description:
 *   Whether an earlier alignment pair already used this source offset.
 *
 ****************************************************************************/

static bool bench_seen_src(size_t ai)
{
  size_t i;

  for (i = 0; i < ai; i++)
    {
      if (g_aligns[i][0] == g_aligns[ai][0])
        {
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: bench_prepare
 *
 * Description:
 *   Content for one measurement.  The compares need the two operands equal
 *   for the whole length, or they stop early and time nothing, and the
 *   absent-character scans need a byte that never appears.
 *
 ****************************************************************************/

static void bench_prepare(FAR const struct bench_op_s *o, size_t n,
                          int so, int dofs)
{
  FAR char *s = g_src + BENCH_GUARD + so;
  FAR char *d = g_dst + BENCH_GUARD + dofs;
  size_t i;

  for (i = 0; i < n + BENCH_GUARD; i++)
    {
      s[i] = 'a' + (i % 23);
    }

  memcpy(d, s, n + BENCH_GUARD);

  if (o->str)
    {
      s[n] = '\0';
      d[n] = '\0';
    }
}

/****************************************************************************
 * Name: bench_one
 *
 * Description:
 *   One operation at one size and one alignment pair, timed until it has
 *   run for at least the configured interval.
 *
 ****************************************************************************/

static void bench_one(FAR const struct bench_op_s *o, size_t n,
                      int so, int dofs)
{
#ifdef BENCH_CYCLES
  clock_t c0;
  clock_t c1;
#endif
  double  t0;
  double  el;
  double  mbs;
  unsigned long reps = 0;
  int i;

  bench_prepare(o, n, so, dofs);

  t0 = bench_now();
#ifdef BENCH_CYCLES
  c0 = perf_gettime();
#endif

  do
    {
      for (i = 0; i < BENCH_BATCH; i++)
        {
          FAR char *s = g_src + BENCH_GUARD + so;
          FAR char *d = g_dst + BENCH_GUARD + dofs;

          __asm__ volatile("" : "+r"(s), "+r"(d));

          switch (o->op)
            {
              case OP_MEMCPY:
                memcpy(d, s, n);
                break;
              case OP_MEMMOVE:
                memmove(d + 8, d, n);
                break;
              case OP_MEMSET:
                memset(d, i, n);
                break;
              case OP_MEMCMP:
                g_sink += memcmp(s, d, n);
                break;
              case OP_MEMCHR:
                g_sink += (uintptr_t)memchr(s, '~', n);
                break;
              case OP_STRLEN:
                g_sink += strlen(s);
                break;
              case OP_STRCMP:
                g_sink += strcmp(s, d);
                break;
              case OP_STRNCMP:
                g_sink += strncmp(s, d, n);
                break;
              case OP_STRCPY:
                strcpy(d, s);
                break;
              case OP_STRLCPY:
                g_sink += strlcpy(d, s, n + 1);
                break;
              case OP_STRCHR:
                g_sink += (uintptr_t)strchr(s, '~');
                break;
              case OP_STRRCHR:
                g_sink += (uintptr_t)strrchr(s, '~');
                break;
              case OP_STRCHRNUL:
                g_sink += (uintptr_t)strchrnul(s, '~');
                break;
              case OP_STRNLEN:
                g_sink += strnlen(s, n);
                break;
              case OP_STPCPY:
                g_sink += (uintptr_t)stpcpy(d, s);
                break;
              case OP_STRNCPY:
                strncpy(d, s, n);
                break;
              case OP_MEMCCPY:
                g_sink += (uintptr_t)memccpy(d, s, '~', n);
                break;
              case OP_STPNCPY:
                g_sink += (uintptr_t)stpncpy(d, s, n);
                break;
              case OP_STRCAT:

                /* Appending to what the last turn appended would grow the
                 * destination without bound, so it starts empty each time.
                 * The store is inside the measurement.
                 */

                d[0] = '\0';
                strcat(d, s);
                break;
            }

          g_sink += (unsigned char)d[0];
        }

      reps += BENCH_BATCH;
      el    = bench_now() - t0;
    }
  while (el * 1000.0 < BENCH_MINMS);

  mbs = (double)reps * n / el / 1048576.0;

#ifdef BENCH_CYCLES
  c1 = perf_gettime();
  printf("  %-8s %6zu B  s+%d/d+%d  %9.1f MB/s  %8.3f cyc/B\n",
         o->name, n, so, dofs, mbs,
         (double)(uintmax_t)(c1 - c0) / ((double)reps * n));
#else
  printf("  %-8s %6zu B  s+%d/d+%d  %9.1f MB/s\n",
         o->name, n, so, dofs, mbs);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arch_libc_bench
 *
 * Description:
 *   Run every operation over the size sweep and the alignment matrix.
 *
 ****************************************************************************/

int arch_libc_bench(void)
{
  size_t oi;
  size_t si;
  size_t ai;

  if (!bench_pick_clock())
    {
      printf("arch_libc bench: no clock advances, cannot time anything\n");
      return 1;
    }

  g_src = malloc(BENCH_BUF);
  g_dst = malloc(BENCH_BUF);
  if (g_src == NULL || g_dst == NULL)
    {
      printf("arch_libc bench: cannot allocate 2 x %d\n", BENCH_BUF);
      free(g_src);
      free(g_dst);
      return 1;
    }

  printf("== throughput ==\n");

  for (oi = 0; oi < NOPS; oi++)
    {
      for (si = 0; si < NSIZES; si++)
        {
          if (g_sizes[si] + 2 * BENCH_GUARD + 8 > BENCH_BUF)
            {
              continue;
            }

          for (ai = 0; ai < NALIGNS; ai++)
            {
              /* An operation that never touches the destination is decided
               * by the source offset alone, so a pair repeating one already
               * measured would report the same number twice.
               */

              if (!g_ops[oi].dst && bench_seen_src(ai))
                {
                  continue;
                }

              bench_one(&g_ops[oi], g_sizes[si],
                        g_aligns[ai][0], g_aligns[ai][1]);
            }
        }
    }

  free(g_src);
  free(g_dst);
  return 0;
}
