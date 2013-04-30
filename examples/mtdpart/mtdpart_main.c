/****************************************************************************
 * examples/mtdpart/mtdpart_main.c
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

#include <nuttx/mtd.h>
#include <nuttx/fs/fs.h>
#include <nuttx/fs/ioctl.h>

/****************************************************************************
 * Definitions
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

/* This must exactly match the default configuration in drivers/mtd/rammtd.c */

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

#ifdef CONFIG_EXAMPLES_MTDPART_NPARTITIONS
#  define CONFIG_EXAMPLES_MTDPART_NPARTITIONS 3
#endif

/* Debug ********************************************************************/
#if defined(CONFIG_DEBUG) && defined(CONFIG_DEBUG_FS)
#  define message    syslog
#  define msgflush()
#else
#  define message    printf
#  define msgflush() fflush(stdout);
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
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mtdpart_main
 ****************************************************************************/

int mtdpart_main(int argc, char *argv[])
{
  FAR struct mtd_dev_s *master;
  FAR struct mtd_dev_s *part[CONFIG_EXAMPLES_MTDPART_NPARTITIONS];
  FAR struct mtd_geometry_s geo;
  FAR uint32_t *buffer;
  char blockname[32];
  char charname[32];
  ssize_t nbytes;
  off_t nblocks;
  off_t offset;
  off_t check;
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
      message("ERROR: Failed to create RAM MTD instance\n");
      msgflush();
      exit(1);
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
      message("ERROR: ftl_initialize /dev/mtdblock0 failed: %d\n", ret);
      msgflush();
      exit(2);
    }

  /* Now create a character device on the block device */

  ret = bchdev_register("/dev/mtdblock0", "/dev/mtd0", false);
  if (ret < 0)
    {
      message("ERROR: bchdev_register /dev/mtd0 failed: %d\n", ret);
      msgflush();
      exit(3);
    }

  /* Get the geometry of the FLASH device */

  ret = master->ioctl(master, MTDIOC_GEOMETRY, (unsigned long)((uintptr_t)&geo));
  if (ret < 0)
    {
      fdbg("ERROR: mtd->ioctl failed: %d\n", ret);
      exit(3);
    }

  message("Flash Geometry:\n");
  message("  blocksize:    %uld\n", (unsigned long)geo.blocksize);
  message("  erasesize:    %uld\n", (unsigned long)geo.erasesize);
  message("  neraseblocks: %uld\n", (unsigned long)geo.neraseblocks);
  
  /* Determine the size of each partition */

  blkpererase = geo.erasesize / geo.blocksize;
  nblocks = geo.neraseblocks * blkpererase / CONFIG_EXAMPLES_MTDPART_NPARTITIONS;

  /* Now create MTD FLASH partitions */

  for (offset = 0, i = 1;
       i <= CONFIG_EXAMPLES_MTDPART_NPARTITIONS;
       offset += nblocks, i++)
    {
      /* Create the partition */

      part[i] = mtd_partition(master, offset, nblocks);
      if (!part[i])
        {
          message("ERROR: mtd_partition failed. offset=%uld nblocks=%uld\n",
                  (unsigned long)offset, (unsigned long)nblocks);
          msgflush();
          exit(4);
        }

      /* Initialize to provide an FTL block driver on the MTD FLASH interface */

      snprintf(blockname, 32, "/dev/mtdblock%d", i);
      snprintf(charname, 32, "/dev/mtd%d", i);

      ret = ftl_initialize(i, part[i]);
      if (ret < 0)
        {
          message("ERROR: ftl_initialize %s failed: %d\n", blockname, ret);
          msgflush();
          exit(5);
        }

      /* Now create a character device on the block device */

      ret = bchdev_register(blockname, charname, false);
      if (ret < 0)
        {
          message("ERROR: bchdev_register %s failed: %d\n", charname, ret);
          msgflush();
          exit(6);
        }
    }

  /* Allocate a buffer */

  buffer = (FAR uint32_t *)malloc(geo.blocksize);
  if (!buffer)
    {
      message("ERROR: failed to allocate a sector buffer\n");
      msgflush();
      exit(7);
    }

  /* Open the master MTD FLASH character driver for writing */

  fd = open("/dev/mtd0", O_WRONLY);
  if (fd < 0)
    {
      message("ERROR: open /dev/mtd0 failed: %d\n", errno);
      msgflush();
      exit(8);
    }

  /* Now write the offset into every block */

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
              message("ERROR: write to /dev/mtd0 failed: %d\n", errno);
              msgflush();
              exit(9);
            }
        }
    }

  close(fd);

  /* Now read each partition */

  for (offset = 0, i = 1;
       i <= CONFIG_EXAMPLES_MTDPART_NPARTITIONS;
       offset += nblocks, i++)
    {
      /* Open the master MTD partition character driver for writing */

      snprintf(charname, 32, "/dev/mtd%d", i);
      fd = open(charname, O_RDONLY);
      if (fd < 0)
        {
          message("ERROR: open %s failed: %d\n", charname, errno);
          msgflush();
          exit(10);
        }

      /* Now verify the offset in every block */

      check = offset;
      for (i = 0; i < nblocks; i++)
        {
          for (j = 0; j < blkpererase; j++)
            {
              /* Read the next block into memory */

              nbytes = read(fd, buffer, geo.blocksize);
              if (nbytes < 0)
                {
                  message("ERROR: read from %s failed: %d\n", charname, errno);
                  msgflush();
                  exit(11);
                }

              /* Verfy the offsets in the block */

              check = offset;
              for (k = 0; k < geo.blocksize / sizeof(uint32_t); k++)
                {
                  if (buffer[k] != check)
                    {
                      message("ERROR: Bad offset %uld, expected %uld\n",
                              buffer[k], check);
                      msgflush();
                      exit(12);
                    }

                  check += 4;
                }
            }
        }

      close(fd);
    }

  /* And exit without bothering to clean up */

  msgflush();
  return 0;
}

