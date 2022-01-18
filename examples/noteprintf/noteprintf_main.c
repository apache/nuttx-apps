/****************************************************************************
 * apps/examples/noteprintf/noteprintf_main.c
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

#include "nuttx/sched_note.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAIN_MODULE NOTE_MODULE('m', 'a', 'i', 'n')

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * noteprintf_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct binary
    {
      uint8_t val1;
      uint8_t val2;
    } binary =
      {
        .val1        = 0x01,
        .val2        = 0x02
      };

  char *str      = "shced note test";
  int count      = 0;
  char c         = 1;
  short s        = 2;
  int i          = 3;
  long l         = 4;
#ifdef CONFIG_LIBC_LONG_LONG
  long long ll   = 5;
#endif
  intmax_t im    = 6;
  size_t sz      = 7;
  ptrdiff_t ptr  = 8;
#ifdef CONFIG_HAVE_FLOAT
  float f        = 0.1;
#endif
#ifdef CONFIG_HAVE_DOUBLE
  double d       = 0.2;
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
  long double ld = 0.3;
#endif

  while (1)
    {
      sched_note_printf("shced note test count = %d.", count++);
      sched_note_string(str);
      sched_note_dump(MAIN_MODULE, 1, &binary, sizeof(struct binary));
      sched_note_bprintf(MAIN_MODULE, 2, "%hhd", c);
      sched_note_bprintf(MAIN_MODULE, 3, "%hd", s);
      sched_note_bprintf(MAIN_MODULE, 4, "%d", i);
      sched_note_bprintf(MAIN_MODULE, 5, "%ld", l);
      sched_note_bprintf(MAIN_MODULE, 6, "%lld", ll);
      sched_note_bprintf(MAIN_MODULE, 7, "%jd", im);
      sched_note_bprintf(MAIN_MODULE, 8, "%zd", sz);
      sched_note_bprintf(MAIN_MODULE, 9, "%td", ptr);
#ifdef CONFIG_HAVE_FLOAT
      sched_note_bprintf(MAIN_MODULE, 10, "%e", f);
#endif
#ifdef CONFIG_HAVE_DOUBLE
      sched_note_bprintf(MAIN_MODULE, 11, "%le", d);
#endif
#ifdef CONFIG_HAVE_LONG_DOUBLE
      sched_note_bprintf(MAIN_MODULE, 12, "%Le", ld);
#endif
      sched_note_bprintf(MAIN_MODULE, 13,
                         "%hhd  %hd  %d  %ld  %lld  %jd  %zd  %td",
                          c,    s,   i,  l,    ll,  im,  sz,  ptr);
      usleep(10);
    }

  return 0;
}
