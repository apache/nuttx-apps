/****************************************************************************
 * apps/fsutils/mksmartfs/mksmartfs.c
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

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#  ifdef CONFIG_FS_SMARTFS
#    include "fsutils/mksmartfs.h"
#    include <nuttx/fs/ioctl.h>
#    include <nuttx/fs/smart.h>
#  endif
#endif

#include <unistd.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: issmartfs
 *
 * Description:
 *   Check a SMART (Sector Mapped Allocation for Really Tiny) Flash file
 *   system image on the specified block device (must be a SMART device).
 *
 * Inputs:
 *   pathname - the full path to a registered block driver
 *
 * Return:
 *   Zero (OK) on success; -1 (ERROR) on failure with errno set:
 *
 *   EINVAL - NULL block driver string
 *   ENOENT - 'pathname' does not refer to anything in the filesystem.
 *   ENOTBLK - 'pathname' does not refer to a block driver
 *   EFTYPE - the block driver hasn't been formatted yet
 *
 ****************************************************************************/

int issmartfs(FAR const char *pathname)
{
  struct smart_format_s fmt;
  int fd;
  int ret;

  /* Find the inode of the block driver identified by 'source' */

  fd = open(pathname, O_RDONLY);
  if (fd < 0)
    {
      return ERROR;
    }

  /* Get the format information so we know the block have been formatted */

  ret = ioctl(fd, BIOC_GETFORMAT, (unsigned long)&fmt);
  if (ret < 0)
    {
      goto out;
    }

  if (!(fmt.flags & SMART_FMT_ISFORMATTED))
    {
      errno = EFTYPE;
      ret = ERROR;
      goto out;
    }

out:

  /* Close the driver */

  close(fd);
  return ret;
}

/****************************************************************************
 * Name: mksmartfs
 *
 * Description:
 *   Make a SMART Flash file system image on the specified block device
 *
 * Inputs:
 *   pathname   - the full path to a registered block driver
 *   sectorsize - the size of logical sectors on the device from 256-16384.
 *                Setting this to zero will cause the device to be formatted
 *                using the default CONFIG_MTD_SMART_SECTOR_SIZE value.
 *   nrootdirs  - Number of root directory entries to create.
 *
 * Return:
 *   Zero (OK) on success; -1 (ERROR) on failure with errno set.
 *
 * Assumptions:
 *   - The caller must assure that the block driver is not mounted and not in
 *     use when this function is called.  The result of formatting a mounted
 *     device is indeterminate (but likely not good).
 *
 ****************************************************************************/

#ifdef CONFIG_SMARTFS_MULTI_ROOT_DIRS
int mksmartfs(FAR const char *pathname, uint16_t sectorsize,
              uint8_t nrootdirs)
#else
int mksmartfs(FAR const char *pathname, uint16_t sectorsize)
#endif
{
  struct smart_format_s fmt;
  struct smart_read_write_s request;
  uint8_t type;
  int fd;
  int x;
  int ret;

  /* Find the inode of the block driver identified by 'pathname' */

  fd = open(pathname, O_RDWR);
  if (fd < 0)
    {
      ret = -ENOENT;
      goto errout;
    }

  /* Perform a low-level SMART format */

#ifdef CONFIG_SMARTFS_MULTI_ROOT_DIRS
  ret = ioctl(fd, BIOC_LLFORMAT, (sectorsize << 16) | nrootdirs);
#else
  ret = ioctl(fd, BIOC_LLFORMAT, sectorsize << 16);
#endif
  if (ret != OK)
    {
      ret = -errno;
      goto errout_with_driver;
    }

  /* Get the format information so we know how big the sectors are */

  ret = ioctl(fd, BIOC_GETFORMAT, (unsigned long) &fmt);
  if (ret != OK)
    {
      ret = -errno;
      goto errout_with_driver;
    }

  /* Now write the filesystem to media.  Loop for each root dir entry and
   * allocate the reserved Root Dir Entry, then write a blank root dir for
   * it.
   */

  type = SMARTFS_SECTOR_TYPE_DIR;
  request.offset = 0;
  request.count = 1;
  request.buffer = &type;
  x = 0;
#ifdef CONFIG_SMARTFS_MULTI_ROOT_DIRS
  for (; x < nrootdirs; x++)
#endif
    {
      ret = ioctl(fd, BIOC_ALLOCSECT, SMARTFS_ROOT_DIR_SECTOR + x);
      if (ret != SMARTFS_ROOT_DIR_SECTOR + x)
        {
          ret = -EIO;
          goto errout_with_driver;
        }

      /* Mark this block as a directory entry */

      request.logsector = SMARTFS_ROOT_DIR_SECTOR + x;

      /* Issue a write to the sector, single byte */

      ret = ioctl(fd, BIOC_WRITESECT, (unsigned long) &request);
      if (ret != OK)
        {
          ret = -EIO;
          goto errout_with_driver;
        }
    }

errout_with_driver:

  /* Close the driver */

  close(fd);

errout:

  /* Return any reported errors */

  if (ret < 0)
    {
      errno = -ret;
      return ERROR;
    }

  return OK;
}
