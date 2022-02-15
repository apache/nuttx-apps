/****************************************************************************
 * apps/fsutils/mkmbr/mkmbr.c
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

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <endian.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef FAR
#define FAR
#endif

#define PACKED                   __attribute__((packed))
#define PARTITION_BOOTABLE_FLAG  0x80
#define PARTITION_TYPE           0x83  /* GNU/Linux */
#define MBR_BOOT_SIGNATURE       0xAA55

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct PACKED partition_s
{
  uint8_t  bootable_flag;
  uint8_t  first_sector_chs[3];
  uint8_t  type;
  uint8_t  last_sector_chs[3];
  uint32_t first_sector_lba;
  uint32_t sectors_num_lba;
};

struct PACKED mbr_s
{
  uint8_t            bootstrap[446];
  struct partition_s partitions[4];
  uint16_t           boot_signature;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_usage[] = "\
Usage: \n\
\n\
  mkmbr dev p1_start p1_sectors [p2_start p2_sectors [...]]\n\
\n\
Where:\n\
  dev    - block device path\n\
  pX_start   - first sector number or \"auto\" to use next free sector\n\
  pX_sectors - partition's sectors count or \"auto\" to use all free space\n\
\n";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_usage(void)
{
  write(STDERR_FILENO, g_usage, sizeof(g_usage));
}

static void mbr_init(FAR struct mbr_s *mbr)
{
  memset(mbr, 0, sizeof(struct mbr_s));
  mbr->boot_signature = MBR_BOOT_SIGNATURE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  unsigned long bsize;
  unsigned long start;
  unsigned long size;
  unsigned long next;
  struct mbr_s mbr;
  struct stat st;
  int partn;
  int argn;
  int fd;
  int ret;

  if (argc < 4 || argc % 2 != 0)
    {
      print_usage();
      return EINVAL;
    }

  fd = open(argv[1], O_RDWR);
  if (fd < 0)
    {
      return errno;
    }

  ret = fstat(fd, &st);
  if (ret < 0)
    {
      goto err_exit;
    }

  next = 1;
  bsize = st.st_size / 512;
  mbr_init(&mbr);
  for (partn = 0; partn < 4 && next < bsize; partn++)
    {
      argn = 2 * (partn + 1);
      if (!strcmp(argv[argn], "auto"))
        {
          start = next;
        }
      else
        {
          errno = 0;
          start = strtoul(argv[argn], NULL, 0);
          if (errno)
            {
              goto err_exit;
            }
        }

      if (!strcmp(argv[argn + 1], "auto"))
        {
          size = bsize - start;
        }
      else
        {
          errno = 0;
          size = strtoul(argv[argn + 1], NULL, 0);
          if (errno)
            {
              goto err_exit;
            }
        }

      if (start < next || start + size > bsize)
        {
          errno = EINVAL;
          goto err_exit;
        }

      mbr.partitions[partn].first_sector_lba = htole32(start);
      mbr.partitions[partn].sectors_num_lba  = htole32(size);
      mbr.partitions[partn].type             = PARTITION_TYPE;
      next = start + size;
    }

  ret = write(fd, &mbr, sizeof(mbr));
  if (ret >= 0)
    {
      errno = 0;
    }

err_exit:
  close(fd);
  return errno;
}
