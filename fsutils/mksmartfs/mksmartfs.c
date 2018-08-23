/****************************************************************************
 * apps/fsutils/mksmartfs/mksmartfs.c
 * Implementation of the SMARTFS mksmartfs utility
 *
 *   Copyright (C) 2015, Ken Pettit. All rights reserved.
 *   Author: Ken Pettit <pettitkd@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#  include <nuttx/config.h>

#if CONFIG_NFILE_DESCRIPTORS > 0
# include <sys/stat.h>
# include <sys/ioctl.h>
# include <fcntl.h>
#include <errno.h>
# if !defined(CONFIG_DISABLE_MOUNTPOINT)
#   ifdef CONFIG_FS_SMARTFS
#     include "fsutils/mksmartfs.h"
#     include <nuttx/fs/ioctl.h>
#     include <nuttx/fs/smart.h>
#   endif
#endif
#endif

# include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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
 *   Zero (OK) on success; -1 (ERROR) on failure with errno set appropriately:
 *
 *   EINVAL - NULL block driver string
 *   ENOENT - 'pathname' does not refer to anything in the filesystem.
 *   ENOTBLK - 'pathname' does not refer to a block driver
 *   EFTYPE - the block driver hasn't been formated yet
 *
 ****************************************************************************/

int issmartfs(FAR const char *pathname)
{
  struct smart_format_s fmt;
  int ret, fd;

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
      set_errno(EFTYPE);
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
 *   Zero (OK) on success; -1 (ERROR) on failure with errno set appropriately:
 *
 *   EINVAL  - NULL block driver string, bad number of FATS in 'fmt', bad FAT
 *     size in 'fmt', bad cluster size in 'fmt'
 *   ENOENT  - 'pathname' does not refer to anything in the filesystem.
 *   ENOTBLK - 'pathname' does not refer to a block driver
 *   EACCES  - block driver does not support wrie or geometry methods
 *
 * Assumptions:
 *   - The caller must assure that the block driver is not mounted and not in
 *     use when this function is called.  The result of formatting a mounted
 *     device is indeterminate (but likely not good).
 *
 ****************************************************************************/

#ifdef CONFIG_SMARTFS_MULTI_ROOT_DIRS
int mksmartfs(FAR const char *pathname, uint16_t sectorsize, uint8_t nrootdirs)
#else
int mksmartfs(FAR const char *pathname, uint16_t sectorsize)
#endif
{
  struct smart_format_s fmt;
  int ret, fd;
  int x;
  uint8_t type;
  struct smart_read_write_s request;

  /* Find the inode of the block driver indentified by 'source' */

  fd = open(pathname, O_RDWR);
  if (fd < 0)
    {
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
      goto errout_with_driver;
    }

  /* Get the format information so we know how big the sectors are */

  ret = ioctl(fd, BIOC_GETFORMAT, (unsigned long) &fmt);

  /* Now Write the filesystem to media.  Loop for each root dir entry and
   * allocate the reserved Root Dir Enty, then write a blank root dir for it.
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
      if (ret != 0)
        {
          ret = -EIO;
          goto errout_with_driver;
        }
    }

errout_with_driver:
  /* Close the driver */

  (void)close(fd);

errout:
  /* Release all allocated memory */

  /* Return any reported errors */

  if (ret < 0)
    {
      set_errno(-ret);
      return ERROR;
    }

  return OK;
}

