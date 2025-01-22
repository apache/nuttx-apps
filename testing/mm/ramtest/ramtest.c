/****************************************************************************
 * apps/testing/mm/ramtest/ramtest.c
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

#include <sys/types.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <sched.h>
#include <syslog.h>
#include <errno.h>

#include <nuttx/usb/usbdev_trace.h>

#ifdef CONFIG_TESTING_RAMTEST

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RAMTEST_PREFIX "RAMTest: "

#define OPTARG_TO_VALUE(value, type) \
  do \
  { \
    FAR char *ptr; \
    value = (type)strtoul(optarg, &ptr, 0); \
    if (*ptr != '\0') \
      { \
        printf(RAMTEST_PREFIX "Parameter error: -%c %s\n", option, optarg); \
        show_usage(argv[0], EXIT_FAILURE); \
      } \
  } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ramtest_s
{
  uint8_t width;
  uintptr_t start;
  size_t size;
  size_t nxfrs;
  uint32_t mask;
  bool free;
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
  printf("\nUsage: %s [-w|h|b] -a [hex-address] -s <decimal-size>\n",
         progname);
  printf("\nWhere:\n");
  printf("  -a [hex-address] starting address of the test,\n");
  printf("  -s <decimal-size> number of memory locations (in bytes).\n");
  printf("  -w Sets the width of a memory location to 32-bits.\n");
  printf("  -h Sets the width of a memory location to 16-bits (default).\n");
  printf("  -b Sets the width of a memory location to 8-bits.\n");
  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(int argc, char **argv,
                              FAR struct ramtest_s *info)
{
  int option;

  while ((option = getopt(argc, argv, "whba::s:")) != ERROR)
    {
      switch (option)
        {
          case 'w':
            info->width = 32;
            info->mask  = 0xffffffff;
            break;
          case 'h':
            info->width = 16;
            info->mask  = 0x0000ffff;
            break;
          case 'b':
            info->width = 8;
            info->mask  = 0x000000ff;
            break;
          case 'a':
            OPTARG_TO_VALUE(info->start, uintptr_t);
            break;
          case 's':
            OPTARG_TO_VALUE(info->size, size_t);
            break;
          case '?':
            printf(RAMTEST_PREFIX "Unrecognized option: '%c'\n", option);
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }

  /* There should be one parameters remaining on the command line */

  if (info->size == 0)
    {
      printf(RAMTEST_PREFIX "Missing <decimal-size>\n");
      show_usage(argv[0], EXIT_FAILURE);
    }

  /* If no binary address is specified, then the operation is performed by
   * allocation from the heap.
   */

  if (!info->start)
    {
      info->start = (uintptr_t)malloc(info->size);
      if (!info->start)
        {
          printf(RAMTEST_PREFIX "malloc failed, We were unable"
                 "to continue with the follow-up work\n");
          show_usage(argv[0], EXIT_FAILURE);
        }

      info->free = true;
    }

  /* Convert the size (in bytes) to the corresponding number of transfers
   * of the selected width.
   */

  if (info->width == 8)
    {
      info->nxfrs = info->size;
    }
  else if (info->width == 32)
    {
      info->nxfrs = info->size >> 2;
    }
  else
    {
      info->width = 16;
      info->nxfrs = info->size >> 1;
    }
}

/****************************************************************************
 * Name: write_memory
 ****************************************************************************/

static void write_memory(FAR struct ramtest_s *info, uint32_t value)
{
  size_t i;

  if (info->width == 32)
    {
      uint32_t *ptr = (uint32_t *)info->start;
      for (i = 0; i < info->nxfrs; i++)
        {
          *ptr++ = value;
        }
    }
  else if (info->width == 16)
    {
      uint16_t *ptr = (uint16_t *)info->start;
      for (i = 0; i < info->nxfrs; i++)
        {
          *ptr++ = (uint16_t)value;
        }
    }
  else /* if (info->width == 8) */
    {
      uint8_t *ptr = (uint8_t *)info->start;
      for (i = 0; i < info->nxfrs; i++)
        {
          *ptr++ = (uint8_t)value;
        }
    }
}

/****************************************************************************
 * Name: verify_memory
 ****************************************************************************/

static void verify_memory(FAR struct ramtest_s *info, uint32_t value)
{
  size_t i;

  if (info->width == 32)
    {
      uint32_t *ptr = (uint32_t *)info->start;
      for (i = 0; i < info->nxfrs; i++)
        {
          if (*ptr != value)
            {
              printf(RAMTEST_PREFIX
                     "ERROR: Address %p Found: %08" PRIx32
                     " Expected %08" PRIx32 "\n",
                     ptr, *ptr, value);
            }

          ptr++;
        }
    }
  else if (info->width == 16)
    {
      uint16_t value16 = (uint16_t)(value & 0x0000ffff);
      uint16_t *ptr = (uint16_t *)info->start;
      for (i = 0; i < info->nxfrs; i++)
        {
          if (*ptr != value16)
            {
              printf(RAMTEST_PREFIX
                     "ERROR: Address %p Found: %04" PRIx16
                     " Expected %04" PRIx16 "\n",
                     ptr, *ptr, value16);
            }

          ptr++;
        }
    }
  else /* if (info->width == 8) */
    {
      uint8_t value8 = (uint8_t)(value & 0x000000ff);
      uint8_t *ptr = (uint8_t *)info->start;
      for (i = 0; i < info->nxfrs; i++)
        {
          if (*ptr != value8)
            {
              printf(RAMTEST_PREFIX
                     "ERROR: Address %p Found: %02" PRIx8
                     " Expected %02" PRIx8 "\n",
                     ptr, *ptr, value8);
            }

          ptr++;
        }
    }
}

/****************************************************************************
 * Name: marching_ones
 ****************************************************************************/

static void marching_ones(FAR struct ramtest_s *info)
{
  uint32_t pattern = 0x00000001;

  printf(RAMTEST_PREFIX "Marching ones: %08" PRIxPTR " %zu\n",
         info->start, info->size);

  while (pattern != 0)
    {
      write_memory(info, pattern);
      verify_memory(info, pattern);
      pattern <<= 1;
      pattern &= info->mask;
    }
}

/****************************************************************************
 * Name: marching_zeros
 ****************************************************************************/

static void marching_zeros(FAR struct ramtest_s *info)
{
  uint32_t pattern = 0xfffffffe;

  printf(RAMTEST_PREFIX "Marching zeroes: %08" PRIxPTR " %zu\n",
         info->start, info->size);

  while (pattern != 0xffffffff)
    {
      write_memory(info, pattern);
      verify_memory(info, pattern);
      pattern <<= 1;
      pattern |= 1;
      pattern |= ~info->mask;
    }
}

/****************************************************************************
 * Name: write_memory2
 ****************************************************************************/

static void write_memory2(FAR struct ramtest_s *info, uint32_t value_1,
                          uint32_t value_2)
{
  size_t even_nxfrs = info->nxfrs & ~1;
  size_t i;

  if (info->width == 32)
    {
      uint32_t *ptr = (uint32_t *)info->start;
      for (i = 0; i < even_nxfrs; i += 2)
        {
          *ptr++ = value_1;
          *ptr++ = value_2;
        }
    }
  else if (info->width == 16)
    {
      uint16_t *ptr = (uint16_t *)info->start;
      for (i = 0; i < even_nxfrs; i += 2)
        {
          *ptr++ = (uint16_t)value_1;
          *ptr++ = (uint16_t)value_2;
        }
    }
  else /* if (info->width == 8) */
    {
      uint8_t *ptr = (uint8_t *)info->start;
      for (i = 0; i < even_nxfrs; i += 2)
        {
          *ptr++ = (uint8_t)value_1;
          *ptr++ = (uint8_t)value_2;
        }
    }
}

/****************************************************************************
 * Name: verify_memory2
 ****************************************************************************/

static void verify_memory2(FAR struct ramtest_s *info, uint32_t value_1,
                           uint32_t value_2)
{
  size_t even_nxfrs = info->nxfrs & ~1;
  size_t i;

  if (info->width == 32)
    {
      uint32_t *ptr = (uint32_t *)info->start;
      for (i = 0; i < even_nxfrs; i += 2)
        {
          if (ptr[0] != value_1 || ptr[1] != value_2)
            {
              printf(RAMTEST_PREFIX
                     "ERROR: Address %p Found: %08" PRIx32
                     " and %08" PRIx32 "\n",
                     ptr, ptr[0], ptr[1]);
              printf(RAMTEST_PREFIX
                     "               Expected: %08" PRIx32
                     " and %08" PRIx32 "\n",
                     value_1, value_2);
            }

          ptr += 2;
        }
    }
  else if (info->width == 16)
    {
      uint16_t value16_1 = (uint16_t)(value_1 & 0x0000ffff);
      uint16_t value16_2 = (uint16_t)(value_2 & 0x0000ffff);
      uint16_t *ptr = (uint16_t *)info->start;
      for (i = 0; i < even_nxfrs; i += 2)
        {
          if (ptr[0] != value16_1 || ptr[1] != value16_2)
            {
              printf(RAMTEST_PREFIX
                     "ERROR: Address %p Found: %04" PRIx16
                     " and %04" PRIx16 "\n",
                     ptr, ptr[0], ptr[1]);
              printf(RAMTEST_PREFIX
                     "               Expected: %04" PRIx16
                     " and %04" PRIx16 "\n",
                     value16_1, value16_2);
            }

          ptr += 2;
        }
    }
  else /* if (info->width == 8) */
    {
      uint8_t value8_1 = (uint8_t)(value_1 & 0x000000ff);
      uint8_t value8_2 = (uint8_t)(value_2 & 0x000000ff);
      uint8_t *ptr = (uint8_t *)info->start;
      for (i = 0; i < even_nxfrs; i += 2)
        {
          if (ptr[0] != value8_1 || ptr[1] != value8_2)
            {
              printf(RAMTEST_PREFIX
                     "ERROR: Address %p Found: %02" PRIx8
                     " and %02" PRIx8 "\n",
                     ptr, ptr[0], ptr[1]);
              printf(RAMTEST_PREFIX
                     "               Expected: %02" PRIx8
                     " and %02" PRIx8 "\n",
                     value8_1, value8_2);
            }

          ptr += 2;
        }
    }
}

/****************************************************************************
 * Name: pattern_test
 ****************************************************************************/

static void pattern_test(FAR struct ramtest_s *info, uint32_t pattern1,
                         uint32_t pattern2)
{
  printf(RAMTEST_PREFIX "Pattern test: %08" PRIxPTR " %zu %08" PRIx32
         " %08" PRIx32 "\n",
         info->start, info->size, pattern1, pattern2);

  write_memory2(info, pattern1, pattern2);
  verify_memory2(info, pattern1, pattern2);
}

/****************************************************************************
 * Name: write_addrinaddr
 ****************************************************************************/

static void write_addrinaddr(FAR struct ramtest_s *info)
{
  size_t i;

  if (info->width == 32)
    {
      uint32_t *ptr = (uint32_t *)info->start;
      for (i = 0; i < info->nxfrs; i++)
        {
          uint32_t value32 = (uint32_t)((uintptr_t)ptr);
          *ptr++ = value32;
        }
    }
  else if (info->width == 16)
    {
      uint16_t *ptr = (uint16_t *)info->start;
      for (i = 0; i < info->nxfrs; i++)
        {
          uint16_t value16 = (uint16_t)((uintptr_t)ptr & 0x0000ffff);
          *ptr++ = value16;
        }
    }
  else /* if (info->width == 8) */
    {
      uint8_t *ptr = (uint8_t *)info->start;
      for (i = 0; i < info->nxfrs; i++)
        {
          uint8_t value8 = (uint8_t)((uintptr_t)ptr & 0x000000ff);
          *ptr++ = value8;
        }
    }
}

/****************************************************************************
 * Name: verify_addrinaddr
 ****************************************************************************/

static void verify_addrinaddr(FAR struct ramtest_s *info)
{
  size_t i;

  if (info->width == 32)
    {
      uint32_t *ptr = (uint32_t *)info->start;
      for (i = 0; i < info->nxfrs; i++)
        {
          uint32_t value32 = (uint32_t)((uintptr_t)ptr);
          if (*ptr != value32)
            {
              printf(RAMTEST_PREFIX
                     "ERROR: Address %p Found: %08" PRIx32
                     " Expected %08" PRIx32 "\n",
                     ptr, *ptr, value32);
            }

          ptr++;
        }
    }
  else if (info->width == 16)
    {
      uint16_t *ptr = (uint16_t *)info->start;
      for (i = 0; i < info->nxfrs; i++)
        {
          uint16_t value16 = (uint16_t)((uintptr_t)ptr & 0x0000ffff);
          if (*ptr != value16)
            {
              printf(RAMTEST_PREFIX
                     "ERROR: Address %p Found: %04" PRIx16
                     " Expected %04" PRIx16 "\n",
                     ptr, *ptr, value16);
            }

          ptr++;
        }
    }
  else /* if (info->width == 8) */
    {
      uint8_t *ptr = (uint8_t *)info->start;
      for (i = 0; i < info->nxfrs; i++)
        {
          uint16_t value8 = (uint8_t)((uintptr_t)ptr & 0x000000ff);
          if (*ptr != value8)
            {
              printf(RAMTEST_PREFIX
                     "ERROR: Address %p Found: %02" PRIx8
                     " Expected %02" PRIx8 "\n",
                     ptr, *ptr, value8);
            }

          ptr++;
        }
    }
}

/****************************************************************************
 * Name: addr_in_addr
 ****************************************************************************/

static void addr_in_addr(FAR struct ramtest_s *info)
{
  printf(RAMTEST_PREFIX "Address-in-address test: %08" PRIxPTR" %zu\n",
         info->start, info->size);

  write_addrinaddr(info);
  verify_addrinaddr(info);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ramtest_s info;

  /* Setup defaults and parse the command line */

  info.free  = false;
  info.width = 16;
  info.mask  = 0x0000ffff;
  info.size  = 0;
  info.start = 0;
  parse_commandline(argc, argv, &info);

  /* Perform the memory tests */

  marching_ones(&info);
  marching_zeros(&info);
  pattern_test(&info, 0x55555555, 0xaaaaaaaa);
  pattern_test(&info, 0x66666666, 0x99999999);
  pattern_test(&info, 0x33333333, 0xcccccccc);
  addr_in_addr(&info);

  /* Let's check if we need to do cleanup work at the end */

  if (info.free)
    {
      free((void *)info.start);
    }

  return 0;
}

#endif /* CONFIG_TESTING_RAMTEST */
