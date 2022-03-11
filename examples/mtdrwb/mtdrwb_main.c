/****************************************************************************
 * apps/examples/mtdrwb/mtdrwb_main.c
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

#ifdef CONFIG_EXAMPLES_MTDRWB

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#if !defined(CONFIG_MTD_WRBUFFER) && !defined(CONFIG_MTD_READAHEAD)
#  error CONFIG_MTD_WRBUFFER or CONFIG_MTD_READAHEAD must be selected
#endif

/* The default is to use the RAM MTD device at drivers/mtd/rammtd.c.  But
 * an architecture-specific MTD driver can be used instead by defining
 * CONFIG_EXAMPLES_MTDRWB_ARCHINIT.  In this case, the initialization logic
 * will call mtdrwb_archinitialize() to obtain the MTD driver instance.
 */

#ifndef CONFIG_EXAMPLES_MTDRWB_ARCHINIT

/* Make sure that the RAM MTD driver is enabled */

#  ifndef CONFIG_RAMMTD
#    error "CONFIG_RAMMTD is required without CONFIG_EXAMPLES_MTDRWB_ARCHINIT"
#  endif

/* This must exactly match the default configuration in
 * drivers/mtd/rammtd.c
 */

#  ifndef CONFIG_RAMMTD_ERASESIZE
#    define CONFIG_RAMMTD_ERASESIZE 4096
#  endif

/* Given the ERASESIZE, CONFIG_EXAMPLES_MTDRWB_NEBLOCKS will determine the
 * size of the RAM allocation needed.
 */

#  ifndef CONFIG_EXAMPLES_MTDRWB_NEBLOCKS
#    define CONFIG_EXAMPLES_MTDRWB_NEBLOCKS (32)
#  endif

#  undef MTDRWB_BUFSIZE
#  define MTDRWB_BUFSIZE \
    (CONFIG_RAMMTD_ERASESIZE * CONFIG_EXAMPLES_MTDRWB_NEBLOCKS)

#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct mtdweb_filedesc_s
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

#ifndef CONFIG_EXAMPLES_MTDRWB_ARCHINIT
static uint8_t g_simflash[MTDRWB_BUFSIZE];
#endif

/****************************************************************************
 * External Functions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_MTDRWB_ARCHINIT
extern FAR struct mtd_dev_s *mtdrwb_archinitialize(void);
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mtdrwb_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct mtd_dev_s *mtdraw;
  FAR struct mtd_dev_s *mtdrwb;
  FAR struct mtd_geometry_s geo;
  FAR uint32_t *buffer;
  ssize_t nbytes;
  off_t nblocks;
  off_t offset;
  off_t seekpos;
  unsigned int blkpererase;
  int fd;
  int i;
  int j;
  int k;
  int ret;

  /* Create and initialize a RAM MTD FLASH driver instance */

#ifdef CONFIG_EXAMPLES_MTDRWB_ARCHINIT
  mtdraw = mtdrwb_archinitialize();
#else
  mtdraw = rammtd_initialize(g_simflash, MTDRWB_BUFSIZE);
#endif
  if (!mtdraw)
    {
      printf("ERROR: Failed to create RAM MTD instance\n");
      fflush(stdout);
      exit(1);
    }

  /* Perform the IOCTL to erase the entire FLASH part */

  ret = mtdraw->ioctl(mtdraw, MTDIOC_BULKERASE, 0);
  if (ret < 0)
    {
      printf("ERROR: MTDIOC_BULKERASE ioctl failed: %d\n", ret);
    }

  /* Initialize to support buffering on the MTD device */

  mtdrwb = mtd_rwb_initialize(mtdraw);
  if (!mtdrwb)
    {
      printf("ERROR: Failed to create RAM MTD R/W buffering\n");
      fflush(stdout);
      exit(2);
    }

  /* Initialize to provide an FTL block driver on the MTD FLASH interface.
   *
   * NOTE:  We could just skip all of this FTL and BCH stuff.  We could
   * instead just use the MTD drivers bwrite and bread to perform this
   * test.  Creating the character drivers, however, makes this test more
   * interesting.
   */

  ret = ftl_initialize(0, mtdrwb);
  if (ret < 0)
    {
      printf("ERROR: ftl_initialize /dev/mtdblock0 failed: %d\n", ret);
      fflush(stdout);
      exit(3);
    }

  /* Now create a character device on the block device */

  ret = bchdev_register("/dev/mtdblock0", "/dev/mtd0", false);
  if (ret < 0)
    {
      printf("ERROR: bchdev_register /dev/mtd0 failed: %d\n", ret);
      fflush(stdout);
      exit(4);
    }

  /* Get the geometry of the FLASH device */

  ret = mtdrwb->ioctl(mtdrwb, MTDIOC_GEOMETRY,
                      (unsigned long)((uintptr_t)&geo));
  if (ret < 0)
    {
      ferr("ERROR: mtdrwb->ioctl failed: %d\n", ret);
      exit(5);
    }

  printf("Flash Geometry:\n");
  printf("  blocksize:      %lu\n", (unsigned long)geo.blocksize);
  printf("  erasesize:      %lu\n", (unsigned long)geo.erasesize);
  printf("  neraseblocks:   %lu\n", (unsigned long)geo.neraseblocks);

  blkpererase = geo.erasesize / geo.blocksize;
  printf("  blkpererase:    %u\n", blkpererase);

  nblocks = geo.neraseblocks * blkpererase;
  printf("  nblocks:        %lu\n", (unsigned long)nblocks);

  /* Allocate a buffer */

  buffer = (FAR uint32_t *)malloc(geo.blocksize);
  if (!buffer)
    {
      printf("ERROR: failed to allocate a sector buffer\n");
      fflush(stdout);
      exit(6);
    }

  /* Open the MTD FLASH character driver for writing */

  fd = open("/dev/mtd0", O_WRONLY);
  if (fd < 0)
    {
      printf("ERROR: open /dev/mtd0 failed: %d\n", errno);
      fflush(stdout);
      exit(7);
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
              offset += sizeof(uint32_t);
            }

          /* And write it using the character driver */

          nbytes = write(fd, buffer, geo.blocksize);
          if (nbytes < 0)
            {
              printf("ERROR: write to /dev/mtd0 failed: %d\n", errno);
              fflush(stdout);
              exit(8);
            }
        }
    }

  close(fd);

  /* Open the MTD character driver for reading */

  fd = open("/dev/mtd0", O_RDONLY);
  if (fd < 0)
    {
      printf("ERROR: open /dev/mtd0 failed: %d\n", errno);
      fflush(stdout);
      exit(9);
    }

  /* Now verify the offset in every block */

  offset  = 0;
  for (j = 0; j < nblocks; j++)
    {
      /* Seek to the next read position */

      seekpos = lseek(fd, offset, SEEK_SET);
      if (seekpos != offset)
        {
          printf("ERROR: lseek to offset %ld failed: %d\n",
                 (unsigned long)offset, errno);
          fflush(stdout);
          exit(10);
        }

      /* Read the next block into memory */

      nbytes = read(fd, buffer, geo.blocksize);
      if (nbytes < 0)
        {
          printf("ERROR: read from /dev/mtd0 failed: %d\n", errno);
          fflush(stdout);
          exit(11);
        }
      else if (nbytes == 0)
        {
          printf("ERROR: Unexpected end-of file in /dev/mtd0\n");
          fflush(stdout);
          exit(12);
        }
      else if (nbytes != geo.blocksize)
        {
          printf("ERROR: Unexpected read size from /dev/mtd0 : %ld\n",
                 (unsigned long)nbytes);
          fflush(stdout);
          exit(13);
        }

      /* Since we forced the size of the partition to be an even number
       * of erase blocks, we do not expect to encounter the end of file
       * indication.
       */

      else if (nbytes == 0)
        {
          printf("ERROR: Unexpected end of file on /dev/mtd0\n");
          fflush(stdout);
         exit(14);
        }

      /* This is not expected at all */

      else if (nbytes != geo.blocksize)
        {
          printf("ERROR: Short read from /dev/mtd0 failed: %lu\n",
                 (unsigned long)nbytes);
          fflush(stdout);
          exit(15);
        }

      /* Verify the offsets in the block */

      for (k = 0; k < geo.blocksize / sizeof(uint32_t); k++)
        {
          if (buffer[k] != offset)
            {
              printf("ERROR: Bad offset %lu, expected %lu\n",
                     (long)buffer[k], (long)offset);
              fflush(stdout);
              exit(16);
            }

          offset += sizeof(uint32_t);
        }
    }

  /* Try reading one more time.  We should get the end of file */

  nbytes = read(fd, buffer, geo.blocksize);
  if (nbytes != 0)
    {
      printf("ERROR: Expected end-of-file from /dev/mtd0 failed: %zd %d\n",
             nbytes, errno);
      fflush(stdout);
      exit(20);
    }

  close(fd);

  /* And exit without bothering to clean up */

  printf("PASS: Everything looks good\n");
  fflush(stdout);
  return 0;
}

#endif /* CONFIG_EXAMPLES_MTDRWB */
