/****************************************************************************
 * apps/testing/arch/himem_test/himem_chardev_test.c
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <nuttx/himem/himem.h>
#include <math.h>

#include <nuttx/config.h>
#include <fcntl.h>
#include <stdbool.h>
#include <assert.h>
#include <arch/esp32/esp32_himem_chardev.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define HIMEM_DEV1 "/dev/chardev1"
#define HIMEM_DEV2 "/dev/chardev2"
#define DATA_OUTPUT 0

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Data check function
 ****************************************************************************/

static void himem_data_check(uint8_t *data1, uint8_t *data2,
                             uint32_t len, uint32_t offset)
{
  uint32_t ok = 0;
  uint32_t ng = 0;

  for (int i = 0; i < len; i++)
    {
      if (data1[i] == data2[i])
        {
           ok++;
        }
        else
        {
          ng++;
        }
    }

  if (ng == 0)
    {
      printf("Data check OK=%d KB\n", ok / 1024);
    }
    else
    {
      printf("Data check NG=%d OK=%d\n", ng, ok);
    }
}

/****************************************************************************
 * Alternate access verification for multiple devices
 ****************************************************************************/

static void himem_multi_dev_check(uint32_t size, uint32_t offset)
{
  int fd1;
  int fd2;
  uint8_t *work_buffer1 = NULL;
  uint8_t *work_buffer2 = NULL;
  uint8_t *work_buffer3 = NULL;

  if (himem_chardev_register(HIMEM_DEV1, size + offset) != 0)
    {
      printf("HIMEM_DEV1 Create failed. \n");
      goto common_exit3;
    }

  if (himem_chardev_register(HIMEM_DEV2, size + offset) != 0)
    {
      printf("HIMEM_DEV2 Create failed. \n");
      goto common_exit3;
    }

  if ((fd1 = open(HIMEM_DEV1, O_RDWR)) < 0)
    {
      printf("HIMEM_DEV1 Open failed. \n");
      goto common_exit2;
    }

  if ((fd2 = open(HIMEM_DEV2, O_RDWR)) < 0)
    {
      printf("HIMEM_DEV1 Open failed. \n");
      goto common_exit1;
    }

  if ((work_buffer1 = malloc(size)) == NULL)
    {
      printf("Allocate failed1");
      goto common_exit;
    }

  if ((work_buffer2 = malloc(size)) == NULL)
    {
      printf("Allocate failed2");
      goto common_exit;
    }

  if ((work_buffer3 = malloc(size)) == NULL)
    {
      printf("Allocate failed2");
      goto common_exit;
    }

  memset(work_buffer1, 0, size);
  memset(work_buffer2, 0, size);
  memset(work_buffer3, 0, size);

  if (size + offset < 1048576)
    {
      printf("Memory Size: %d KB\nHimemSizeTest=%d\n",
              (size + offset) / 1024, size + offset);
    }
  else
    {
      printf("Memory Size: %d MB\nHimemSizeTest=%d\n",
             (size + offset) / (1024 * 1024), size + offset);
    }

  printf("write HIMEM_DEV1 data:");
  for (int i = 0; i < size; i++)
    {
      work_buffer1[i] = i;
#if DATA_OUTPUT
      if (i < 128)
        printf("%d ", work_buffer1[i]);
#endif
    }

  printf("\n");

  if (lseek(fd1, offset, SEEK_SET) < 0)
    {
      printf("HiMem Seek failed. \n");
      goto common_exit;
    }

  if (write(fd1, work_buffer1, size) != size)
    {
      printf("HiMem Write failed. \n");
      goto common_exit;
    }

  printf("write HIMEM_DEV2 data:");
  for (int i = 5; i < size; i++)
    {
      work_buffer3[i - 5] = i;
#if DATA_OUTPUT
      if (i < 128)
        printf("%d ", i);
#endif
    }

  printf("\n");

  if (lseek(fd2, offset, SEEK_SET) < 0)
    {
      printf("HiMem Seek failed. \n");
      goto common_exit;
    }

  if (write(fd2, work_buffer3, size) != size)
    {
      printf("HiMem Write failed. \n");
      goto common_exit;
    }

  /* read dev1 */

  memset(work_buffer2, 0, size);
  if (lseek(fd1, offset, SEEK_SET) < 0)
    {
      printf("HiMem Seek failed. \n");
      goto common_exit;
    }

  if (read(fd1, work_buffer2, size) != size)
    {
      printf("HiMem Read failed. \n");
      goto common_exit;
    }

  printf("read HIMEM_DEV1 data:");
#if DATA_OUTPUT
  for (int j = 0; j < 128; j++)
    {
      printf("%d ", work_buffer2[j]);
    }
#endif

  printf("\n");
  himem_data_check(work_buffer1, work_buffer2, size, offset);

  /* read dev2 */

  memset(work_buffer2, 0, size);
  if (lseek(fd2, offset, SEEK_SET) < 0)
    {
      printf("HiMem Seek failed. \n");
      goto common_exit;
    }

  if (read(fd2, work_buffer2, size) != size)
    {
      printf("HiMem Read failed. \n");
      goto common_exit;
    }

  printf("read HIMEM_DEV2 data:");
#if DATA_OUTPUT
  for (int j = 0; j < 128; j++)
    {
      printf("%d ", work_buffer2[j]);
    }
#endif

  printf("\n");
  himem_data_check(work_buffer3, work_buffer2, size, offset);

common_exit:
  if (work_buffer1 == NULL)
    {
      free(work_buffer1);
    }

  if (work_buffer2 == NULL)
    {
      free(work_buffer2);
    }

  if (work_buffer3 == NULL)
    {
      free(work_buffer3);
    }

common_exit1:
  close(fd2);
common_exit2:
  close(fd1);
common_exit3:

  himem_chardev_unregister(HIMEM_DEV1);
  himem_chardev_unregister(HIMEM_DEV2);

  free(work_buffer1);
  free(work_buffer2);
  free(work_buffer3);
}

/****************************************************************************
 * Himem access function verification from 1MB to 4 MB
 ****************************************************************************/

static void himem_check(uint32_t size, uint32_t offset)
{
  int fd;
  uint8_t *work_buffer1 = NULL;
  uint8_t *work_buffer2 = NULL;

  if (himem_chardev_register(HIMEM_DEV1, size + offset + 1024) != 0)
    {
      printf("HIMEM_DEV Create failed. \n");
      goto common_exit3;
    }

  if ((fd = open(HIMEM_DEV1, O_RDWR)) < 0)
    {
      printf("HiMemo Open failed. \n");
      goto common_exit2;
    }

  if ((work_buffer1 = malloc(size)) == NULL)
    {
      printf("Allocate failed1");
      goto common_exit;
    }

  if ((work_buffer2 = malloc(size)) == NULL)
    {
      printf("Allocate failed2");
      goto common_exit;
    }

  memset(work_buffer1, 0, size);
  memset(work_buffer2, 0, size);

  printf("Memory Size: %d KB\nHimemSizeTest=%d\n",
               size / 1024, size + offset);

  for (int i = 0; i < size; i++)
    {
      work_buffer1[i] = i;
    }

  if (lseek(fd, offset, SEEK_SET) < 0)
    {
      printf("HiMem Seek failed. \n");
      goto common_exit;
    }

  if (write(fd, work_buffer1, size) != size)
    {
      printf("HiMem Write failed. \n");
      goto common_exit;
    }

  if (lseek(fd, offset, SEEK_SET) < 0)
    {
      printf("HiMem Seek failed. \n");
      goto common_exit;
    }

  if (read(fd, work_buffer2, size) != size)
    {
      printf("HiMem Read failed. \n");
      goto common_exit;
    }

  himem_data_check(work_buffer1, work_buffer2, size, offset);

common_exit:
  if (work_buffer1 == NULL)
    {
      free(work_buffer1);
    }

  if (work_buffer2 == NULL)
    {
      free(work_buffer2);
    }

common_exit2:
  close(fd);
common_exit3:
  himem_chardev_unregister(HIMEM_DEV1);

  free(work_buffer1);
  free(work_buffer2);
}

/****************************************************************************
 * Himem Initialization processing section
 ****************************************************************************/

static int himem_chardev_test(void)
{
  uint32_t result;

  /* himem Cdev initialization */

  result = himem_chardev_init();
  if (result != 0)
    {
      printf("himem_chardev_init() err:%d", result);
    }

  /* 1M--->4M */

  for (int i = 0; i < 4; i++)
    {
      printf("\nHimem Cdev test: %d MB\n", (i + 1));
      for (int j = 0; j < 11; j++)
        {
          /* 1K--->1M */

          himem_check(1024 * pow(2, j), 1048576 * i);
        }
    }

  printf("\n\nRead and write multiple device files!\n");

  /* 16K  and 1024K */

  himem_multi_dev_check(16384, 0);
  himem_multi_dev_check(1048576, 0);

  result = himem_chardev_exit();
  if (result != 0)
    {
      printf("himem_chardev_exit() err:%d", result);
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * himem_cdev_main
 ****************************************************************************/

int main(int argc, char*argv[])
{
  if (!himem_chardev_test())
    {
      printf("Himemchardev test success!\n");
    }
  else
    {
      printf("Himemchardev test failed!\n");
    }

  return 0;
}
