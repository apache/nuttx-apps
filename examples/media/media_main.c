/****************************************************************************
 * apps/examples/media/media_main.c
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

#include <sys/ioctl.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/fs/fs.h>
#include <nuttx/mtd/mtd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define START_VALUE 0x20
#define END_VALUE   0x7f

/****************************************************************************
 * Private types
 ****************************************************************************/

struct media_info_s
{
  off_t blocksize;
  off_t nblocks;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * get_blocksize
 ****************************************************************************/

static void get_blocksize(int fd, FAR struct media_info_s *info)
{
  struct mtd_geometry_s mtdgeo;
  int ret;

  /* If the underlying driver is an MTD driver, then we should able to get
   * the erase block size from an MTD ioctl call.
   */

  ret = ioctl(fd, MTDIOC_GEOMETRY, (unsigned long)((uintptr_t)&mtdgeo));
  if (ret >= 0)
    {
      /* Its an MTD device.  Use its geometry */

      printf("MTD Geometry:\n");
      printf("  blocksize:    %u\n", (unsigned int)mtdgeo.blocksize);
      printf("  erasesize:    %lu\n", (unsigned long)mtdgeo.erasesize);
      printf("  neraseblocks: %lu\n", (unsigned long)mtdgeo.neraseblocks);

      info->blocksize = mtdgeo.erasesize;
      info->nblocks   = mtdgeo.neraseblocks;

      /* Attempt to erase the entire MTD device */

      ret = ioctl(fd, MTDIOC_BULKERASE, 0);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: Failed erase the MTD device\n");
        }
    }

  /* Otherwise, use the configured default.  We have no idea of the size
   * of the media in this case.
   */

  else
    {
      info->blocksize = CONFIG_EXAMPLES_MEDIA_BLOCKSIZE;
      info->nblocks   = 0;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * media_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR uint8_t *txbuffer;
  FAR uint8_t *rxbuffer;
  FAR char *devpath;
  struct media_info_s info;
  ssize_t nwritten;
  ssize_t nread;
  off_t blockno;
  off_t pos;
  off_t seekpos;
  off_t i;
  uint32_t nerrors;
  uint8_t value;
  int fd;

  /* Open the character driver that wraps the media */

  if (argc < 2)
    {
      devpath = CONFIG_EXAMPLES_MEDIA_DEVPATH;
    }
  else
    {
      devpath = argv[1];
    }

  fd = open(devpath, O_RDWR);

  if (fd < 0)
    {
      fprintf(stderr, "ERROR: failed to open %s: %d\n",
              devpath, errno);
      return 1;
    }

  /* Get the block size to use */

  get_blocksize(fd, &info);

  printf("Using:\n");
  printf("  blocksize:    %ju\n", (uintmax_t)info.blocksize);
  printf("  nblocks:      %ju\n", (uintmax_t)info.nblocks);

  /* Allocate I/O buffers of the correct block size */

  txbuffer = (FAR uint8_t *)malloc((size_t)info.blocksize);
  if (txbuffer == NULL)
    {
      fprintf(stderr,
              "ERROR: failed to allocate TX I/O buffer of size %ju\n",
              (uintmax_t)info.blocksize);
      close(fd);
      return 1;
    }

  rxbuffer = (FAR uint8_t *)malloc((size_t)info.blocksize);
  if (rxbuffer == NULL)
    {
      fprintf(stderr,
              "ERROR: failed to allocate IRX /O buffer of size %ju\n",
              (uintmax_t)info.blocksize);
      free(txbuffer);
      close(fd);
      return 1;
    }

  /* Write and verify each sector */

  pos     = 0;
  nerrors = 0;
  value   = START_VALUE;

  for (blockno = 0; info.nblocks == 0 || blockno < info.nblocks; blockno++)
    {
      printf("Write/Verify: Block %ju\n", (uintmax_t)blockno);

      /* Fill buffer with a (possibly) unique pattern */

      for (i = 0; i < info.blocksize; i++)
        {
          txbuffer[i] = value;
          if (++value >= END_VALUE)
            {
              value = START_VALUE;
            }
        }

      seekpos = lseek(fd, pos, SEEK_SET);
      if (seekpos == (off_t)-1)
        {
          int errcode = errno;

          fprintf(stderr, "ERROR: lseek to %ju failed: %d\n",
                  (uintmax_t)pos, errcode);
          fprintf(stderr, "ERROR: Aborting at block: %ju\n",
                  (uintmax_t)blockno);
          info.nblocks = blockno;
          break;
        }
      else if (seekpos != pos)
        {
          fprintf(stderr, "ERROR: lseek failed: %ju vs %ju\n",
                  (uintmax_t)seekpos, (uintmax_t) pos);
          fprintf(stderr, "ERROR: Aborting at block: %ju\n",
                  (uintmax_t)blockno);
          info.nblocks = blockno;
          break;
        }

      nwritten = write(fd, txbuffer, info.blocksize);
      if (nwritten < 0)
        {
          int errcode = errno;

          fprintf(stderr, "ERROR: write failed: %d\n", errcode);
          if (errno != EINTR)
            {
              fprintf(stderr, "ERROR: Aborting at block: %ju\n",
                      (uintmax_t)blockno);
              info.nblocks = blockno;
              break;
            }
        }
      else if (nwritten != info.blocksize)
        {
          fprintf(stderr, "ERROR: Unexpected write size: %zu vs. %ju\n",
                  nwritten, (uintmax_t)info.blocksize);
          fprintf(stderr, "ERROR: Aborting at block: %ju\n",
                  (uintmax_t)blockno);
          info.nblocks = blockno;
          break;
        }

      seekpos = lseek(fd, pos, SEEK_SET);
      if (seekpos == (off_t)-1)
        {
          int errcode = errno;

          fprintf(stderr, "ERROR: lseek to %ju failed: %d\n",
                  (uintmax_t)pos, errcode);
          fprintf(stderr, "ERROR: Aborting at block: %ju\n",
                  (uintmax_t)blockno);
          info.nblocks = blockno;
          break;
        }
      else if (seekpos != pos)
        {
          fprintf(stderr, "ERROR: lseek failed: %ju vs %ju\n",
                  (uintmax_t)seekpos, (uintmax_t)pos);
          fprintf(stderr, "ERROR: Aborting at block: %ju\n",
                  (uintmax_t)blockno);
          info.nblocks = blockno;
          break;
        }

      nread = read(fd, rxbuffer, info.blocksize);
      if (nread < 0)
        {
          int errcode = errno;

          fprintf(stderr, "ERROR: read failed: %d\n", errcode);
          if (errno != EINTR)
            {
              fprintf(stderr, "ERROR: Aborting at block: %ju\n",
                      (uintmax_t)blockno);
              info.nblocks = blockno;
              break;
            }
        }
      else if (nread != info.blocksize)
        {
          fprintf(stderr, "ERROR: Unexpected read size: %zu vs. %ju\n",
                  nread, (uintmax_t)info.blocksize);
          fprintf(stderr, "ERROR: Aborting at block: %ju\n",
                  (uintmax_t)blockno);
          info.nblocks = blockno;
          break;
        }
      else
        {
          for (i = 0; i < info.blocksize && nerrors <= 100; i++)
            {
              if (txbuffer[i] != rxbuffer[i])
                {
                  fprintf(stderr,
                          "ERROR: block=%ju offset=%ju.  "
                          "Unexpected value: %02x vs. %02x\n",
                          (uintmax_t)blockno, (uintmax_t)i,
                          rxbuffer[i], txbuffer[i]);
                  nerrors++;
                }
            }

          if (nerrors > 100)
            {
              fprintf(stderr, "ERROR: Too many errors\n");
              fprintf(stderr, "ERROR: Aborting at block: %ju\n",
                      (uintmax_t)blockno);
              info.nblocks = blockno;
              break;
            }
        }

      pos += info.blocksize;
    }

  /* Set the number of blocks if it was unknown before */

  if (info.nblocks == 0)
    {
      info.nblocks = blockno;
    }

  /* Seek to the beginning of the file */

  seekpos = lseek(fd, 0, SEEK_SET);
  if (seekpos == (off_t)-1)
    {
      int errcode = errno;

      fprintf(stderr, "ERROR: lseek to 0 failed: %d\n", errcode);
    }
  else if (seekpos != 0)
    {
      fprintf(stderr, "ERROR: lseek to 0 failed: %ju\n",
              (uintmax_t)seekpos);
    }

  /* Re-read and verify each sector */

  value = START_VALUE;

  for (blockno = 0; blockno < info.nblocks; blockno++)
    {
      printf("Re-read/Verify: Block %ju\n", (uintmax_t)blockno);

      /* Fill buffer with a (possibly) unique pattern */

      for (i = 0; i < info.blocksize; i++)
        {
          txbuffer[i] = value;
          if (++value >= END_VALUE)
            {
              value = START_VALUE;
            }
        }

      nread = read(fd, rxbuffer, info.blocksize);
      if (nread < 0)
        {
          int errcode = errno;

          fprintf(stderr, "ERROR: read failed: %d\n", errcode);
          if (errno != EINTR)
            {
              fprintf(stderr, "ERROR: Aborting at block: %ju\n",
                      (uintmax_t)blockno);
              break;
            }
        }
      else if (nread != info.blocksize)
        {
          fprintf(stderr, "ERROR: Unexpected read size: %zu vs. %ju\n",
                  nread, (uintmax_t)info.blocksize);
          fprintf(stderr, "ERROR: Aborting at block: %ju\n",
                  (uintmax_t)blockno);
          break;
        }
      else
        {
          for (i = 0; i < info.blocksize && nerrors <= 100; i++)
            {
              if (txbuffer[i] != rxbuffer[i])
                {
                  fprintf(stderr,
                          "ERROR: block=%ju offset=%ju.  "
                          "Unexpected value: %02x vs. %02x\n",
                          (uintmax_t)blockno, (uintmax_t)i,
                          rxbuffer[i], txbuffer[i]);
                  nerrors++;
                }
            }

          if (nerrors > 100)
            {
              fprintf(stderr, "ERROR: Too many errors\n");
              fprintf(stderr, "ERROR: Aborting at block: %ju\n",
                      (uintmax_t)blockno);
              info.nblocks = blockno;
              break;
            }
        }
    }

  /* Clean-up and exit */

  free(rxbuffer);
  free(txbuffer);
  close(fd);
  return 0;
}
