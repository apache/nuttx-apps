/****************************************************************************
 * apps/system/flash_test/flash_test.c
 *
 *   Copyright (C) 2013 Ken Pettit. All rights reserved.
 *   Author: Ken Pettit <pettitkd@gmail.com>
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

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  struct inode* inode;
  int           ret;
  int           x;
  int           logsector;
  uint16_t      seq;
  struct smart_format_s fmt;
  uint16_t     *sectors;
  uint16_t     *seqs;
  char         *buffer;
  struct smart_read_write_s readwrite;

  /* Argument given? */

  if (argc < 2)
    {
      fprintf(stderr, "usage: flash_test flash_block_device\n");
      return -1;
    }

  /* Find the inode of the block driver identified by 'source' */

  ret = open_blockdriver(argv[1], 0, &inode);
  if (ret < 0)
    {
      fprintf(stderr, "Failed to open %s\n", argv[1]);
      goto errout;
    }

  /* Get the low-level format from the device. */

  ret = inode->u.i_bops->ioctl(inode, BIOC_GETFORMAT, (unsigned long) &fmt);
  if (ret != OK)
    {
      fprintf(stderr, "Device is not a SMART block device\n");
      goto errout_with_driver;
    }

  /* Test if the device is formatted.  If not, then we must do a
   * low-level format first */

  if (!(fmt.flags & SMART_FMT_ISFORMATTED))
    {
      /* Perform a low-level format */

      inode->u.i_bops->ioctl(inode, BIOC_LLFORMAT, 0);
      inode->u.i_bops->ioctl(inode, BIOC_GETFORMAT, (unsigned long) &fmt);
    }

  if (!(fmt.flags & SMART_FMT_ISFORMATTED))
    {
      fprintf(stderr, "Unable to get device format\n");
      goto errout_with_driver;
    }

  /* Report the device structure */

  printf("FLASH Test on device with:\n");
  printf("   Sector size:  %10d\n", fmt.sectorsize);
  printf("   Sector count: %10d\n", fmt.nsectors);
  printf("   Avail bytes:  %10d\n", fmt.availbytes);
  printf("   Total size:   %10d\n", fmt.sectorsize * fmt.nsectors);

  /* Allocate buffers to use */

  buffer = (char *) malloc(fmt.availbytes);
  if (buffer == NULL)
    {
      fprintf(stderr, "Error allocating buffer\n");
      goto errout_with_driver;
    }

  seqs = (uint16_t *) malloc(fmt.nsectors << 1);
  if (seqs == NULL)
    {
      free(buffer);
      fprintf(stderr, "Error allocating seqs buffer\n");
      goto errout_with_driver;
    }

  sectors = (uint16_t *) malloc(fmt.nsectors << 1);
  if (sectors == NULL)
    {
      free(seqs);
      free(buffer);
      fprintf(stderr, "Error allocating sectors buffer\n");
      goto errout_with_driver;
    }

  /* Write a bunch of data to the flash */

  printf("\nAllocating and writing to logical sectors\n");

  seq = 0;
  for (x = 0; x < fmt.nsectors >> 1; x++)
    {
      /* Allocate a new sector */

      logsector = inode->u.i_bops->ioctl(inode, BIOC_ALLOCSECT,
                                         (unsigned long) -1);
      if (logsector < 0)
        {
          fprintf(stderr, "Error allocating sector: %d\n", logsector);
          goto errout_with_driver;
        }

      /* Save the sector in our array */

      sectors[x] = (uint16_t) logsector;
      seqs[x] = seq++;

      /* Now write some data to the sector */

      sprintf(buffer, "Logical sector %d sequence %d\n",
              sectors[x], seqs[x]);

      readwrite.logsector = sectors[x];
      readwrite.offset = 0;
      readwrite.count = strlen(buffer) + 1;
      readwrite.buffer = (uint8_t *) buffer;
      inode->u.i_bops->ioctl(inode, BIOC_WRITESECT, (unsigned long)
                             &readwrite);

      /* Print the logical sector number */

      printf("\r%d    ", sectors[x]);
    }

  /* Now read the data back to validate everything was written and can
   * be read. */

  printf("\nDoing read verify test\n");
  for (x = 0; x < fmt.nsectors >> 1; x++)
    {
      /* Read from the logical sector */

      readwrite.logsector = sectors[x];
      readwrite.offset = 0;
      readwrite.count = fmt.availbytes;
      readwrite.buffer = (uint8_t *) buffer;
      ret = inode->u.i_bops->ioctl(inode, BIOC_READSECT, (unsigned long)
                                   &readwrite);

      if (ret != fmt.availbytes)
        {
          fprintf(stderr, "Error reading sector %d\n", sectors[x]);
          goto errout_with_buffers;
        }

      /* Generate compare string and do the compare */

      printf("\r%d     ", sectors[x]);

      sprintf(&buffer[100], "Logical sector %d sequence %d\n",
              sectors[x], seqs[x]);

      if (strcmp(buffer, &buffer[100]) != 0)
        {
          printf("Sector %d read verify failed\n", sectors[x]);
        }
    }

  printf("\nPeforming sector re-write\n");

  /* Overwrite data on the sectors to cause relocation */

  for (x = 0; x < fmt.nsectors >> 1; x++)
    {
      /* Save a new sequence number for the sector */

      seqs[x] = seq++;

      /* Now write over the sector data with new data, causing a relocation. */

      sprintf(buffer, "Logical sector %d sequence %d\n", sectors[x], seqs[x]);
      readwrite.logsector = sectors[x];
      readwrite.offset = 0;
      readwrite.count = strlen(buffer) + 1;
      readwrite.buffer = (uint8_t *) buffer;
      inode->u.i_bops->ioctl(inode, BIOC_WRITESECT, (unsigned long)
                             &readwrite);

      /* Print the logical sector number */

      printf("\r%d    ", sectors[x]);
    }

  /* Now append data to empty region of sector */

  printf("\nAppending data to empty region of sector\n");

  for (x = 0; x < fmt.nsectors >> 1; x++)
    {
      /* Save a new sequence number for the sector */

      seqs[x] = seq++;

      /* Now write over the sector data with new data, causing a relocation. */

      sprintf(buffer, "Appended data in sector %d\n", sectors[x]);
      readwrite.logsector = sectors[x];
      readwrite.offset = 64;
      readwrite.count = strlen(buffer) + 1;
      readwrite.buffer = (uint8_t *) buffer;
      inode->u.i_bops->ioctl(inode, BIOC_WRITESECT, (unsigned long)
                             &readwrite);

      /* Print the logical sector number */

      printf("\r%d    ", sectors[x]);
    }

  printf("\nDone\n");

  /* Free all the allocated sectors */

  for (x = 0; x < fmt.nsectors >> 1; x++)
    {
      /* Only free the sector if it is still valid */

      if (sectors[x] != 0xFFFF)
        {
          inode->u.i_bops->ioctl(inode, BIOC_FREESECT, (unsigned long)
                                 sectors[x]);
        }
    }

errout_with_buffers:
  /* Free the allocated buffers */

  free(seqs);
  free(sectors);
  free(buffer);

errout_with_driver:
  /* Now close the block device and exit */

  close_blockdriver(inode);

errout:

  return 0;
}
