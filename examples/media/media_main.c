/****************************************************************************
 * examples/media/hello_main.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
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

#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/fs/fs.h>
#include <nuttx/mtd/mtd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

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
      printf("MTD Geometry:\n");
      printf("  blocksize:    %u\n", (unsigned int)mtdgeo.blocksize);
      printf("  erasesize:    %lu\n", (unsigned long)mtdgeo.erasesize);
      printf("  neraseblocks: %lu\n", (unsigned long)mtdgeo.neraseblocks);

      info->blocksize = mtdgeo.erasesize;
      info->nblocks   = mtdgeo.neraseblocks;
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

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int media_main(int argc, char *argv[])
#endif
{
  FAR uint8_t *buffer;
  struct media_info_s info;
  int fd;

  /* Open the character driver that wraps the media */

  fd = open(CONFIG_EXAMPLES_MEDIA_DEVPATH, O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: failed to open %s: %d\n",
              CONFIG_EXAMPLES_MEDIA_DEVPATH, errno);
      return 1;
    }

  /* Get the block size to use */

  get_blocksize(fd, &info);
  printf("Using:\n");
  printf("  blocksize:    %lu\n", (unsigned long)info.blocksize);
  printf("  nblocks:      %lu\n", (unsigned long)info.nblocks);

  /* Allocate an I/O buffer of the correct block size */

  buffer = (FAR uint8_t *)malloc((size_t)info.blocksize);
  if (buffer == NULL)
    {
      fprintf(stderr, "ERROR: failed to allocate I/O buffer of size %ul\n",
              (unsigned long)info.blocksize);
      close(fd);
      return 1;
    }

  /* Clean-up and exit */

  free(buffer);
  close(fd);
  return 0;
}
