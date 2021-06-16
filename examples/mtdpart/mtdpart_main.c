/****************************************************************************
 * apps/examples/mtdpart/mtdpart_main.c
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

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct mtdpart_filedesc_s
{
  FAR char *name;
  bool deleted;
  size_t len;
  uint32_t crc;
};

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
  char blockname[32];
  char charname[32];
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

  /* Create and initialize a RAM MTD FLASH driver instance */

#ifdef CONFIG_EXAMPLES_MTDPART_ARCHINIT
  master = mtdpart_archinitialize();
#else
  master = rammtd_initialize(g_simflash, MTDPART_BUFSIZE);
#endif
  if (!master)
    {
      printf("ERROR: Failed to create RAM MTD instance\n");
      fflush(stdout);
      exit(1);
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

  ret = ftl_initialize(0, master);
  if (ret < 0)
    {
      printf("ERROR: ftl_initialize /dev/mtdblock0 failed: %d\n", ret);
      fflush(stdout);
      exit(2);
    }

  /* Now create a character device on the block device */

  ret = bchdev_register("/dev/mtdblock0", "/dev/mtd0", false);
  if (ret < 0)
    {
      printf("ERROR: bchdev_register /dev/mtd0 failed: %d\n", ret);
      fflush(stdout);
      exit(3);
    }

  /* Get the geometry of the FLASH device */

  ret = master->ioctl(master, MTDIOC_GEOMETRY,
                      (unsigned long)((uintptr_t)&geo));
  if (ret < 0)
    {
      ferr("ERROR: mtd->ioctl failed: %d\n", ret);
      exit(3);
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
  printf("  Partition size: %ju Blocks (%lu bytes)\n", (uintmax_t)nblocks,
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
          fflush(stdout);
          exit(4);
        }

      /* Initialize to provide an FTL block driver on the MTD FLASH
       * interface
       */

      snprintf(blockname, 32, "/dev/mtdblock%d", i);
      snprintf(charname, 32, "/dev/mtd%d", i);

      ret = ftl_initialize(i, part[i]);
      if (ret < 0)
        {
          printf("ERROR: ftl_initialize %s failed: %d\n", blockname, ret);
          fflush(stdout);
          exit(5);
        }

      /* Now create a character device on the block device */

      ret = bchdev_register(blockname, charname, false);
      if (ret < 0)
        {
          printf("ERROR: bchdev_register %s failed: %d\n", charname, ret);
          fflush(stdout);
          exit(6);
        }
    }

  /* Allocate a buffer */

  buffer = (FAR uint32_t *)malloc(geo.blocksize);
  if (!buffer)
    {
      printf("ERROR: failed to allocate a sector buffer\n");
      fflush(stdout);
      exit(7);
    }

  /* Open the master MTD FLASH character driver for writing */

  fd = open("/dev/mtd0", O_WRONLY);
  if (fd < 0)
    {
      printf("ERROR: open /dev/mtd0 failed: %d\n", errno);
      fflush(stdout);
      exit(8);
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
              fflush(stdout);
              exit(9);
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

      snprintf(charname, 32, "/dev/mtd%d", i);
      fd = open(charname, O_RDWR);
      if (fd < 0)
        {
          printf("ERROR: open %s failed: %d\n", charname, errno);
          fflush(stdout);
          exit(10);
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
              fflush(stdout);
              exit(11);
            }

          /* Read the next block into memory */

          nbytes = read(fd, buffer, geo.blocksize);
          if (nbytes < 0)
            {
              printf("ERROR: read from %s failed: %d\n", charname, errno);
              fflush(stdout);
              exit(12);
            }
          else if (nbytes == 0)
            {
              printf("ERROR: Unexpected end-of file in %s\n", charname);
              fflush(stdout);
              exit(13);
            }
          else if (nbytes != geo.blocksize)
            {
              printf("ERROR: Unexpected read size from %s: %ld\n",
                     charname, (unsigned long)nbytes);
              fflush(stdout);
              exit(14);
            }

          /* Since we forced the size of the partition to be an even number
           * of erase blocks, we do not expect to encounter the end of file
           * indication.
           */

          else if (nbytes == 0)
            {
               printf("ERROR: Unexpected end of file on %s\n", charname);
               fflush(stdout);
               exit(15);
            }

          /* This is not expected at all */

          else if (nbytes != geo.blocksize)
            {
               printf("ERROR: Short read from %s failed: %lu\n",
                      charname, (unsigned long)nbytes);
               fflush(stdout);
               exit(16);
            }

          /* Verify the offsets in the block */

          for (k = 0; k < geo.blocksize / sizeof(uint32_t); k++)
            {
              if (buffer[k] != check)
                {
                  printf("ERROR: Bad offset %lu, expected %lu\n",
                         (long)buffer[k], (long)check);
                  fflush(stdout);
                  exit(17);
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
              fflush(stdout);
              exit(18);
            }

          /* Now write the block back to FLASH with the modified value */

          nbytes = write(fd, buffer, geo.blocksize);
          if (nbytes < 0)
            {
              printf("ERROR: write to %s failed: %d\n", charname, errno);
              fflush(stdout);
              exit(19);
            }
          else if (nbytes != geo.blocksize)
            {
              printf("ERROR: Unexpected write size to %s: %ld\n",
                     charname, (unsigned long)nbytes);
              fflush(stdout);
              exit(20);
            }

          /* Get the offset to the next block */

          sectoff += geo.blocksize;
        }

      /* Try reading one more time.  We should get the end of file */

      nbytes = read(fd, buffer, geo.blocksize);
      if (nbytes != 0)
        {
          printf("ERROR: Expected end-of-file from %s failed: %zd %d\n",
                 charname, nbytes, errno);
          fflush(stdout);
          exit(22);
        }

      close(fd);
    }

  /* Now verify that all of the verifed blocks appear where we thing they
   * should on the device.
   */

  printf("Verifying media:\n");

  fd = open("/dev/mtd0", O_RDONLY);
  if (fd < 0)
    {
      printf("ERROR: open /dev/mtd0 failed: %d\n", errno);
      fflush(stdout);
      exit(23);
    }

  offset = 0;
  check  = 0;

  for (i = 0; i < nblocks * CONFIG_EXAMPLES_MTDPART_NPARTITIONS; i++)
    {
      /* Read the next block into memory */

      nbytes = read(fd, buffer, geo.blocksize);
      if (nbytes < 0)
        {
          printf("ERROR: read from %s failed: %d\n", charname, errno);
          fflush(stdout);
          exit(24);
        }
      else if (nbytes == 0)
        {
          printf("ERROR: Unexpected end-of file in %s\n", charname);
          fflush(stdout);
          exit(25);
        }
      else if (nbytes != geo.blocksize)
        {
          printf("ERROR: Unexpected read size from %s: %ld\n",
                 charname, (unsigned long)nbytes);
          fflush(stdout);
          exit(26);
        }

      /* Verify the values in the block */

      for (k = 0; k < geo.blocksize / sizeof(uint32_t); k++)
        {
          if (buffer[k] != ~check)
            {
              printf("ERROR: Bad value %lu, expected %lu\n",
                     (long)buffer[k], (long)(~check));
              fflush(stdout);
              exit(27);
            }

          check += sizeof(uint32_t);
        }
    }

  close(fd);

  /* And exit without bothering to clean up */

  printf("PASS: Everything looks good\n");
  fflush(stdout);
  return 0;
}
