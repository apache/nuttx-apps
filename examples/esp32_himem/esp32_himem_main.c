/****************************************************************************
 * apps/examples/esp32_himem/esp32_himem_main.c
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
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include <nuttx/himem/himem.h>

/* Fills the memory in 32-bit words for speed. */

static void fill_mem(void *mem, int len)
{
    uint32_t *p = (uint32_t *)mem;
    unsigned int val = 0xa5a5a5a5;
    for (int i = 0; i < len / 4; i++)
      {
        *p++ = val;
      }
}

/* Check the memory filled by fill_mem. Returns true if the data matches
 * the data that fill_mem wrote.
 * Returns true if there's a match, false when the region differs from what
 * should be there.
 */

static bool check_mem(void *mem, int len, int phys_addr)
{
    uint32_t *p = (uint32_t *)mem;
    unsigned int val = 0xa5a5a5a5;
    for (int i = 0; i < len / 4; i++)
      {
        if (val != *p)
          {
            printf("check_mem: %x has 0x%08x expected 0x%08x\n",
                   phys_addr + ((char *) p - (char *) mem), *p, val);
            return false;
          }
        p++;
      }
    return true;
}

/* Allocate a himem region, fill it with data, check it and release it. */

static int test_region(int fd, struct esp_himem_par *param)
{
  int ret;
  int i;

  DEBUGASSERT(param != NULL);

  /* Set predefined parameters */

  param->len = ESP_HIMEM_BLKSZ;
  param->range_offset = 0;
  param->flags = 0;

  /* Allocate the memory we're going to check. */

  ret = ioctl(fd, HIMEMIOC_ALLOC_BLOCKS, (unsigned long)((uintptr_t)param));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(ALLOC_BLOCKS) failed: %d\n", errno);
      return -ENOMEM;
    }

  /* Allocate a block of address range */

  ret = ioctl(fd, HIMEMIOC_ALLOC_MAP_RANGE,
              (unsigned long)((uintptr_t)param));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(ALLOC_MAP_RANGE) failed: %d\n", errno);
      return -ENOMEM;
    }

  for (i = 0; i < param->memfree; i += ESP_HIMEM_BLKSZ)
    {
      param->ptr = NULL;
      param->ram_offset = i;

      /* Map in block, write pseudo-random data, unmap block. */

      ret = ioctl(fd, HIMEMIOC_MAP, (unsigned long)((uintptr_t)param));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: ioctl(MAP) failed: %d\n", errno);
          ret = -ENOMEM;
          goto free_rammem;
        }

      fill_mem(param->ptr, ESP_HIMEM_BLKSZ);

      ret = ioctl(fd, HIMEMIOC_UNMAP, (unsigned long)((uintptr_t)param));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: ioctl(UNMAP) failed: %d\n", errno);
          ret = -ENOMEM;
          goto free_rammem;
        }
    }

  /* give the OS some time to do things so the task watchdog doesn't bark */

  usleep(1);

  for (i = 0; i < param->memfree; i += ESP_HIMEM_BLKSZ)
    {
      param->ptr = NULL;
      param->ram_offset = i;

      /* Map in block, check against earlier written pseudo-random data,
       * unmap block.
       */

      ret = ioctl(fd, HIMEMIOC_MAP, (unsigned long)((uintptr_t)param));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: ioctl(MAP) failed: %d\n", errno);
          ret = -ENOMEM;
          goto free_rammem;
        }

      if (!check_mem(param->ptr, ESP_HIMEM_BLKSZ, i))
        {
          printf("Error in block %d\n", i / ESP_HIMEM_BLKSZ);
          ret = -ENOMEM;
          goto free_rammem;
        }

      ret = ioctl(fd, HIMEMIOC_UNMAP, (unsigned long)((uintptr_t)param));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: ioctl(UNMAP) failed: %d\n", errno);
          ret = -ENOMEM;
          goto free_rammem;
        }
    }

  /* Free allocated memory */

free_rammem:
  ret = ioctl(fd, HIMEMIOC_FREE_MAP_RANGE,
              (unsigned long)((uintptr_t)param));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(FREE_MAP_RANGE) failed: %d\n", errno);
    }

free_memaddr:
  ret = ioctl(fd, HIMEMIOC_FREE_BLOCKS, (unsigned long)((uintptr_t)param));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(FREE_BLOCKS) failed: %d\n", errno);
    }

    return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * esp32_himem_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct esp_himem_par param;
  int ret;
  int fd;

  fd = open("/dev/himem", O_RDONLY);
  if (fd < 0)
    {
      printf("Failed to open /dev/himem, error = %d!\n", errno);
      return -ENODEV;
    }

  ret = ioctl(fd, HIMEMIOC_GET_PHYS_SIZE, &param);
  if (ret < 0)
    {
      printf("Failed to run ioctl HIMEMIOC_GET_PHYS_SIZE, error = %d!\n",
             errno);
      close(fd);
      return -ENOMEM;
    }

  ret = ioctl(fd, HIMEMIOC_GET_FREE_SIZE, &param);
  if (ret < 0)
    {
      printf("Failed to run ioctl HIMEMIOC_GET_FREE_SIZE, error = %d!\n",
             errno);
      close(fd);
      return -ENOMEM;
    }

  printf("Himem has %dKiB of memory, %dKiB of which is free.\
          \nTesting the free memory...\n", (int) param.memcnt / 1024,
          (int) param.memfree / 1024);

  test_region(fd, &param);
  printf("Done!\n");
}

