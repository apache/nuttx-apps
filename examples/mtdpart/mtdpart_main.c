/****************************************************************************
 * apps/examples/mtdpart/mtdpart_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/mtd/mtd.h>
#include <nuttx/drivers/drivers.h>
#include <nuttx/fs/ioctl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* Make sure that support for MTD partitions is enabled */

#ifndef CONFIG_MTD_PARTITION
#  error "CONFIG_MTD_PARTITION is required"
#endif

/* The default is to use the RAM MTD device at drivers/mtd/rammtd.c.  But
 * an architecture-specific MTD driver can be used instead by defining
 * CONFIG_EXAMPLES_MTDPART_ARCHINIT.  In this case, the initialization logic
 * will call mtdpart_archinitialize() to obtain the MTD driver instance.
 */

#ifndef CONFIG_EXAMPLES_MTDPART_ARCHINIT

/* Make sure that the RAM MTD driver is enabled */

#  ifndef CONFIG_RAMMTD
#    error "CONFIG_RAMMTD is required without CONFIG_EXAMPLES_MTDPART_ARCHINIT"
#  endif

/* This must exactly match the default configuration in
 * drivers/mtd/rammtd.c
 */

#  ifndef CONFIG_RAMMTD_ERASESIZE
#    define CONFIG_RAMMTD_ERASESIZE 4096
#  endif

/* Given the ERASESIZE, CONFIG_EXAMPLES_MTDPART_NEBLOCKS will determine the
 * size of the RAM allocation needed.
 */

#  ifndef CONFIG_EXAMPLES_MTDPART_NEBLOCKS
#    define CONFIG_EXAMPLES_MTDPART_NEBLOCKS (32)
#  endif

#  undef MTDPART_BUFSIZE
#  define MTDPART_BUFSIZE \
    (CONFIG_RAMMTD_ERASESIZE * CONFIG_EXAMPLES_MTDPART_NEBLOCKS)

#endif

#ifndef CONFIG_EXAMPLES_MTDPART_NPARTITIONS
#  define CONFIG_EXAMPLES_MTDPART_NPARTITIONS 3
#endif

#if CONFIG_EXAMPLES_MTDPART_NPARTITIONS <= 0
#  error "CONFIG_EXAMPLES_MTDPART_NPARTITIONS must be greater than 0"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Pre-allocated simulated flash */

#ifndef CONFIG_EXAMPLES_MTDPART_ARCHINIT
static uint8_t g_simflash[MTDPART_BUFSIZE];
#endif

/****************************************************************************
 * External Functions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_MTDPART_ARCHINIT
extern FAR struct mtd_dev_s *mtdpart_archinitialize(void);
extern void mtdpart_archuninitialize(FAR struct mtd_dev_s *dev);
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mtdpart_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct mtd_dev_s *master;
  FAR struct mtd_dev_s *part[CONFIG_EXAMPLES_MTDPART_NPARTITIONS + 1];
  FAR struct mtd_geometry_s geo;
  FAR uint32_t *buffer;
  char mtdname[32];
  size_t partsize;
  ssize_t nbytes;
  off_t nblocks;
  off_t offset;
  off_t check;
  off_t sectoff;
  off_t seekpos;
  unsigned int blkpererase;
  int fd;
  int i;
  int j;
  int k;
  int ret;
  int status;

  /* Create and initialize a RAM MTD FLASH driver instance */

#ifdef CONFIG_EXAMPLES_MTDPART_ARCHINIT
  master = mtdpart_archinitialize();
#else
  master = rammtd_initialize(g_simflash, MTDPART_BUFSIZE);
#endif
  if (!master)
    {
      printf("ERROR: Failed to create RAM MTD instance\n");
      status = 1;
      exit(status);
    }

  /* Perform the IOCTL to erase the entire FLASH part */

  ret = master->ioctl(master, MTDIOC_BULKERASE, 0);
  if (ret < 0)
    {
      printf("ERROR: MTDIOC_BULKERASE ioctl failed: %d\n", ret);
    }

  /* Initialize to provide an FTL block driver on the MTD FLASH interface.
   *
   * NOTE:  We could just skip all of this FTL and BCH stuff.  We could
   * instead just use the MTD drivers bwrite and bread to perform this
   * test.  Creating the character drivers, however, makes this test more
   * interesting.
   */

  ret = register_mtddriver("/dev/mtd0", master, 0775, NULL);
  if (ret < 0)
    {
      printf("ERROR: register_mtddriver /dev/mtd0 failed: %d\n", ret);
      status = 2;
      goto err_master_mtd;
    }

  /* Get the geometry of the FLASH device */

  ret = master->ioctl(master, MTDIOC_GEOMETRY,
                      (unsigned long)((uintptr_t)&geo));
  if (ret < 0)
    {
      printf("ERROR: mtd->ioctl failed: %d\n", ret);
      status = 3;
      goto err_master_mtd;
    }

  printf("Flash Geometry:\n");
  printf("  blocksize:      %lu\n", (unsigned long)geo.blocksize);
  printf("  erasesize:      %lu\n", (unsigned long)geo.erasesize);
  printf("  neraseblocks:   %lu\n", (unsigned long)geo.neraseblocks);

  /* Determine the size of each partition.  Make each partition an even
   * multiple of the erase block size (perhaps not using some space at the
   * end of the FLASH).
   */

  blkpererase = geo.erasesize / geo.blocksize;
  nblocks     = (geo.neraseblocks / CONFIG_EXAMPLES_MTDPART_NPARTITIONS) *
                blkpererase;
  partsize    = nblocks * geo.blocksize;

  printf("  No. partitions: %u\n", CONFIG_EXAMPLES_MTDPART_NPARTITIONS);
  printf("  Partition size: %ju Blocks (%zu bytes)\n", (uintmax_t)nblocks,
         partsize);

  /* Now create MTD FLASH partitions */

  printf("Creating partitions\n");

  for (offset = 0, i = 1;
       i <= CONFIG_EXAMPLES_MTDPART_NPARTITIONS;
       offset += nblocks, i++)
    {
      printf("  Partition %d. Block offset=%lu, size=%lu\n",
             i, (unsigned long)offset, (unsigned long)nblocks);

      /* Create the partition */

      part[i] = mtd_partition(master, offset, nblocks);
      if (!part[i])
        {
          printf("ERROR: mtd_partition failed. offset=%lu nblocks=%lu\n",
                (unsigned long)offset, (unsigned long)nblocks);
          status = 4;
          goto err_part_mtd;
        }

      snprintf(mtdname, sizeof(mtdname), "/dev/mtd%d", i);

      ret = register_mtddriver(mtdname, part[i], 0775, NULL);
      if (ret < 0)
        {
          printf("ERROR: register_mtddriver %s failed: %d\n", mtdname, ret);
          status = 5;
          goto err_part_mtd;
        }
    }

  /* Allocate a buffer */

  buffer = (FAR uint32_t *)malloc(geo.blocksize);
  if (!buffer)
    {
      printf("ERROR: failed to allocate a sector buffer\n");
      status = 6;
      goto err_part_mtd;
    }

  /* Open the master MTD FLASH character driver for writing */

  fd = open("/dev/mtd0", O_WRONLY);
  if (fd < 0)
    {
      printf("ERROR: open /dev/mtd0 failed: %d\n", errno);
      status = 7;
      goto err_buf;
    }

  /* Now write the offset into every block */

  printf("Initializing media:\n");

  offset = 0;
  for (i = 0; i < geo.neraseblocks; i++)
    {
      for (j = 0; j < blkpererase; j++)
        {
          /* Fill the block with the offset */

          for (k = 0; k < geo.blocksize / sizeof(uint32_t); k++)
            {
              buffer[k] = offset;
              offset += 4;
            }

          /* And write it using the character driver */

          nbytes = write(fd, buffer, geo.blocksize);
          if (nbytes < 0)
            {
              printf("ERROR: write to /dev/mtd0 failed: %d\n", errno);
              status = 8;
              goto err_fd;
            }
        }
    }

  close(fd);

  /* Now read each partition */

  printf("Checking partitions:\n");

  for (offset = 0, i = 1;
       i <= CONFIG_EXAMPLES_MTDPART_NPARTITIONS;
       offset += partsize, i++)
    {
      printf("  Partition %d. Byte offset=%lu, size=%lu\n",
             i, (unsigned long)offset, (unsigned long)partsize);

      /* Open the master MTD partition character driver for writing */

      snprintf(mtdname, sizeof(mtdname), "/dev/mtd%d", i);
      fd = open(mtdname, O_RDWR);
      if (fd < 0)
        {
          printf("ERROR: open %s failed: %d\n", mtdname, errno);
          status = 9;
          goto err_buf;
        }

      /* Now verify the offset in every block */

      check = offset;
      sectoff = 0;

      for (j = 0; j < nblocks; j++)
        {
#if 0 /* Too much */
          printf("  block=%u offset=%lu\n", j, (unsigned long) check);
#endif
          /* Seek to the next read position */

          seekpos = lseek(fd, sectoff, SEEK_SET);
          if (seekpos != sectoff)
            {
              printf("ERROR: lseek to offset %ld failed: %d\n",
                     (unsigned long)sectoff, errno);
              status = 10;
              goto err_fd;
            }

          /* Read the next block into memory */

          nbytes = read(fd, buffer, geo.blocksize);
          if (nbytes < 0)
            {
              printf("ERROR: read from %s failed: %d\n", mtdname, errno);
              status = 11;
              goto err_fd;
            }

          /* Since we forced the size of the partition to be an even number
           * of erase blocks, we do not expect to encounter the end of file
           * indication.
           */

          else if (nbytes == 0)
            {
              printf("ERROR: Unexpected end of file on %s\n", mtdname);
              status = 12;
              goto err_fd;
            }

          /* This is not expected at all */

          else if (nbytes != geo.blocksize)
            {
              printf("ERROR: Short read from %s failed: %lu\n",
                      mtdname, (unsigned long)nbytes);
              status = 13;
              goto err_fd;
            }

          /* Verify the offsets in the block */

          for (k = 0; k < geo.blocksize / sizeof(uint32_t); k++)
            {
              if (buffer[k] != check)
                {
                  printf("ERROR: Bad offset %lu, expected %lu\n",
                         (long)buffer[k], (long)check);
                  status = 14;
                  goto err_fd;
                }

              /* Invert the value to indicate that we have verified
               * this value.
               */

              buffer[k] = ~check;
              check += sizeof(uint32_t);
            }

          /* Seek to the next write position */

          seekpos = lseek(fd, sectoff, SEEK_SET);
          if (seekpos != sectoff)
            {
              printf("ERROR: lseek to offset %ld failed: %d\n",
                     (unsigned long)sectoff, errno);
              status = 15;
              goto err_fd;
            }

          /* Now write the block back to FLASH with the modified value */

          nbytes = write(fd, buffer, geo.blocksize);
          if (nbytes < 0)
            {
              printf("ERROR: write to %s failed: %d\n", mtdname, errno);
              status = 16;
              goto err_fd;
            }
          else if (nbytes != geo.blocksize)
            {
              printf("ERROR: Unexpected write size to %s: %ld\n",
                     mtdname, (unsigned long)nbytes);
              status = 17;
              goto err_fd;
            }

          /* Get the offset to the next block */

          sectoff += geo.blocksize;
        }

      /* Try reading one more time.  We should get the end of file */

      nbytes = read(fd, buffer, geo.blocksize);
      if (nbytes != 0)
        {
          printf("ERROR: Expected end-of-file from %s failed: %zd %d\n",
                 mtdname, nbytes, errno);
          status = 18;
          goto err_fd;
        }

      close(fd);
    }

  /* Now verify that all of the verified blocks appear where we thing they
   * should on the device.
   */

  printf("Verifying media:\n");

  fd = open("/dev/mtd0", O_RDONLY);
  if (fd < 0)
    {
      printf("ERROR: open /dev/mtd0 failed: %d\n", errno);
      status = 19;
      goto err_buf;
    }

  offset = 0;
  check  = 0;

  for (i = 0; i < nblocks * CONFIG_EXAMPLES_MTDPART_NPARTITIONS; i++)
    {
      /* Read the next block into memory */

      nbytes = read(fd, buffer, geo.blocksize);
      if (nbytes < 0)
        {
          printf("ERROR: read from /dev/mtd0 failed: %d\n", errno);
          status = 20;
          goto err_fd;
        }
      else if (nbytes == 0)
        {
          printf("ERROR: Unexpected end-of file in /dev/mtd0\n");
          status = 21;
          goto err_fd;
        }
      else if (nbytes != geo.blocksize)
        {
          printf("ERROR: Unexpected read size from /dev/mtd0: %ld\n",
                 (unsigned long)nbytes);
          status = 22;
          goto err_fd;
        }

      /* Verify the values in the block */

      for (k = 0; k < geo.blocksize / sizeof(uint32_t); k++)
        {
          if (buffer[k] != ~check)
            {
              printf("ERROR: Bad value %lu, expected %lu\n",
                     (long)buffer[k], (long)(~check));
              status = 23;
              goto err_fd;
            }

          check += sizeof(uint32_t);
        }
    }

  status = 0;

  /* And exit without bothering to clean up */

  printf("PASS: Everything looks good\n");

err_fd:
  close(fd);

err_buf:
  free(buffer);

err_part_mtd:
  for (i = 1; i <= CONFIG_EXAMPLES_MTDPART_NPARTITIONS; i++)
    {
      snprintf(mtdname, sizeof(mtdname), "/dev/mtd%d", i);
      unregister_mtddriver(mtdname);
    }

err_master_mtd:
  unregister_mtddriver("/dev/mtd0");
#ifdef CONFIG_EXAMPLES_MTDPART_ARCHINIT
  mtdpart_archuninitialize(master);
#else
  rammtd_uninitialize(master);
#endif
  fflush(stdout);
  exit(status);
}
