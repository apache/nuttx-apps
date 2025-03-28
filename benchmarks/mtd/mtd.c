/****************************************************************************
 * apps/benchmarks/mtd/mtd.c
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

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <nuttx/mtd/mtd.h>
#include <nuttx/fs/smart.h>
#include <nuttx/fs/ioctl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct inode            *inode;
  struct timespec         start;
  struct timespec         end;
  struct mtd_geometry_s   geo;
  struct partition_info_s info;
  int                     ret;
  int                     x;
  double                  elapsed_time;
  double                  transfer_rate;
  size_t                  total_bytes_written = 0;
  size_t                  total_bytes_read = 0;
  char                    *buffer;

  /* Argument given? */

  if (argc < 2)
    {
      fprintf(stderr, "usage: mtd flash_block_device\n");
      return -1;
    }

  /* Find the inode of the block driver identified by 'source' */

  ret = open_blockdriver(argv[1], 0, &inode);
  if (ret < 0)
    {
      fprintf(stderr, "Failed to open %s\n", argv[1]);
      return ret;
    }

  /* Get the low-level format from the device. */

  ret = inode->u.i_bops->ioctl(inode, BIOC_PARTINFO, (unsigned long) &info);
  if (ret != OK)
    {
      fprintf(stderr, "Device is not a block device\n");
      goto errout_with_driver;
    }

  /* Get the MTD geometry */

  ret = inode->u.i_bops->ioctl(inode, MTDIOC_GEOMETRY, (unsigned long) &geo);
  if (ret != OK)
    {
      fprintf(stderr, "Device is not a MTD device");
      goto errout_with_driver;
    }

  /* Report the device structure */

  printf("FLASH device parameters:\n");
  printf("   Sector size:  %10d\n", info.sectorsize);
  printf("   Sector count: %10d\n", info.numsectors);
  printf("   Erase block:  %10" PRIx32 "\n", geo.erasesize);
  printf("   Total size:   %10d\n", info.sectorsize * info.numsectors);

  if (info.sectorsize != geo.erasesize)
    {
      fprintf(stderr, "Sector size does not match the erase block size.\n"
             "Please adjust the sector size to enable erasing and writing "
             "without using an intermediary read buffer.\n");
      goto errout_with_driver;
    }

  /* Allocate buffers to use */

  buffer = (char *)malloc(info.sectorsize);
  if (buffer == NULL)
    {
      fprintf(stderr, "Error allocating buffer\n");
      goto errout_with_driver;
    }

  /* Fill the buffer with known data and print it in hex format */

  for (int i = 0; i < info.sectorsize; i++)
    {
      buffer[i] = (char)(i & 0xff);
    }

  /* Now write some data to the sector */

  printf("\nStarting write operation...\n");

  clock_gettime(CLOCK_MONOTONIC, &start);
  for (x = 0; x < info.numsectors; x++)
    {
      inode->u.i_bops->write(inode, (const unsigned char *)buffer, x, 1);

      total_bytes_written += info.sectorsize;
    }

  clock_gettime(CLOCK_MONOTONIC, &end);

  elapsed_time = (end.tv_sec - start.tv_sec) + \
                 (end.tv_nsec - start.tv_nsec) / 1e9;
  transfer_rate = (total_bytes_written / elapsed_time) / 1024;

  printf("\nWrite operation completed in %.2f seconds\n", elapsed_time);
  printf("Total bytes written: %zu\n", total_bytes_written);
  printf("Transfer rate [write]: %.2f KiB/s\n", transfer_rate);

  /* Now read the data back to validate everything was written and can
   * be read.
   */

  printf("\nStarting read operation...\n");

  clock_gettime(CLOCK_MONOTONIC, &start);

  for (x = 0; x < info.numsectors; x++)
    {
      inode->u.i_bops->read(inode, (unsigned char *)buffer, x, 1);

      total_bytes_read += info.sectorsize;
    }

  clock_gettime(CLOCK_MONOTONIC, &end);

  elapsed_time = (end.tv_sec - start.tv_sec) + \
                 (end.tv_nsec - start.tv_nsec) / 1e9;
  transfer_rate = (total_bytes_written / elapsed_time) / 1024;

  printf("\nRead operation completed in %.2f seconds\n", elapsed_time);
  printf("Total bytes read: %zu\n", total_bytes_read);
  printf("Transfer rate [read]: %.2f KiB/s\n", transfer_rate);

  /* Compare the read data with the written data */

  for (int i = 0; i < info.sectorsize; i++)
    {
      if (buffer[i] != (char)(i & 0xff))
        {
          printf("\nData mismatch at byte %d: expected %02X, got %02X\n",
                 i, (unsigned char)(i & 0xff), (unsigned char)buffer[i]);
          goto errout_with_buffers;
        }
    }

  printf("\nData verification successful: read data matches written data\n");

errout_with_buffers:

  /* Free the allocated buffers */

  free(buffer);

errout_with_driver:

  /* Now close the block device and exit */

  close_blockdriver(inode);
  return 0;
}
