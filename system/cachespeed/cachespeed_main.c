/****************************************************************************
 * apps/system/cachespeed/cachespeed_main.c
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

#include <nuttx/arch.h>
#include <nuttx/cache.h>
#include <nuttx/irq.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CACHESPEED_PREFIX "CACHE Speed: "

#define GET_VALUE(value, type) \
  do \
  { \
   FAR char *ptr; \
   value = (type)strtoul(optarg, &ptr, 0); \
   if (*ptr != '\0') \
    { \
      printf(CACHESPEED_PREFIX "Parameter error: -%c %s\n", option, optarg); \
      show_usage(argv[0], EXIT_FAILURE); \
    } \
  } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct cachespeed_s
{
  FAR void *begin;
  size_t memset_size;
  uint32_t repeat_num;
  size_t opt_size;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
{
  printf("\nUsage: %s -b <address>  -o <operation size>"
         "  -s <memset size>[262144] -n <repeat number>[100] \n",
         progname);
  printf("\nWhere:\n");
  printf("  -b <hex-address> begin memset address.\n");
  printf("  -o <operation size> The size of the operation.\n");
  printf("  -s <memset size> Execute memset size (in bytes)."
         "  [default value: 262144].\n");
  printf("  -n <repeat num> number of repetitions"
         " [default value: 1000].\n");
  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(int argc, FAR char **argv,
                              FAR struct cachespeed_s *info)
{
  int option;

  memset(info, 0, sizeof(struct cachespeed_s));
  info->repeat_num = 1000;
  info->memset_size = 262144;

  while ((option = getopt(argc, argv, "b:o:s:n:")) != ERROR)
    {
      switch (option)
        {
          case 'b':
            GET_VALUE(info->begin, void *);
            break;
          case 'o':
            GET_VALUE(info->opt_size, size_t);
            break;
          case 's':
            GET_VALUE(info->memset_size, size_t);
            break;
          case 'n':
            GET_VALUE(info->repeat_num, uint32_t);
            if (info->repeat_num == 0)
              {
                printf(CACHESPEED_PREFIX "<repeat number> must > 0\n");
                exit(EXIT_FAILURE);
              }
            break;
          case '?':
            printf(CACHESPEED_PREFIX "Unknown option: %c\n", optopt);
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }

    if (info->opt_size == 0 || info->begin == 0)
      {
        printf(CACHESPEED_PREFIX "Missing required arguments\n");
        show_usage(argv[0], EXIT_FAILURE);
      }
}

/****************************************************************************
 * Name: get_perf_time
 ****************************************************************************/

static uint32_t get_perf_time(void)
{
  return up_perf_gettime();
}

/****************************************************************************
 * Name: get_time_elaps
 ****************************************************************************/

static uint32_t get_time_elaps(uint32_t prev_time)
{
  return get_perf_time() - prev_time;
}

/****************************************************************************
 * Name: print_result
 ****************************************************************************/

static void print_result(FAR const char *name, size_t bytes,
                         uint32_t cost_time, uint32_t repeat_cnt)
{
  uint32_t rate;
  struct timespec ts;

  /* Converted to ns */

  up_perf_convert(cost_time, &ts);
  cost_time = ts.tv_sec * 1000000000 + ts.tv_nsec;

  if (cost_time / 1000000 == 0)
    {
      printf(CACHESPEED_PREFIX
             "The total overhead time in millisecond precision"
             "is too short.\n");
    }

  /* rate  = (bytes / 1024) / (cost_time / 1000000000) */

  rate = (uint64_t)bytes * 1000000000 / cost_time / 1024;
  printf(CACHESPEED_PREFIX
         "%s avg = %"PRIu32 " ns\t Rate: %" PRIu32 " KB/s\t"
         "[cost = %" PRIu32 " ms]\n",
         name, cost_time / repeat_cnt, rate, cost_time / 1000000);
}

/****************************************************************************
 * Name: dcache_speed_test
 ****************************************************************************/

static void dcache_speed_test(FAR void *begin, size_t memset_size,
                              size_t opt_size, uint32_t repeat_cnt)
{
  size_t total_size = memset_size * repeat_cnt;
  uint32_t invalidate_cost_time;
  uint32_t clean_cost_time;
  uint32_t flush_cost_time;
  uint32_t cnt;
  uint32_t pt;
  irqstate_t flags;

  /* Initialize a variable */

  invalidate_cost_time = 0;
  clean_cost_time = 0;
  flush_cost_time = 0;

  /* Accumulate the time to get the total time */

  printf("______dcache performance______\n");

  printf("______do all operation______\n");
  flags = enter_critical_section();
  for (cnt = 0; cnt < repeat_cnt; cnt++)
    {
      uint32_t start_time;
      memset(begin, 0, memset_size);
      start_time = get_perf_time();
      up_clean_dcache_all();
      clean_cost_time += get_time_elaps(start_time);

      memset(begin, 0, memset_size);
      start_time = get_perf_time();
      up_flush_dcache_all();
      flush_cost_time += get_time_elaps(start_time);
    }

  leave_critical_section(flags);
  print_result("clean dcache():\t", total_size, clean_cost_time, repeat_cnt);
  print_result("flush dcache():\t", total_size, flush_cost_time, repeat_cnt);

  for (pt = 32; pt <= opt_size; pt <<= 1)
    {
      total_size =  pt * repeat_cnt;
      invalidate_cost_time = 0;
      clean_cost_time = 0;
      flush_cost_time = 0;

      if (pt < 1024)
        {
          printf("______do %" PRIu32 " B operation______\n", pt);
        }
      else
        {
          printf("______do %" PRIu32  " KB operation______\n", pt / 1024);
        }

      flags = enter_critical_section();
      for (cnt = 0; cnt < repeat_cnt; cnt++)
        {
          uint32_t start_time;
          memset(begin, 0, memset_size);
          start_time = get_perf_time();
          up_invalidate_dcache((uintptr_t)begin,
                               (uintptr_t)((uint8_t *)begin + pt));
          invalidate_cost_time += get_time_elaps(start_time);

          memset(begin, 0, memset_size);
          start_time = get_perf_time();
          up_clean_dcache((uintptr_t)begin,
                          (uintptr_t)((uint8_t *)begin + pt));
          clean_cost_time += get_time_elaps(start_time);

          memset(begin, 0, memset_size);
          start_time = get_perf_time();
          up_flush_dcache((uintptr_t)begin,
                          (uintptr_t)((uint8_t *)begin + pt));
          flush_cost_time += get_time_elaps(start_time);
        }

      leave_critical_section(flags);
      print_result("invalidate dcache():\t",
                   total_size, invalidate_cost_time, repeat_cnt);
      print_result("clean dcache():\t", total_size, clean_cost_time,
                    repeat_cnt);
      print_result("flush dcache():\t", total_size, flush_cost_time,
                    repeat_cnt);
    }
}

/****************************************************************************
 * Name: icache_speed_test
 ****************************************************************************/

static void icache_speed_test(FAR void *begin, size_t memset_size,
                              size_t opt_size, uint32_t repeat_cnt)
{
  irqstate_t flags;
  int32_t cnt;
  uint32_t pt;
  uint32_t invalidate_cost_time = 0;

  /* Accumulate the time to get the total time */

  printf("______icache performance______\n");

  printf("______do all operation______\n");
  flags = enter_critical_section();
  for (cnt = 0; cnt < repeat_cnt; cnt++)
    {
      uint32_t start_time;
      memset(begin, 0, memset_size);
      start_time = get_perf_time();
      up_invalidate_icache_all();
      invalidate_cost_time += get_time_elaps(start_time);
    }

  leave_critical_section(flags);
  print_result("invalidate dcache():\t",
               memset_size * repeat_cnt, invalidate_cost_time, repeat_cnt);

  for (pt = 32; pt <= opt_size; pt <<= 1)
    {
      const size_t total_size =  pt * repeat_cnt;
      invalidate_cost_time = 0;
      if (pt < 1024)
        {
          printf("______do %" PRIu32 " B operation______\n", pt);
        }
      else
        {
          printf("______do %" PRIu32  " KB operation______\n", pt / 1024);
        }

      flags = enter_critical_section();
      for (cnt = 0; cnt < repeat_cnt; cnt++)
        {
          uint32_t start_time;
          memset(begin, 0, memset_size);
          start_time = get_perf_time();
          up_invalidate_icache((uintptr_t)begin,
                               (uintptr_t)((uint8_t *)begin + pt));
          invalidate_cost_time += get_time_elaps(start_time);
        }

      leave_critical_section(flags);
      print_result("invalidate icache():\t",
                   total_size, invalidate_cost_time, repeat_cnt);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cachespeed_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct cachespeed_s cachespeed;

  /* Setup defaults and parse the command line */

  parse_commandline(argc, argv, &cachespeed);

  /* Perform the dcache and icache test */

  dcache_speed_test(cachespeed.begin, cachespeed.memset_size,
                    cachespeed.opt_size, cachespeed.repeat_num);

  icache_speed_test(cachespeed.begin, cachespeed.memset_size,
                    cachespeed.opt_size, cachespeed.repeat_num);

  return 0;
}
