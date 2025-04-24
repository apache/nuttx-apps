/****************************************************************************
 * apps/boot/nxboot/loader/flash.c
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
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <nuttx/fs/fs.h>
#include <nuttx/mtd/mtd.h>

#include "flash.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: flash_partition_open
 *
 * Description:
 *   Opens the partition based on a given name and returns the file
 *   descriptor to it.
 *
 * Input parameters:
 *   path: Path to the device.
 *
 * Returned Value:
 *   Valid file descriptor on success, -1 on failure.
 *
 ****************************************************************************/

int flash_partition_open(const char *path)
{
  int fd;

  fd = open(path, O_RDWR);
  if (fd < 0)
    {
      syslog(LOG_ERR, "Could not open %s partition: %s\n",
              path, strerror(errno));
      return ERROR;
    }

  return fd;
}

/****************************************************************************
 * Name: flash_partition_close
 *
 * Description:
 *   Closes opened partition.
 *
 * Input parameters:
 *   fd: Valid file descriptor.
 *
 * Returned Value:
 *   0 on success, -1 on failure.
 *
 ****************************************************************************/

int flash_partition_close(int fd)
{
  return close(fd);
}

/****************************************************************************
 * Name: flash_partition_write
 *
 * Description:
 *   Writes count data pointed to by buf at offset off to a partition
 *   referenced by file descriptor fd.
 *
 * Input parameters:
 *   fd: Valid file descriptor.
 *   buf: The pointer to data to be written.
 *   count: Number of bytes to be written.
 *   off: Write offset in bytes.
 *
 * Returned Value:
 *   0 on success, -1 on failure.
 *
 ****************************************************************************/

int flash_partition_write(int fd, const void *buf, size_t count, off_t off)
{
  int ret;
  off_t pos;
  size_t size;
  ssize_t nbytes;
  struct mtd_geometry_s geometry;

  ret = ioctl(fd, MTDIOC_GEOMETRY, (unsigned long)((uintptr_t)&geometry));
  if (ret < 0)
    {
      syslog(LOG_ERR, "ioctl MTDIOC_GEOMETRY failed: %s\n", strerror(errno));
      return ERROR;
    }

  size = geometry.erasesize * geometry.neraseblocks;
  if (count + off > size)
    {
      syslog(LOG_ERR, "Trying to write outside of flash area.\n");
      return ERROR;
    }

  pos = lseek(fd, off, SEEK_SET);
  if (pos != off)
    {
      syslog(LOG_ERR, "Could not seek to %ld: %s\n", off, strerror(errno));
      return ERROR;
    }

  nbytes = write(fd, buf, count);
  if (nbytes != count)
    {
      syslog(LOG_ERR, "Write to offset %ld failed %s\n",
              off, strerror(errno));
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: flash_partition_read
 *
 * Description:
 *   Read count data to buffer buf at offset off from a partition
 *   referenced by file descriptor fd.
 *
 * Input parameters:
 *   fd: Valid file descriptor.
 *   buf: The pointer where read data are stored.
 *   count: Number of bytes to be read.
 *   off: Read offset in bytes.
 *
 * Returned Value:
 *   0 on success, -1 on failure.
 *
 ****************************************************************************/

int flash_partition_read(int fd, void *buf, size_t count, off_t off)
{
  int ret;
  off_t pos;
  size_t size;
  ssize_t nbytes;
  struct mtd_geometry_s geometry;

  ret = ioctl(fd, MTDIOC_GEOMETRY, (unsigned long)((uintptr_t)&geometry));
  if (ret < 0)
    {
      syslog(LOG_ERR, "ioctl MTDIOC_GEOMETRY failed: %s\n", strerror(errno));
      return ERROR;
    }

  size = geometry.erasesize * geometry.neraseblocks;
  if (count + off > size)
    {
      syslog(LOG_ERR, "Trying to read outside of flash area.\n");
      return ERROR;
    }

  pos = lseek(fd, off, SEEK_SET);
  if (pos != off)
    {
      syslog(LOG_ERR, "Could not seek to %ld: %s\n", off, strerror(errno));
      return ERROR;
    }

  nbytes = read(fd, buf, count);
  if (nbytes != count)
    {
      syslog(LOG_ERR, "Read from offset %ld failed %s\n", off,
                      strerror(errno));
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: flash_partition_erase
 *
 * Description:
 *   Erases the entire partition.
 *
 * Input parameters:
 *   fd: Valid file descriptor.
 *
 * Returned Value:
 *   0 on success, -1 on failure.
 *
 ****************************************************************************/

int flash_partition_erase(int fd)
{
  int ret;

  ret = ioctl(fd, MTDIOC_BULKERASE, 0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "Could not erase the partition: %s\n",
              strerror(errno));
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: flash_partition_erase_first_sector
 *
 * Description:
 *   Erases the first sector of the partition
 *
 * Input parameters:
 *   fd: Valid file descriptor.
 *
 * Returned Value:
 *   0 on success, -1 on failure.
 *
 ****************************************************************************/

int flash_partition_erase_first_sector(int fd)
{
  int ret;
  struct mtd_erase_s erase;

  erase.startblock = 0;
  erase.nblocks = 1;

  ret = ioctl(fd, MTDIOC_ERASESECTORS, &erase);
  if (ret < 0)
    {
      syslog(LOG_ERR, "Could not erase the partition: %s\n",
                      strerror(errno));
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: flash_partition_info
 *
 * Description:
 *   Returns the size of one block.
 *
 * Input parameters:
 *   fd: Valid file descriptor.
 *   info: Pointer to flash_partition_info structure where info is filled.
 *
 * Returned Value:
 *   Size of the block on success, -1 on failure.
 *
 ****************************************************************************/

int flash_partition_info(int fd, struct flash_partition_info *info)
{
  int ret;
  struct mtd_geometry_s geometry;

  ret = ioctl(fd, MTDIOC_GEOMETRY, (unsigned long)((uintptr_t)&geometry));
  if (ret < 0)
    {
      syslog(LOG_ERR, "ioctl MTDIOC_GEOMETRY failed: %s\n",
                              strerror(errno));
      return ERROR;
    }

  info->blocksize = geometry.blocksize;
  info->size = geometry.erasesize * geometry.neraseblocks;
  info->neraseblocks = geometry.neraseblocks;
  info->erasesize = geometry.erasesize;

  return OK;
}
