/****************************************************************************
 * apps/benchmarks/ramspeed/ramspeed_main.c
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

#define RAMSPEED_PREFIX "RAM Speed: "

#if defined(UINTPTR_MAX) && UINTPTR_MAX > 0xFFFFFFFF
#  define MEM_UNIT         uint64_t
#  define ALIGN_MASK       0x7
#else
#  define MEM_UNIT         uint32_t
#  define ALIGN_MASK       0x3
#endif

#define COPY32 *d32 = *s32; d32++; s32++;
#define COPY8 *d8 = *s8; d8++; s8++;
#define SET32(x) *d32 = x; d32++;
#define SET8(x) *d8 = x; d8++;
#define REPEAT8(expr) expr expr expr expr expr expr expr expr

#define OPTARG_TO_VALUE(value, type, base) \
  do \
  { \
    FAR char *ptr; \
    value = (type)strtoul(optarg, &ptr, base); \
    if (*ptr != '\0') \
      { \
        printf(RAMSPEED_PREFIX "Parameter error: -%c %s\n", ch, optarg); \
        show_usage(argv[0], EXIT_FAILURE); \
      } \
  } while (0)

  #define HAS_IRQ_CONTROL !defined(CONFIG_BUILD_KERNEL) && \
                          !defined(CONFIG_BUILD_PROTECTED)

  #if HAS_IRQ_CONTROL
  #  define ENABLE_IRQ(flags) leave_critical_section(flags);
  #  define DISABLE_IRQ(flags) flags=enter_critical_section();
  #else
  #  define ENABLE_IRQ(flags) (void)flags;
  #  define DISABLE_IRQ(flags) (void)flags;
  #endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ramspeed_s
{
  FAR void *dest;
  FAR const void *src;
  size_t size;
  uint8_t value;
  uint32_t repeat_num;
  bool irq_disable;
  bool allocate_rw_address;
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
  printf("\nUsage: %s -a -r <hex-address> -w <hex-address> -s <decimal-size>"
         " -v <hex-value>[0x00] -n <decimal-repeat number>[100] -i\n",
         progname);
  printf("\nWhere:\n");
  printf("  -a allocate RW buffers on heap. Overwrites -r and -w option.\n");
  printf("  -r <hex-address> read address.\n");
  printf("  -w <hex-address> write address.\n");
  printf("  -s <decimal-size> number of memory locations (in bytes).\n");
  printf("  -v <hex-value> value to fill in memory"
         " [default value: 0x00].\n");
  printf("  -n <decimal-repeat num> number of repetitions"
         " [default value: 100].\n");
  #if HAS_IRQ_CONTROL
  printf("  -i turn off interrupts while testing"
         " [default value: false].\n");
  #endif
  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(int argc, FAR char **argv,
                              FAR struct ramspeed_s *info)
{
  int ch;

  memset(info, 0, sizeof(struct ramspeed_s));
  info->repeat_num = 100;

  if (argc < 4)
    {
      printf(RAMSPEED_PREFIX "Missing required arguments\n");
      show_usage(argv[0], EXIT_FAILURE);
    }

  while ((ch = getopt(argc, argv, "r:w:s:v:n:ia")) != ERROR)
    {
      switch (ch)
        {
          case 'a':
            info->allocate_rw_address = true;
            break;
          case 'r':
            OPTARG_TO_VALUE(info->src, const void *, 16);
            if (((uintptr_t)info->src & ALIGN_MASK) != 0)
              {
                printf(RAMSPEED_PREFIX "<read-adress> must align %p\n",
                       info->src);
                exit(EXIT_FAILURE);
              }

            break;
          case 'w':
            OPTARG_TO_VALUE(info->dest, void *, 16);
            if (((uintptr_t)info->dest & ALIGN_MASK) != 0)
              {
                printf(RAMSPEED_PREFIX "<write-adress> must align %p\n",
                       info->dest);
                exit(EXIT_FAILURE);
              }
            break;
          case 's':
            OPTARG_TO_VALUE(info->size, size_t, 10);
            if (info->size < 32)
              {
                printf(RAMSPEED_PREFIX "<size> must >= 32\n");
                exit(EXIT_FAILURE);
              }

            break;
          case 'v':
            OPTARG_TO_VALUE(info->value, uint8_t, 16);
            break;
          case 'n':
            OPTARG_TO_VALUE(info->repeat_num, uint32_t, 10);
            if (info->repeat_num == 0)
              {
                printf(RAMSPEED_PREFIX "<repeat number> must > 0\n");
                exit(EXIT_FAILURE);
              }

            break;
          #if HAS_IRQ_CONTROL
          case 'i':
            info->irq_disable = true;
            break;
          #endif
          case '?':
            printf(RAMSPEED_PREFIX "Unknown option: %c\n", (char)optopt);
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }

  if (!info->allocate_rw_address && info->dest == NULL)
    {
      /* We allow only set write address to test memset only.
       * But if need test read by specific address, need write address also.
       */

      printf(RAMSPEED_PREFIX "Required Address Failed\n");
      goto out;
    }
  else if (info->allocate_rw_address)
    {
      if (info->size == 0)
        {
          printf(RAMSPEED_PREFIX "Required Size Failed\n");
          goto out;
        }

      /* We need to automatically apply for memory */

      printf(RAMSPEED_PREFIX "Allocate RW buffers on heap\n");
      info->dest = malloc(info->size);
      if (info->dest == NULL)
        {
          printf(RAMSPEED_PREFIX "Dest Alloc Memory Failed!\n");
          goto out;
        }

      info->src = malloc(info->size);
      if (info->src == NULL)
        {
          printf(RAMSPEED_PREFIX "Src Alloc Memory Failed!\n");
          goto out;
        }
    }

  /* Print info */

  printf(RAMSPEED_PREFIX "Write address: %p\n", info->dest);
  printf(RAMSPEED_PREFIX "Read address: %p\n", info->src);
  printf(RAMSPEED_PREFIX "Size: %zu bytes\n", info->size);
  printf(RAMSPEED_PREFIX "Value: 0x%02x\n", info->value);
  printf(RAMSPEED_PREFIX "Repeat number: %" PRIu32 "\n", info->repeat_num);
  printf(RAMSPEED_PREFIX "Interrupts disabled: %s\n",
         info->irq_disable ? "true" : "false");

  return;

out:
    show_usage(argv[0], EXIT_FAILURE);
}

/****************************************************************************
 * Name: get_timestamp
 ****************************************************************************/

static uint32_t get_timestamp(void)
{
  struct timespec ts;
  uint32_t us;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  us = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
  return us;
}

/****************************************************************************
 * Name: get_time_elaps
 ****************************************************************************/

static uint32_t get_time_elaps(uint32_t prev_time)
{
  uint32_t act_time = get_timestamp();

  /* If there is no overflow in sys_time simple subtract */

  if (act_time >= prev_time)
    {
      prev_time = act_time - prev_time;
    }
  else
    {
      prev_time = UINT32_MAX - prev_time + 1;
      prev_time += act_time;
    }

  return prev_time;
}

/****************************************************************************
 * Name: internal_memcpy
 ****************************************************************************/

static void *internal_memcpy(FAR void *dst, FAR const void *src, size_t len)
{
  FAR uint8_t *d8 = dst;
  FAR const uint8_t *s8 = src;

  uintptr_t d_align = (uintptr_t)d8 & ALIGN_MASK;
  uintptr_t s_align = (uintptr_t)s8 & ALIGN_MASK;
  FAR uint32_t *d32;
  FAR const uint32_t *s32;

  /* Byte copy for unaligned memories */

  if (s_align != d_align)
    {
      while (len > 32)
        {
          REPEAT8(COPY8);
          REPEAT8(COPY8);
          REPEAT8(COPY8);
          REPEAT8(COPY8);
          len -= 32;
        }

      while (len)
        {
          COPY8;
          len--;
        }

      return dst;
    }

  /* Make the memories aligned */

  if (d_align)
    {
      d_align = ALIGN_MASK + 1 - d_align;
      while (d_align && len)
        {
          COPY8;
          d_align--;
          len--;
        }
    }

  d32 = (FAR uint32_t *)d8;
  s32 = (FAR uint32_t *)s8;
  while (len > 32)
    {
      REPEAT8(COPY32);
      len -= 32;
    }

  while (len > 4)
    {
      COPY32;
      len -= 4;
    }

  d8 = (FAR uint8_t *)d32;
  s8 = (FAR const uint8_t *)s32;
  while (len)
    {
      COPY8;
      len--;
    }

  return dst;
}

/****************************************************************************
 * Name: internal_memset
 ****************************************************************************/

static void internal_memset(FAR void *dst, uint8_t v, size_t len)
{
  FAR uint8_t *d8 = (FAR uint8_t *)dst;
  uintptr_t d_align = (uintptr_t) d8 & ALIGN_MASK;
  FAR uint32_t v32;
  FAR uint32_t *d32;

  /* Make the address aligned */

  if (d_align)
    {
      d_align = ALIGN_MASK + 1 - d_align;
      while (d_align && len)
        {
          SET8(v);
          len--;
          d_align--;
        }
    }

  v32 = (uint32_t)v + ((uint32_t)v << 8)
        + ((uint32_t)v << 16) + ((uint32_t)v << 24);

  d32 = (FAR uint32_t *)d8;

  while (len > 32)
    {
      REPEAT8(SET32(v32));
      len -= 32;
    }

  while (len > 4)
    {
      SET32(v32);
      len -= 4;
    }

  d8 = (FAR uint8_t *)d32;
  while (len)
    {
      SET8(v);
      len--;
    }
}

/****************************************************************************
 * Name: print_rate
 ****************************************************************************/

static void print_rate(FAR const char *name, uint64_t bytes,
                       uint32_t cost_time)
{
  double rate;
  if (cost_time == 0)
    {
      printf(RAMSPEED_PREFIX
             "Time-consuming is too short,"
             " please increase the <repeat number>\n");
      return;
    }

  rate = (double)bytes / 1024 / (cost_time / 1000000.0);
  printf(RAMSPEED_PREFIX
         "%s Rate = %.3f KB/s\t[cost: %.3f ms]\n",
         name, rate, cost_time / 1000.0f);
}

/****************************************************************************
 * Name: memcpy_speed_test
 ****************************************************************************/

static void memcpy_speed_test(FAR void *dest, FAR const void *src,
                              size_t size, uint32_t repeat_cnt,
                              bool irq_disable)
{
  uint32_t start_time;
  uint32_t cost_time_system;
  uint32_t cost_time_internal;
  uint32_t cnt;
  uint32_t step;
  uint64_t total_size;
  irqstate_t flags = 0;

  printf("______memcpy performance______\n");

  for (step = 32; step <= size; step <<= 1)
    {
      total_size = (uint64_t)step * (uint64_t)repeat_cnt;

      if (step < 1024)
        {
          printf("______Perform %" PRIu32 " Bytes access ______\n", step);
        }
      else
        {
          printf("______Perform %" PRIu32  " KBytes access ______\n",
                 step / 1024);
        }

      if (irq_disable)
        {
          DISABLE_IRQ(flags);
        }

      start_time = get_timestamp();

      for (cnt = 0; cnt < repeat_cnt; cnt++)
        {
          memcpy(dest, src, step);
        }

      cost_time_system = get_time_elaps(start_time);

      start_time = get_timestamp();

      for (cnt = 0; cnt < repeat_cnt; cnt++)
        {
          internal_memcpy(dest, src, step);
        }

      cost_time_internal = get_time_elaps(start_time);

      if (irq_disable)
        {
          ENABLE_IRQ(flags);
        }

      print_rate("system memcpy():\t", total_size, cost_time_system);
      print_rate("internal memcpy():\t", total_size, cost_time_internal);
    }
}

/****************************************************************************
 * Name: memset_speed_test
 ****************************************************************************/

static void memset_speed_test(FAR void *dest, uint8_t value,
                              size_t size, uint32_t repeat_num,
                              bool irq_disable)
{
  uint32_t start_time;
  uint32_t cost_time_system;
  uint32_t cost_time_internal;
  uint32_t cnt;
  uint32_t step;
  uint64_t total_size;
  irqstate_t flags = 0;

  printf("______memset performance______\n");

  for (step = 32; step <= size; step <<= 1)
    {
      total_size = (uint64_t)step * (uint64_t)repeat_num;

      if (step < 1024)
        {
          printf("______Perform %" PRIu32 " Bytes access______\n", step);
        }
      else
        {
          printf("______Perform %" PRIu32  " KBytes access______\n",
                 step / 1024);
        }

      if (irq_disable)
        {
          DISABLE_IRQ(flags);
        }

      start_time = get_timestamp();

      for (cnt = 0; cnt < repeat_num; cnt++)
        {
          memset(dest, value, step);
        }

      cost_time_system = get_time_elaps(start_time);

      start_time = get_timestamp();

      for (cnt = 0; cnt < repeat_num; cnt++)
        {
          internal_memset(dest, value, step);
        }

      cost_time_internal = get_time_elaps(start_time);

      if (irq_disable)
        {
          ENABLE_IRQ(flags);
        }

      print_rate("system memset():\t", total_size, cost_time_system);
      print_rate("internal memset():\t", total_size, cost_time_internal);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ramspeed_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ramspeed_s ramspeed;

  parse_commandline(argc, argv, &ramspeed);

  if (ramspeed.src != NULL)
    {
      memcpy_speed_test(ramspeed.dest, ramspeed.src,
                        ramspeed.size, ramspeed.repeat_num,
                        ramspeed.irq_disable);
    }

  memset_speed_test(ramspeed.dest, ramspeed.value,
                    ramspeed.size, ramspeed.repeat_num,
                    ramspeed.irq_disable);

  /* Check if alloc from heap? */

  if (ramspeed.allocate_rw_address)
    {
      free(ramspeed.dest);
      free((void *)ramspeed.src);
    }

  return EXIT_SUCCESS;
}
