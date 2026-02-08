/****************************************************************************
 * apps/testing/libc/stdbit/stdbit.c
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

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbit.h>
#include <limits.h>
#include <sys/param.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CHAR_SIZ (8)
#define NBITS(x) (sizeof(x) * CHAR_SIZ)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct test_entry_s
{
  const char name[NAME_MAX];
  CODE int (*entry)(void);
};

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static int leading_zeros(void);
static int leading_ones(void);
static int trailing_zeros(void);
static int trailing_ones(void);
static int first_leading_zero(void);
static int first_leading_one(void);
static int first_trailing_zero(void);
static int first_trailing_one(void);
static int count_zeros(void);
static int count_ones(void);
static int has_single_bit(void);
static int bit_width(void);
static int bit_floor(void);
static int bit_ceil(void);

/****************************************************************************
 * Private data
 ****************************************************************************/

static const struct test_entry_s g_entry_list[] =
{
  {"leading_zeros", leading_zeros},
  {"leading_ones", leading_ones},
  {"trailing_zeros", trailing_zeros},
  {"trailing_ones", trailing_ones},
  {"first_leading_zero", first_leading_zero},
  {"first_leading_one", first_leading_one},
  {"first_trailing_zero", first_trailing_zero},
  {"first_trailing_one", first_trailing_one},
  {"count_zeros", count_zeros},
  {"count_ones", count_ones},
  {"has_single_bit", has_single_bit},
  {"bit_width", bit_width},
  {"bit_floor", bit_floor},
  {"bit_ceil", bit_ceil},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: leading_zeros
 ****************************************************************************/

static int leading_zeros(void)
{
  int i;
  int nbits;
  int ret;
  int ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  nbits = NBITS(uc);
  uc = (1 << (nbits - 1));
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_leading_zeros_uc(uc);
      ret2 = stdc_leading_zeros(uc);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      uc = uc >> 1;
    }

  nbits = NBITS(us);
  us = (1 << (nbits - 1));
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_leading_zeros_us(us);
      ret2 = stdc_leading_zeros(us);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      us = us >> 1;
    }

  nbits = NBITS(ui);
  ui = (1U << (nbits - 1));
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_leading_zeros_ui(ui);
      ret2 = stdc_leading_zeros(ui);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ui = ui >> 1;
    }

  nbits = NBITS(ul);
  ul = (1UL << (nbits - 1));
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_leading_zeros_ul(ul);
      ret2 = stdc_leading_zeros(ul);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ul = ul >> 1;
    }

  nbits = NBITS(ull);
  ull = (1ULL << (nbits - 1));
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_leading_zeros_ul(ull);
      ret2 = stdc_leading_zeros(ull);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ull = ull >> 1;
    }

  return 0;
}

/****************************************************************************
 * Name: leading_ones
 ****************************************************************************/

static int leading_ones(void)
{
  int i;
  int nbits;
  int ret;
  int ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  nbits = NBITS(uc);
  uc = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_leading_ones_uc(uc);
      ret2 = stdc_leading_ones(uc);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      uc |= (1 << (nbits - (i + 1)));
    }

  nbits = NBITS(us);
  us = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_leading_ones_us(us);
      ret2 = stdc_leading_ones(us);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      us |= (1 << (nbits - (i + 1)));
    }

  nbits = NBITS(ui);
  ui = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_leading_ones_ui(ui);
      ret2 = stdc_leading_ones(ui);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ui |= (1U << (nbits - (i + 1)));
    }

  nbits = NBITS(ul);
  ul = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_leading_ones_ul(ul);
      ret2 = stdc_leading_ones(ul);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ul |= (1UL << (nbits - (i + 1)));
    }

  nbits = NBITS(ull);
  ull = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_leading_ones_ul(ull);
      ret2 = stdc_leading_ones(ull);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ull |= (1ULL << (nbits - (i + 1)));
    }

  return 0;
}

/****************************************************************************
 * Name: trailing_zeros
 ****************************************************************************/

static int trailing_zeros(void)
{
  int i;
  int nbits;
  int ret;
  int ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  nbits = NBITS(uc);
  uc = 1;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_trailing_zeros_uc(uc);
      ret2 = stdc_trailing_zeros(uc);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      uc = uc << 1;
    }

  nbits = NBITS(us);
  us = 1;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_trailing_zeros_us(us);
      ret2 = stdc_trailing_zeros(us);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      us = us << 1;
    }

  nbits = NBITS(ui);
  ui = 1;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_trailing_zeros_ui(ui);
      ret2 = stdc_trailing_zeros(ui);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ui = ui << 1;
    }

  nbits = NBITS(ul);
  ul = 1;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_trailing_zeros_ul(ul);
      ret2 = stdc_trailing_zeros(ul);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ul = ul << 1;
    }

  nbits = NBITS(ull);
  ull = 1;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_trailing_zeros_ul(ull);
      ret2 = stdc_trailing_zeros(ull);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ull = ull << 1;
    }

  return 0;
}

/****************************************************************************
 * Name: traling_ones
 ****************************************************************************/

static int trailing_ones(void)
{
  int i;
  int nbits;
  int ret;
  int ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  nbits = NBITS(uc);
  uc = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_trailing_ones_uc(uc);
      ret2 = stdc_trailing_ones(uc);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      uc |= (1 << i);
    }

  nbits = NBITS(us);
  us = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_trailing_ones_us(us);
      ret2 = stdc_trailing_ones(us);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      us |= (1 << i);
    }

  nbits = NBITS(ui);
  ui = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_trailing_ones_ui(ui);
      ret2 = stdc_trailing_ones(ui);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ui |= (1U << i);
    }

  nbits = NBITS(ul);
  ul = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_trailing_ones_ul(ul);
      ret2 = stdc_trailing_ones(ul);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ul |= (1UL << i);
    }

  nbits = NBITS(ull);
  ull = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_trailing_ones_ul(ull);
      ret2 = stdc_trailing_ones(ull);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ull |= (1ULL << i);
    }

  return 0;
}

/****************************************************************************
 * Name: first_leading_zero
 ****************************************************************************/

static int first_leading_zero(void)
{
  int i;
  int nbits;
  int ret;
  int ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  nbits = NBITS(uc);
  uc = 0;
  for (i = 1; i <= nbits + 1; i++)
    {
      ret = stdc_first_leading_zero_uc(uc);
      ret2 = stdc_first_leading_zero(uc);
      if (ret != (i % (nbits + 1)) || ret2 != (i % (nbits + 1)))
        {
          return -1;
        }

      uc |= (1 << (nbits - i));
    }

  nbits = NBITS(us);
  us = 0;
  for (i = 1; i <= nbits + 1; i++)
    {
      ret = stdc_first_leading_zero_us(us);
      ret2 = stdc_first_leading_zero(us);
      if (ret != (i % (nbits + 1)) || ret2 != (i % (nbits + 1)))
        {
          return -1;
        }

      us |= (1 << (nbits - i));
    }

  nbits = NBITS(ui);
  ui = 0;
  for (i = 1; i <= nbits + 1; i++)
    {
      ret = stdc_first_leading_zero_ui(ui);
      ret2 = stdc_first_leading_zero(ui);
      if (ret != (i % (nbits + 1)) || ret2 != (i % (nbits + 1)))
        {
          return -1;
        }

      ui |= (1U << (nbits - i));
    }

  nbits = NBITS(ul);
  ul = 0;
  for (i = 1; i <= (nbits + 1); i++)
    {
      ret = stdc_first_leading_zero_ul(ul);
      ret2 = stdc_first_leading_zero(ul);
      if (ret != (i % (nbits + 1)) || ret2 != (i % (nbits + 1)))
        {
          return -1;
        }

      ul |= (1UL << (nbits - i));
    }

  nbits = NBITS(ull);
  ull = 0;
  for (i = 1; i <= nbits + 1; i++)
    {
      ret = stdc_first_leading_zero_ul(ull);
      ret2 = stdc_first_leading_zero(ull);
      if (ret != (i % (nbits + 1)) || ret2 != (i % (nbits + 1)))
        {
          return -1;
        }

      ull |= (1ULL << (nbits - i));
    }

  return 0;
}

/****************************************************************************
 * Name: first_leading_zero
 ****************************************************************************/

static int first_leading_one(void)
{
  int i;
  int nbits;
  int ret;
  int ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  nbits = NBITS(uc);
  uc = UCHAR_MAX;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_leading_one_uc(uc);
      ret2 = stdc_first_leading_one(uc);
      if (ret != ((i + 1) % (nbits + 1)) || ret2 != ((i + 1) % (nbits + 1)))
        {
          return -1;
        }

      uc &= ~(1 << (nbits - (i + 1)));
    }

  nbits = NBITS(us);
  us = USHRT_MAX;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_leading_one_us(us);
      ret2 = stdc_first_leading_one(us);
      if (ret != ((i + 1) % (nbits + 1)) || ret2 != ((i + 1) % (nbits + 1)))
        {
          return -1;
        }

      us &= ~(1 << (nbits - (i + 1)));
    }

  nbits = NBITS(ui);
  ui = UINT_MAX;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_leading_one_ui(ui);
      ret2 = stdc_first_leading_one(ui);
      if (ret != ((i + 1) % (nbits + 1)) || ret2 != ((i + 1) % (nbits + 1)))
        {
          return -1;
        }

      ui &= ~(1U << (nbits - (i + 1)));
    }

  nbits = NBITS(ul);
  ul = ULONG_MAX;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_leading_one_ul(ul);
      ret2 = stdc_first_leading_one(ul);
      if (ret != ((i + 1) % (nbits + 1)) || ret2 != ((i + 1) % (nbits + 1)))
        {
          return -1;
        }

      ul &= ~(1UL << (nbits - (i + 1)));
    }

  nbits = NBITS(ull);
  ull = ULLONG_MAX;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_leading_one_ull(ull);
      ret2 = stdc_first_leading_one(ull);
      if (ret != ((i + 1) % (nbits + 1)) || ret2 != ((i + 1) % (nbits + 1)))
        {
          return -1;
        }

      ull &= ~(1ULL << (nbits - (i + 1)));
    }

  return 0;
}

/****************************************************************************
 * Name: first_trailing_zero
 ****************************************************************************/

static int first_trailing_zero(void)
{
  int i;
  int nbits;
  int ret;
  int ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  nbits = NBITS(uc);
  uc = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_trailing_zero_uc(uc);
      ret2 = stdc_first_trailing_zero(uc);
      if (ret != ((i + 1) % (nbits + 1)) || ret2 != ((i + 1) % (nbits + 1)))
        {
          return -1;
        }

      uc |= (1 << i);
    }

  nbits = NBITS(us);
  us = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_trailing_zero_us(us);
      ret2 = stdc_first_trailing_zero(us);
      if (ret != ((i + 1) % (nbits + 1)) || ret2 != ((i + 1) % (nbits + 1)))
        {
          return -1;
        }

      us |= (1 << i);
    }

  nbits = NBITS(ui);
  ui = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_trailing_zero_ui(ui);
      ret2 = stdc_first_trailing_zero(ui);
      if (ret != ((i + 1) % (nbits + 1)) || ret2 != ((i + 1) % (nbits + 1)))
        {
          return -1;
        }

      ui |= (1U << i);
    }

  nbits = NBITS(ul);
  ul = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_trailing_zero_ul(ul);
      ret2 = stdc_first_trailing_zero(ul);
      if (ret != ((i + 1) % (nbits + 1)) || ret2 != ((i + 1) % (nbits + 1)))
        {
          return -1;
        }

      ul |= (1UL << i);
    }

  nbits = NBITS(ull);
  ull = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_trailing_zero_ull(ull);
      ret2 = stdc_first_trailing_zero(ull);
      if (ret != ((i + 1) % (nbits + 1)) || ret2 != ((i + 1) % (nbits + 1)))
        {
          return -1;
        }

      ull |= (1ULL << i);
    }

  return 0;
}

/****************************************************************************
 * Name: first_trailing_one
 ****************************************************************************/

static int first_trailing_one(void)
{
  int i;
  int nbits;
  int ret;
  int ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  nbits = NBITS(uc);
  uc = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_trailing_one_uc(uc);
      ret2 = stdc_first_trailing_one(uc);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      uc = (1 << i);
    }

  nbits = NBITS(us);
  us = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_trailing_one_us(us);
      ret2 = stdc_first_trailing_one(us);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      us = (1 << i);
    }

  nbits = NBITS(ui);
  ui = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_trailing_one_ui(ui);
      ret2 = stdc_first_trailing_one(ui);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ui = (1U << i);
    }

  nbits = NBITS(ul);
  ul = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_trailing_one_ul(ul);
      ret2 = stdc_first_trailing_one(ul);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ul = (1UL << i);
    }

  nbits = NBITS(ull);
  ull = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_first_trailing_one_ull(ull);
      ret2 = stdc_first_trailing_one(ull);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ull = (1ULL << i);
    }

  return 0;
}

/****************************************************************************
 * Name: count_zeros
 ****************************************************************************/

static int count_zeros(void)
{
  int i;
  int nbits;
  int ret;
  int ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  nbits = NBITS(uc);
  uc = 0;
  for (i = nbits; i >= 0; i--)
    {
      ret = stdc_count_zeros_uc(uc);
      ret2 = stdc_count_zeros(uc);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      uc |= (1 << (i - 1));
    }

  nbits = NBITS(us);
  us = 0;
  for (i = nbits; i >= 0; i--)
    {
      ret = stdc_count_zeros_us(us);
      ret2 = stdc_count_zeros(us);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      us |= (1 << (i - 1));
    }

  nbits = NBITS(ui);
  ui = 0;
  for (i = nbits; i >= 0; i--)
    {
      ret = stdc_count_zeros_ui(ui);
      ret2 = stdc_count_zeros(ui);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ui |= (1U << (i - 1));
    }

  nbits = NBITS(ul);
  ul = 0;
  for (i = nbits; i >= 0; i--)
    {
      ret = stdc_count_zeros_ul(ul);
      ret2 = stdc_count_zeros(ul);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ul |= (1UL << (i - 1));
    }

  nbits = NBITS(ull);
  ull = 0;
  for (i = nbits; i >= 0; i--)
    {
      ret = stdc_count_zeros_ull(ull);
      ret2 = stdc_count_zeros(ull);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ull |= (1ULL << (i - 1));
    }

  return 0;
}

/****************************************************************************
 * Name: count_ones
 ****************************************************************************/

static int count_ones(void)
{
  int i;
  int nbits;
  int ret;
  int ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  nbits = NBITS(uc);
  uc = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_count_ones_uc(uc);
      ret2 = stdc_count_ones(uc);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      uc |= (1 << i);
    }

  nbits = NBITS(us);
  us = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_count_ones_us(us);
      ret2 = stdc_count_ones(us);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      us |= (1 << i);
    }

  nbits = NBITS(ui);
  ui = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_count_ones_ui(ui);
      ret2 = stdc_count_ones(ui);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ui |= (1U << i);
    }

  nbits = NBITS(ul);
  ul = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_count_ones_ul(ul);
      ret2 = stdc_count_ones(ul);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ul |= (1UL << i);
    }

  nbits = NBITS(ull);
  ull = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_count_ones_ull(ull);
      ret2 = stdc_count_ones(ull);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ull |= (1ULL << i);
    }

  return 0;
}

/****************************************************************************
 * Name: has_single_bit
 ****************************************************************************/

static int has_single_bit(void)
{
  int i;
  int nbits;
  bool ret;
  bool ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  nbits = NBITS(uc);
  uc = (1 << (nbits - 1));
  for (i = 0; i < nbits; i++)
    {
      ret = stdc_has_single_bit_uc(uc);
      ret2 = stdc_has_single_bit(uc);
      if (!ret || !ret2)
        {
          return -1;
        }

      uc = uc >> 1;
    }

  nbits = NBITS(us);
  us = (1 << (nbits - 1));
  for (i = 0; i < nbits; i++)
    {
      ret = stdc_has_single_bit_us(us);
      ret2 = stdc_has_single_bit(us);
      if (!ret || !ret2)
        {
          return -1;
        }

      us = us >> 1;
    }

  nbits = NBITS(ui);
  ui = (1U << (nbits - 1));
  for (i = 0; i < nbits; i++)
    {
      ret = stdc_has_single_bit_ui(ui);
      ret2 = stdc_has_single_bit(ui);
      if (!ret || !ret2)
        {
          return -1;
        }

      ui = ui >> 1;
    }

  nbits = NBITS(ul);
  ul = (1UL << (nbits - 1));
  for (i = 0; i < nbits; i++)
    {
      ret = stdc_has_single_bit_ul(ul);
      ret2 = stdc_has_single_bit(ul);
      if (!ret || !ret2)
        {
          return -1;
        }

      ul = ul >> 1;
    }

  nbits = NBITS(ull);
  ull = (1ULL << (nbits - 1));
  for (i = 0; i < nbits; i++)
    {
      ret = stdc_has_single_bit_ull(ull);
      ret2 = stdc_has_single_bit(ull);
      if (!ret || !ret2)
        {
          return -1;
        }

      ull = ull >> 1;
    }

  return 0;
}

/****************************************************************************
 * Name: bit_width
 ****************************************************************************/

static int bit_width(void)
{
  int i;
  int nbits;
  int ret;
  int ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  nbits = NBITS(uc);
  uc = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_bit_width_uc(uc);
      ret2 = stdc_bit_width(uc);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      uc = 1 << i;
    }

  nbits = NBITS(us);
  us = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_bit_width_us(us);
      ret2 = stdc_bit_width(us);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      us = 1 << i;
    }

  nbits = NBITS(ui);
  ui = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_bit_width_ui(ui);
      ret2 = stdc_bit_width(ui);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ui = 1U << i;
    }

  nbits = NBITS(ul);
  ul = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_bit_width_ul(ul);
      ret2 = stdc_bit_width(ul);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ul = 1UL << i;
    }

  nbits = NBITS(ull);
  ull = 0;
  for (i = 0; i <= nbits; i++)
    {
      ret = stdc_bit_width_ull(ull);
      ret2 = stdc_bit_width(ull);
      if (ret != i || ret2 != i)
        {
          return -1;
        }

      ull = 1ULL << i;
    }

  return 0;
}

static int bit_floor(void)
{
  int nbits;
  unsigned long long ret;
  unsigned long long ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  uc = 5;
  ret = stdc_bit_floor_uc(uc);
  ret2 = stdc_bit_floor(uc);
  if (ret != 4 || ret2 != 4)
    {
      return -1;
    }

  nbits = NBITS(uc);
  uc = (1 << (nbits - 1)) + 1;
  ret = stdc_bit_floor_uc(uc);
  ret2 = stdc_bit_floor(uc);
  if (ret != uc - 1 || ret2 != uc - 1)
    {
      return -1;
    }

  us = 5;
  ret = stdc_bit_floor_us(us);
  ret2 = stdc_bit_floor(us);
  if (ret != 4 || ret2 != 4)
    {
      return -1;
    }

  nbits = NBITS(us);
  us = (1 << (nbits - 1)) + 1;
  ret = stdc_bit_floor_us(us);
  ret2 = stdc_bit_floor(us);
  if (ret != us - 1 || ret2 != us - 1)
    {
      return -1;
    }

  ui = 5;
  ret = stdc_bit_floor_ui(ui);
  ret2 = stdc_bit_floor(ui);
  if (ret != 4 || ret2 != 4)
    {
      return -1;
    }

  nbits = NBITS(uc);
  ui = (1U << (nbits - 1)) + 1;
  ret = stdc_bit_floor_ui(ui);
  ret2 = stdc_bit_floor(ui);
  if (ret != ui - 1 || ret2 != ui - 1)
    {
      return -1;
    }

  ul = 5;
  ret = stdc_bit_floor_ul(ul);
  ret2 = stdc_bit_floor(ul);
  if (ret != 4 || ret2 != 4)
    {
      return -1;
    }

  nbits = NBITS(ul);
  ul = (1UL << (nbits - 1)) + 1;
  ret = stdc_bit_floor_ul(ul);
  ret2 = stdc_bit_floor(ul);
  if (ret != ul - 1 || ret2 != ul - 1)
    {
      return -1;
    }

  ull = 5;
  ret = stdc_bit_floor_ull(ull);
  ret2 = stdc_bit_floor(ull);
  if (ret != 4 || ret2 != 4)
    {
      return -1;
    }

  nbits = NBITS(ull);
  ull = (1ULL << (nbits - 1)) + 1;
  ret = stdc_bit_floor_ull(ull);
  ret2 = stdc_bit_floor(ull);
  if (ret != ull - 1 || ret2 != ull - 1)
    {
      return -1;
    }

  return 0;
}

static int bit_ceil(void)
{
  int nbits;
  unsigned long long ret;
  unsigned long long ret2;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long int ul;
  unsigned long long int ull;

  uc = 5;
  ret = stdc_bit_ceil_uc(uc);
  ret2 = stdc_bit_ceil(uc);
  if (ret != 8 || ret2 != 8)
    {
      return -1;
    }

  nbits = NBITS(uc);
  uc = (1 << (nbits - 1)) - 1;
  ret = stdc_bit_ceil_uc(uc);
  ret2 = stdc_bit_ceil(uc);
  if (ret != uc + 1 || ret2 != uc + 1)
    {
      return -1;
    }

  us = 5;
  ret = stdc_bit_ceil_us(us);
  ret2 = stdc_bit_ceil(us);
  if (ret != 8 || ret2 != 8)
    {
      return -1;
    }

  nbits = NBITS(us);
  us = (1 << (nbits - 1)) - 1;
  ret = stdc_bit_ceil_us(us);
  ret2 = stdc_bit_ceil(us);
  if (ret != us + 1 || ret2 != us + 1)
    {
      return -1;
    }

  ui = 5;
  ret = stdc_bit_ceil_ui(ui);
  ret2 = stdc_bit_ceil(ui);
  if (ret != 8 || ret2 != 8)
    {
      return -1;
    }

  nbits = NBITS(uc);
  ui = (1U << (nbits - 1)) - 1;
  ret = stdc_bit_ceil_ui(ui);
  ret2 = stdc_bit_ceil(ui);
  if (ret != ui + 1 || ret2 != ui + 1)
    {
      return -1;
    }

  ul = 5;
  ret = stdc_bit_ceil_ul(ul);
  ret2 = stdc_bit_ceil(ul);
  if (ret != 8 || ret2 != 8)
    {
      return -1;
    }

  nbits = NBITS(ul);
  ul = (1UL << (nbits - 1)) - 1;
  ret = stdc_bit_ceil_ul(ul);
  ret2 = stdc_bit_ceil(ul);
  if (ret != ul + 1 || ret2 != ul + 1)
    {
      return -1;
    }

  ull = 5;
  ret = stdc_bit_ceil_ull(ull);
  ret2 = stdc_bit_ceil(ull);
  if (ret != 8 || ret2 != 8)
    {
      return -1;
    }

  nbits = NBITS(ull);
  ull = (1ULL << (nbits - 1)) - 1;
  ret = stdc_bit_ceil_ull(ull);
  ret2 = stdc_bit_ceil(ull);
  if (ret != ull + 1 || ret2 != ull + 1)
    {
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const FAR struct test_entry_s *item = NULL;
  int i;
  int ret;
  int tests_ok;
  int tests_err;

  tests_ok = tests_err = 0;

  for (i = 0; i < sizeof(g_entry_list) / sizeof(g_entry_list[0]); i++)
    {
      item = &g_entry_list[i];
      ret = item->entry();
      if (ret < 0)
        {
          tests_err++;
          printf("stdbit_tests: %s test failed.\n", item->name);
        }
      else
        {
          tests_ok++;
        }
    }

  /* Run tests. They should return 0 on success, -1 otherwise. */

  printf("stdbit tests: SUCCESSFUL: %d; FAILED: %d\n", tests_ok,
          tests_err);

  return 0;
}
