/****************************************************************************
 * apps/testing/libc/arch_libc/arch_libc_string.c
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

/* Correctness and speed for the string and memory functions a machine
 * directory may override, aimed at the mistakes such overrides actually
 * make: alignment cases (every combination of source and destination
 * offset within a register), sizes straddling every internal boundary
 * (word, block, and the small-size cutoff), overlap in both directions
 * for memmove, the interaction of terminators with length caps, and
 * writes past either end, which guard bytes around every destination
 * catch.
 *
 * The speed half measures through CLOCK_MONOTONIC.  Two things would
 * otherwise make it measure nothing: the compiler hoisting a pure call
 * whose arguments never change, countered by laundering the pointers
 * through an asm that claims to change them, and comparison buffers an
 * earlier benchmark scribbled on, countered by checking equality before
 * timing.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef CONFIG_TESTING_ARCH_LIBC_STRING

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STR_BUF     (CONFIG_TESTING_ARCH_LIBC_STRING_BUFSIZE)
#define STR_GUARD   64
#define STR_MINMS   250
#define STR_BIG     (STR_BUF / 4)
#define STR_STRLEN  4096

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR char *g_src;
static FAR char *g_dst;
static volatile unsigned long g_sink;
static int g_fails;

static const int g_sizes[] =
{
  0, 1, 2, 3, 7, 8, 9, 15, 16, 17, 31, 32, 63, 64, 65, 127, 128, 129,
  255, 256, 1000
};

#define NSIZES ((int)(sizeof(g_sizes) / sizeof(g_sizes[0])))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void str_note(FAR const char *fn, int bad)
{
  printf("  %-10s correctness: %s (%d bad)\n", fn,
         bad ? "FAIL" : "ok", bad);
  g_fails += bad;
}

static double str_now(void)
{
  struct timespec t;

  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec + t.tv_nsec / 1e9;
}

/****************************************************************************
 * Name: str_ck_copy
 *
 * Description:
 *   memcpy and memmove against distinct buffers: every alignment pair,
 *   every boundary size, result pointer, content, and the guard bytes.
 *
 ****************************************************************************/

static void str_ck_copy(int move)
{
  int k;
  int so;
  int dofs;
  int i;
  int bad = 0;

  for (k = 0; k < NSIZES; k++)
    {
      for (so = 0; so < 8; so++)
        {
          for (dofs = 0; dofs < 8; dofs++)
            {
              int n = g_sizes[k];
              FAR char *s = g_src + STR_GUARD + so;
              FAR char *d = g_dst + STR_GUARD + dofs;
              FAR void *r;

              for (i = 0; i < n; i++)
                {
                  s[i] = (char)(i * 7 + so + k + 1);
                }

              memset(g_dst, 0x5a, 2 * STR_GUARD + 1100);
              r = move ? memmove(d, s, n) : memcpy(d, s, n);

              if (r != d)
                {
                  bad++;
                  continue;
                }

              for (i = 0; i < n; i++)
                {
                  if (d[i] != (char)(i * 7 + so + k + 1))
                    {
                      bad++;
                      break;
                    }
                }

              if ((unsigned char)d[-1] != 0x5a ||
                  (unsigned char)d[n] != 0x5a)
                {
                  bad++;
                }
            }
        }
    }

  str_note(move ? "memmove" : "memcpy", bad);
}

/****************************************************************************
 * Name: str_ck_overlap
 *
 * Description:
 *   memmove where the regions overlap, forward and backward, at every
 *   distance up to a register width either side, at every size.
 *
 ****************************************************************************/

static void str_ck_overlap(void)
{
  int k;
  int ofs;
  int i;
  int bad = 0;

  for (k = 0; k < NSIZES; k++)
    {
      for (ofs = -8; ofs <= 8; ofs++)
        {
          int n = g_sizes[k];
          FAR char *base = g_dst + STR_GUARD + 16;
          FAR char *d = base + ofs;

          if (n == 0)
            {
              continue;
            }

          for (i = -8; i < n + 8; i++)
            {
              base[i] = (char)(i * 5 + k + 3);
            }

          memmove(d, base, n);

          for (i = 0; i < n; i++)
            {
              if (d[i] != (char)(i * 5 + k + 3))
                {
                  bad++;
                  break;
                }
            }
        }
    }

  str_note("mv-overlap", bad);
}

/****************************************************************************
 * Name: str_ck_memset
 ****************************************************************************/

static void str_ck_memset(void)
{
  int k;
  int dofs;
  int i;
  int bad = 0;

  for (k = 0; k < NSIZES; k++)
    {
      for (dofs = 0; dofs < 8; dofs++)
        {
          int n = g_sizes[k];
          FAR char *d = g_dst + STR_GUARD + dofs;

          memset(g_dst, 0x5a, 2 * STR_GUARD + 1100);
          if (memset(d, 0xc3, n) != d)
            {
              bad++;
              continue;
            }

          for (i = 0; i < n; i++)
            {
              if ((unsigned char)d[i] != 0xc3)
                {
                  bad++;
                  break;
                }
            }

          if ((unsigned char)d[-1] != 0x5a ||
              (unsigned char)d[n] != 0x5a)
            {
              bad++;
            }
        }
    }

  str_note("memset", bad);
}

/****************************************************************************
 * Name: str_ck_memcmp
 *
 * Description:
 *   Equal buffers, then a single difference planted at the start, the
 *   middle and the end: the sign both ways, and that a length cap short
 *   of the difference hides it.  Values stay below 0x80 so the planted
 *   difference cannot wrap.
 *
 ****************************************************************************/

static void str_ck_memcmp(void)
{
  int k;
  int so;
  int dofs;
  int at;
  int bad = 0;

  for (k = 0; k < NSIZES; k++)
    {
      for (so = 0; so < 4; so++)
        {
          for (dofs = 0; dofs < 4; dofs++)
            {
              int n = g_sizes[k];
              FAR char *a = g_src + STR_GUARD + so;
              FAR char *b = g_dst + STR_GUARD + dofs;
              int i;

              for (i = 0; i < n; i++)
                {
                  a[i] = b[i] = (char)((i * 3 + 40) & 0x7f);
                }

              if (memcmp(a, b, n) != 0)
                {
                  bad++;
                  continue;
                }

              for (at = 0; at < n; at += (n > 8 ? n / 3 : 1))
                {
                  b[at] = (char)(a[at] + 13);
                  if (memcmp(a, b, n) >= 0)
                    {
                      bad++;
                    }

                  if (memcmp(b, a, n) <= 0)
                    {
                      bad++;
                    }

                  if (memcmp(a, b, at) != 0)
                    {
                      bad++;
                    }

                  b[at] = a[at];
                }
            }
        }
    }

  str_note("memcmp", bad);
}

/****************************************************************************
 * Name: str_ck_scan
 *
 * Description:
 *   strlen, strnlen, memchr and the strchr family: every alignment,
 *   lengths crossing the word size, the probe at every position, a
 *   probe that is absent, and the terminator probed for by name.  A
 *   tripwire byte sits just past every terminator.
 *
 ****************************************************************************/

static void str_ck_scan(void)
{
  int so;
  int len;
  int at;
  int i;
  int bad = 0;

  for (so = 0; so < 8; so++)
    {
      for (len = 0; len < 40; len++)
        {
          FAR char *s = g_src + STR_GUARD + so;

          for (i = 0; i < len; i++)
            {
              s[i] = 'a' + (i % 23);
            }

          s[len] = '\0';
          s[len + 1] = 'Z';

          if ((int)strlen(s) != len)
            {
              bad++;
            }

          if ((int)strnlen(s, 1000) != len)
            {
              bad++;
            }

          if ((int)strnlen(s, len) != len)
            {
              bad++;
            }

          if (len > 0 && (int)strnlen(s, len - 1) != len - 1)
            {
              bad++;
            }

          if ((int)strnlen(s, 0) != 0)
            {
              bad++;
            }

          for (at = 0; at < len; at += (len > 6 ? len / 3 : 1))
            {
              char c = s[at];
              FAR char *want = NULL;

              for (i = 0; i <= len; i++)
                {
                  if (s[i] == c)
                    {
                      want = s + i;
                      break;
                    }
                }

              if (strchr(s, c) != want)
                {
                  bad++;
                }

              if (strchrnul(s, c) != want)
                {
                  bad++;
                }

              want = NULL;
              for (i = len; i >= 0; i--)
                {
                  if (s[i] == c)
                    {
                      want = s + i;
                      break;
                    }
                }

              if (strrchr(s, c) != want)
                {
                  bad++;
                }
            }

          if (strchr(s, '~') != NULL)
            {
              bad++;
            }

          if (strchrnul(s, '~') != s + len)
            {
              bad++;
            }

          if (strrchr(s, '~') != NULL)
            {
              bad++;
            }

          if (strchr(s, '\0') != s + len)
            {
              bad++;
            }

          if (strrchr(s, '\0') != s + len)
            {
              bad++;
            }

          if (len > 2)
            {
              if (memchr(s, s[len - 1], len) == NULL)
                {
                  bad++;
                }

              if (memchr(s, '~', len) != NULL)
                {
                  bad++;
                }
            }
        }
    }

  str_note("str-scan", bad);
}

/****************************************************************************
 * Name: str_ck_cmpcpy
 *
 * Description:
 *   strcmp, strncmp and strcpy: a planted difference at every position,
 *   the n cap before, at and past it, and the copy checked round trip
 *   with a tripwire past the terminator.
 *
 ****************************************************************************/

static void str_ck_cmpcpy(void)
{
  int ao;
  int bo;
  int at;
  int i;
  int bad = 0;

  for (ao = 0; ao < 8; ao++)
    {
      for (bo = 0; bo < 8; bo++)
        {
          for (at = 0; at < 24; at++)
            {
              FAR char *a = g_src + STR_GUARD + ao;
              FAR char *b = g_dst + STR_GUARD + bo;
              int exp;

              for (i = 0; i < 24; i++)
                {
                  a[i] = b[i] = 'A' + (i % 20);
                }

              a[23] = b[23] = '\0';
              if (at < 23)
                {
                  b[at] = a[at] + 1;
                }

              exp = (at < 23) ? -1 : 0;
              i = strcmp(a, b);
              i = (i < 0) ? -1 : ((i > 0) ? 1 : 0);
              if (i != exp)
                {
                  bad++;
                }

              if (strncmp(a, b, at) != 0)
                {
                  bad++;
                }

              i = strncmp(a, b, 24);
              i = (i < 0) ? -1 : ((i > 0) ? 1 : 0);
              if (i != exp)
                {
                  bad++;
                }

              if (strncmp(a, b, 0) != 0)
                {
                  bad++;
                }

              {
                FAR char *d = g_dst + STR_GUARD + 40 + bo;

                d[24] = 0x33;
                if (strcpy(d, a) != d)
                  {
                    bad++;
                  }

                if (strcmp(d, a) != 0)
                  {
                    bad++;
                  }

                if (d[24] != 0x33)
                  {
                    bad++;
                  }
              }
            }
        }
    }

  str_note("strcmp-fam", bad);
}

/****************************************************************************
 * Name: str_ck_strlcpy
 *
 * Description:
 *   strlcpy: the return is always the source length, the result is
 *   always terminated when the cap is nonzero, at most cap-1 bytes are
 *   copied, and a cap of zero writes nothing at all.
 *
 ****************************************************************************/

static void str_ck_strlcpy(void)
{
  int so;
  int dofs;
  int len;
  size_t cap;
  int i;
  int bad = 0;

  for (so = 0; so < 8; so++)
    {
      for (dofs = 0; dofs < 8; dofs++)
        {
          for (len = 0; len < 24; len++)
            {
              FAR char *a = g_src + STR_GUARD + so;

              for (i = 0; i < len; i++)
                {
                  a[i] = 'a' + (i % 23);
                }

              a[len] = '\0';

              for (cap = 0; cap <= (size_t)len + 2; cap++)
                {
                  FAR char *d = g_dst + STR_GUARD + dofs;

                  memset(g_dst, 0x5a, 2 * STR_GUARD + 64);
                  if (strlcpy(d, a, cap) != (size_t)len)
                    {
                      bad++;
                    }

                  if (cap > 0)
                    {
                      size_t want = ((size_t)len < cap - 1) ?
                                    (size_t)len : cap - 1;

                      if (d[want] != '\0')
                        {
                          bad++;
                        }

                      for (i = 0; i < (int)want; i++)
                        {
                          if (d[i] != a[i])
                            {
                              bad++;
                              break;
                            }
                        }

                      if ((unsigned char)d[want + 1] != 0x5a &&
                          want == cap - 1)
                        {
                          bad++;
                        }
                    }
                  else if ((unsigned char)d[0] != 0x5a)
                    {
                      bad++;
                    }
                }
            }
        }
    }

  str_note("strlcpy", bad);
}

/****************************************************************************
 * Name: str_bench
 *
 * Description:
 *   Time one scenario and print a rate.  The asm around the pointers
 *   stops the compiler treating a pure call with unchanged arguments as
 *   loop invariant, without disturbing the alignment under test.
 *
 ****************************************************************************/

static void str_bench(FAR const char *name, int op, int so, int dofs, int n)
{
  double t0;
  double el;
  long reps = 0;
  int i;

  t0 = str_now();
  do
    {
      for (i = 0; i < 16; i++)
        {
          FAR char *s = g_src + STR_GUARD + so;
          FAR char *d = g_dst + STR_GUARD + dofs;

          __asm__ volatile("" : "+r"(s), "+r"(d));

          switch (op)
            {
              case 0:
                memcpy(d, s, n);
                g_sink += (unsigned char)d[0];
                break;
              case 1:
                memmove(d + 8, d, n);
                g_sink += (unsigned char)d[8];
                break;
              case 2:
                g_sink += memcmp(s, d, n);
                break;
              case 3:
                g_sink += strlen(s);
                break;
              case 4:
                g_sink += strncmp(s, d, n);
                break;
              case 5:
                g_sink += (uintptr_t)memchr(s, '~', n);
                break;
              case 6:
                g_sink += (uintptr_t)strchr(s, '~');
                break;
              case 7:
                strcpy(d, s);
                g_sink += (unsigned char)d[0];
                break;
              case 8:
                g_sink += (uintptr_t)strrchr(s, 'q');
                break;
              case 9:
                memset(d, i, n);
                g_sink += (unsigned char)d[0];
                break;
              case 10:
                g_sink += strlcpy(d, s, n);
                break;
            }
        }

      reps += 16;
      el = str_now() - t0;
    }
  while (el * 1000.0 < STR_MINMS);

  printf("  %-26s %9.1f MB/s\n", name,
         (double)reps * n / el / 1048576.0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arch_libc_test_string
 ****************************************************************************/

int arch_libc_test_string(void)
{
  int i;

  g_src = malloc(STR_BUF);
  g_dst = malloc(STR_BUF);
  if (g_src == NULL || g_dst == NULL)
    {
      printf("arch_libc string: cannot allocate 2 x %d\n", STR_BUF);
      free(g_src);
      free(g_dst);
      return 1;
    }

  printf("== string correctness ==\n");
  str_ck_copy(0);
  str_ck_copy(1);
  str_ck_overlap();
  str_ck_memset();
  str_ck_memcmp();
  str_ck_scan();
  str_ck_cmpcpy();
  str_ck_strlcpy();

  /* Speed.  Equal content in both buffers, strings terminated at the
   * probe lengths, and no '~' anywhere, so the absent-probe scans walk
   * the whole string.
   */

  for (i = 0; i < STR_BUF; i++)
    {
      g_src[i] = 'a' + (i % 23);
    }

  memcpy(g_dst, g_src, STR_BUF);
  g_src[STR_GUARD + STR_BIG] = '\0';
  g_dst[STR_GUARD + STR_BIG] = '\0';
  g_src[STR_GUARD + STR_STRLEN] = '\0';
  g_dst[STR_GUARD + STR_STRLEN] = '\0';

  printf("== string speed ==\n");

  /* Readers first: the writers below scribble on the destination
   * buffer, and the compares depend on the two staying equal.
   */

  if (memcmp(g_src + STR_GUARD, g_dst + STR_GUARD, STR_BIG) != 0)
    {
      printf("  compare buffers differ, speed numbers void\n");
      g_fails++;
    }

  str_bench("memcmp big aligned",     2, 0, 0, STR_BIG);
  str_bench("memcmp big s+1/d+1",     2, 1, 1, STR_BIG);
  str_bench("strncmp 4K aligned",     4, 0, 0, STR_STRLEN);
  str_bench("strncmp 4K both+2",      4, 2, 2, STR_STRLEN);
  str_bench("strlen 4K aligned",      3, 0, 0, STR_STRLEN);
  str_bench("strlen 4K +3",           3, 3, 0, STR_STRLEN);
  str_bench("memchr 4K aligned",      5, 0, 0, STR_STRLEN);
  str_bench("strchr 4K absent",       6, 0, 0, STR_STRLEN);
  str_bench("strchr 4K absent +5",    6, 5, 0, STR_STRLEN);
  str_bench("strrchr 4K",             8, 0, 0, STR_STRLEN);

  /* Mismatched-offset compare wants equal content at shifted positions,
   * and preparing that scribbles, so it comes after every clean reader.
   */

  memcpy(g_dst + STR_GUARD + 2, g_src + STR_GUARD + 1, STR_BIG + 8);
  str_bench("memcmp big s+1/d+2",     2, 1, 2, STR_BIG);

  str_bench("memcpy big aligned",     0, 0, 0, STR_BIG);
  str_bench("memcpy big src+1",       0, 1, 0, STR_BIG);
  str_bench("memmove-bk big",         1, 0, 0, STR_BIG);
  str_bench("memset big aligned",     9, 0, 0, STR_BIG);
  str_bench("strcpy 4K aligned",      7, 0, 0, STR_STRLEN);
  str_bench("strcpy 4K both+1",       7, 1, 1, STR_STRLEN);
  str_bench("strlcpy 4K aligned",    10, 0, 0, STR_STRLEN);

  printf("== string fails: %d ==\n", g_fails);
  free(g_src);
  free(g_dst);
  return g_fails != 0;
}

#endif /* CONFIG_TESTING_ARCH_LIBC_STRING */
