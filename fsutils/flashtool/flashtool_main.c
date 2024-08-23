/****************************************************************************
 * apps/fsutils/flashtool/flashtool_main.c
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
#include <nuttx/mtd/mtd.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <debug.h>
#include <errno.h>
#include <ctype.h>

#include <sys/ioctl.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum opr_list
{
  HELP, DEVICE_INFO, READ_PAGE, WRITE_PAGE, ERASE_BLOCK, ERASE_ALL,
  CHECK_BAD_BLOCK
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void display_help(void)
{
  printf("Usage : flashtool [OPTION [ARG]] device_name ...\n");
  printf("-h         show this help statement\n");
  printf("-i         display device geometry information\n");
  printf("-r <page>  read the specified pages\n");
  printf("-w <page>  write the specified pages\n");
  printf("-e <block> erase the specified blocks\n");
  printf("-n <num>   number of pages or blocks\n");
  printf("-f <file>  file name for read or write\n");
  printf("-b <block> check whether <block> is bad\n");
  printf("-c         erase all of the flash\n");
  printf("-d <device> device name\n");
}

static void display_device_info(FAR const struct mtd_geometry_s *geo)
{
  printf("Size of one read/write page: %lu\n", geo->blocksize);
  printf("Size of one erase block:     %lu\n", geo->erasesize);
  printf("Number of erase blocks:      %lu\n", geo->neraseblocks);
}

static int read_page(int fd, FAR const struct mtd_geometry_s *geo,
                     off_t start_page, size_t npages)
{
  FAR unsigned char *buf;
  ssize_t ret = OK;
  off_t seek;
  size_t i;

  buf = malloc(geo->blocksize);
  if (!buf)
    {
      printf("ERROR: read_page cannot allocate memory %d\n", geo->blocksize);
      return ERROR;
    }

  seek = geo->blocksize * start_page;
  if (lseek(fd, seek, SEEK_SET) != seek)
    {
      printf("ERROR: Cannot seek mtd to offset %lld: %d", seek, errno);
      goto oops;
    }

  printf("Flash pages contents:\n");
  for (i = 0; i < npages; i++)
    {
      ret = read(fd, buf, geo->blocksize);
      if (ret < 0)
        {
          printf("ERROR: Read error: %d!\n", errno);
          goto oops;
        }

      printf("Flash page %lld:\n", start_page + i);
      lib_dumpbuffer(NULL, buf, ret);
    }

oops:
  free(buf);
  return ret;
}

/* Write npages data from img to flash
 * If size of img is less than npags, only write img size
 */

static int write_page(int fd, FAR const struct mtd_geometry_s *geo,
                      off_t start_page, size_t npages, FAR const char *img)
{
  FAR unsigned char *buf;
  off_t imglen;
  off_t seek;
  off_t i;
  int fd_src;
  int ret = OK;

  if (img == NULL)
    {
      printf("ERROR: Need source file path!\n");
      return ERROR;
    }

  printf("Write data from %s...\n", img);
  seek = geo->blocksize * start_page;
  if (lseek(fd, seek, SEEK_SET) != seek)
    {
      printf("ERROR: cannot seek mtd to offset %lld: %d", seek, errno);
      return ERROR;
    }

  fd_src = open(img, O_RDONLY);
  if (fd_src < 0)
    {
      printf("ERROR: Open file %s failed: %d\n", img, errno);
      return ERROR;
    }

  imglen = lseek(fd_src, 0, SEEK_END);
  if (lseek(fd_src, 0, SEEK_SET) != 0)
    {
      close(fd_src);
      return ERROR;
    }

  /* Malloc a size of page for buf */

  buf = malloc(geo->blocksize);
  if (!buf)
    {
      printf("ERROR: write_page cannot allocate memory %d\n",
             geo->blocksize);
      close(fd_src);
      return ERROR;
    }

  for (i = start_page; i < start_page + npages; i++)
    {
      ret = read(fd_src, buf,
                (imglen < geo->blocksize) ? imglen : geo->blocksize);
      if (ret < 0)
        {
          printf("ERROR: read source file error: %d!\n", errno);
          goto oops;
        }

      ret = write(fd, buf, geo->blocksize);
      if (ret < 0)
        {
          printf("ERROR: write error: %d!\n", errno);
          goto oops;
        }

      imglen -= geo->blocksize;
      if (imglen <= 0)
        {
          break;
        }
    }

oops:
  close(fd_src);
  free(buf);
  return ret;
}

static int erase_block(int fd, off_t start_block,
                       size_t nblocks)
{
  struct mtd_erase_s erase;
  int ret;

  printf("Erase block from %lld to %lld!\n", start_block,
         start_block + nblocks -1);
  erase.startblock = start_block;
  erase.nblocks = nblocks;
  ret = ioctl(fd, MTDIOC_ERASESECTORS, &erase);
  if (ret < 0)
    {
      printf("ERROR: Erase failed: %d!\n", errno);
    }

  return ret;
}

static int erase_all(int fd)
{
  if (ioctl(fd, MTDIOC_BULKERASE, 0) < 0)
    {
      printf("ERROR: MTD ioctl(MTDIOC_BULKERASE) failed: %d\n", errno);
      return ERROR;
    }

  return OK;
}

static int check_bad_block(int fd, FAR const struct mtd_geometry_s *geo)
{
  struct mtd_bad_block_s bad_block;
  int flag = 0;
  int ret;
  int i;

  printf("Check bad blocks in flash......\n");
  for (i = 0; i < geo->neraseblocks; i++)
    {
      bad_block.block_num = i;
      ret = ioctl(fd, MTDIOC_ISBAD, &bad_block);
      if (ret != OK)
        {
          printf("ERROR: MTDIOC_ISBAD failed: %d!\n", errno);
          return ERROR;
        }
      else
        {
          if (bad_block.bad_flag)
            {
              printf("Block %d is bad!\n", i);
              flag = 1;
            }
        }
    }

  if (flag == 0)
    {
      printf("No bad blocks!\n");
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR char *mtddev = NULL;
  FAR char *img = NULL;
  struct mtd_geometry_s geo;
  enum opr_list opr = HELP;
  off_t npages = 1;
  off_t start;
  int option;
  int mode = O_RDONLY;
  int fd;
  int ret;

  while ((option = getopt(argc, argv, ":hir:w:e:n:f:bcd:")) != -1)
    {
      switch (option)
        {
          case 'h':
            display_help();
            exit(0);
          case 'i':
            opr = DEVICE_INFO;
            break;
          case 'r':
            opr = READ_PAGE;
            start = strtoul(optarg, NULL, 0);
            break;
          case 'w':
            opr = WRITE_PAGE;
            mode = O_RDWR;
            start = strtoul(optarg, NULL, 0);
            break;
          case 'e':
            opr = ERASE_BLOCK;
            start = strtoul(optarg, NULL, 0);
            break;
          case 'n':
            npages = strtoul(optarg, NULL, 0);
            break;
          case 'f':
            img = optarg;
            break;
          case 'b':
            opr = CHECK_BAD_BLOCK;
            break;
          case 'c':
            opr = ERASE_ALL;
            break;
          case 'd':
            mtddev = optarg;
            break;
          case ':':
            printf("Error: Missing required argument\n");
            exit(1);
          case '?':
            printf("Error: Unrecognized option\n");
            exit(1);
        }
    }

  if (mtddev == NULL)
    {
      printf("Error: Device name is required!\n");
      return -1;
    }

  fd = open(mtddev, mode, 0777);
  if (fd < 0)
    {
      printf("ERROR: Failed to open '%s': %d\n", mtddev, errno);
      return -1;
    }

  ret = ioctl(fd, MTDIOC_GEOMETRY, &geo);
  if (ret < 0)
    {
      printf("ERROR: Ioctl MTDIOC_GEOMETRY failed: %d\n", errno);
      goto oops;
    }

  switch (opr)
    {
      case DEVICE_INFO:
        display_device_info(&geo);
        break;
      case READ_PAGE:
        ret = read_page(fd, &geo, start, npages);
        break;
      case WRITE_PAGE:
        ret = write_page(fd, &geo, start, npages, img);
        break;
      case ERASE_BLOCK:
        ret = erase_block(fd, start, npages);
        break;
      case ERASE_ALL:
        ret = erase_all(fd);
        break;
      case CHECK_BAD_BLOCK:
        ret = check_bad_block(fd, &geo);
        break;
      default:
    }

oops:
  close(fd);
  return ret;
}
