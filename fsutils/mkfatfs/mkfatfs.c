/****************************************************************************
 * apps/fsutils/mkfatfs/writefat.c
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
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/fs/fs.h>

#include "fsutils/mkfatfs.h"
#include "fat32.h"
#include "mkfatfs.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fat_systime2fattime
 *
 * Description:
 *   Get the system time convert to a time and and date suitable for
 *   writing into the FAT FS.
 *
 *    TIME in LS 16-bits:
 *      Bits 0:4   = 2 second count (0-29 representing 0-58 seconds)
 *      Bits 5-10  = minutes (0-59)
 *      Bits 11-15 = hours (0-23)
 *    DATE in MS 16-bits
 *      Bits 0:4   = Day of month (1-31)
 *      Bits 5:8   = Month of year (1-12)
 *      Bits 9:15  = Year from 1980 (0-127 representing 1980-2107)
 *
 ****************************************************************************/

static uint32_t fat_systime2fattime(void)
{
  /* Unless you have a hardware RTC or some other to get accurate time, then
   * there is no reason to support FAT time.
   */

#ifdef CONFIG_FS_FATTIME
  struct timespec ts;
  struct tm tm;
  int ret;

  /* Get the current time in seconds and nanoseconds */

  ret = clock_gettime(CLOCK_REALTIME, &ts);
  if (ret == OK)
    {
      /* Break done the seconds in date and time units */

      if (gmtime_r((FAR const time_t *)&ts.tv_sec, &tm) != NULL)
        {
          /* FAT can only represent dates since 1980.  struct tm can
           * represent dates since 1900.
           */

          if (tm.tm_year >= 80)
            {
              uint16_t fattime;
              uint16_t fatdate;

              fattime  = (tm.tm_sec         >>  1) & 0x001f; /* Bits 0-4: 2 second count (0-29) */
              fattime |= (tm.tm_min         <<  5) & 0x07e0; /* Bits 5-10: minutes (0-59) */
              fattime |= (tm.tm_hour        << 11) & 0xf800; /* Bits 11-15: hours (0-23) */

              fatdate  =  tm.tm_mday               & 0x001f; /* Bits 0-4: Day of month (1-31) */
              fatdate |= ((tm.tm_mon + 1)   <<  5) & 0x01e0; /* Bits 5-8: Month of year (1-12) */
              fatdate |= ((tm.tm_year - 80) <<  9) & 0xfe00; /* Bits 9-15: Year from 1980 */

              return (uint32_t)fatdate << 16 | (uint32_t)fattime;
            }
        }
    }
#endif

  return 0;
}

/****************************************************************************
 * Name: mkfatfs_getgeometry
 *
 * Description:
 *   Get the sector size and number of sectors of the underlying block
 *   device.
 *
 * Input:
 *   fmt - Caller specified format parameters
 *   var - Other format parameters that are not caller specifiable. (Most
 *     set by mkfatfs_configfatfs()).
 *
 * Return:
 *   Zero on success; negated errno on failure
 *
 ****************************************************************************/

static inline int mkfatfs_getgeometry(FAR struct fat_format_s *fmt,
                                      FAR struct fat_var_s *var)
{
  struct geometry geometry;
  int ret;

  /* Get the geometry of the underlying device */

  ret = ioctl(var->fv_fd, BIOC_GEOMETRY,
              (unsigned long)((uintptr_t)&geometry));
  if (ret < 0)
    {
      ferr("ERROR: geometry() returned %d\n", ret);
      return ret;
    }

  if (!geometry.geo_available || !geometry.geo_writeenabled)
    {
      ferr("ERROR: Media is not available\n", ret);
      return -ENODEV;
    }

  /* Check if the user provided maxblocks was provided and, if so, that is it
   * less than the actual number of blocks on the device.
   */

  if (fmt->ff_nsectors != 0)
    {
      if (fmt->ff_nsectors > geometry.geo_nsectors)
        {
          ferr("ERROR: User maxblocks (%d) exceeds blocks on device (%d)\n",
               fmt->ff_nsectors, geometry.geo_nsectors);

          return -EINVAL;
        }
    }
  else
    {
      /* Use the actual number of blocks on the device */

      fmt->ff_nsectors = geometry.geo_nsectors;
    }

  /* Verify that we can handle this sector size */

  var->fv_sectorsize = geometry.geo_sectorsize;
  switch (var->fv_sectorsize)
    {
      case 512:
        var->fv_sectshift = 9;
        break;

      case 1024:
        var->fv_sectshift = 10;
        break;

      case 2048:
        var->fv_sectshift = 11;
        break;

      case 4096:
        var->fv_sectshift = 12;
        break;

      default:
        ferr("ERROR: Unsupported sector size: %d\n", var->fv_sectorsize);
        return -EPERM;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mkfatfs
 *
 * Description:
 *   Make a FAT file system image on the specified block device.  This
 *   function can automatically format a FAT12 or FAT16 file system.  By
 *   tradition, FAT32 will only be selected is explicitly requested.
 *
 * Inputs:
 *   pathname - the full path to a registered block driver
 *   fmt - Describes characteristics of the desired filesystem
 *
 * Return:
 *   Zero (OK) on success; -1 (ERROR) on failure with errno set:
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

int mkfatfs(FAR const char *pathname, FAR struct fat_format_s *fmt)
{
  struct fat_var_s var;
  int ret;

  /* Initialize */

  memset(&var, 0, sizeof(struct fat_var_s));

  /* Get the filesystem creation time */

  var.fv_createtime = fat_systime2fattime();

  /* Verify format options (only when DEBUG enabled) */

#ifdef CONFIG_DEBUG_FEATURES
  if (!pathname)
    {
      ferr("ERROR: No block driver path\n");
      ret = -EINVAL;
      goto errout;
    }

  if (fmt->ff_nfats < 1 || fmt->ff_nfats > 4)
    {
      ferr("ERROR: Invalid number of fats: %d\n", fmt->ff_nfats);
      ret = -EINVAL;
      goto errout;
    }

  if (fmt->ff_fattype != 0  && fmt->ff_fattype != 12 &&
      fmt->ff_fattype != 16 && fmt->ff_fattype != 32)
    {
      ferr("ERROR: Invalid FAT size: %d\n", fmt->ff_fattype);
      ret = -EINVAL;
      goto errout;
    }
#endif

  /* 0 will auto-selected by FAT12 and FAT16 (only).  Otherwise,
   * fv_fattype will specify the exact format to use.
   */

  var.fv_fattype = fmt->ff_fattype;

  /* The valid range off ff_clustshift is {0,1,..7} corresponding to
   * cluster sizes of {1,2,..128} sectors.  The special value of 0xff
   * means that we should autoselect the cluster sizel.
   */

#ifdef CONFIG_DEBUG_FEATURES
  if (fmt->ff_clustshift > 7 && fmt->ff_clustshift != 0xff)
    {
      ferr("ERROR: Invalid cluster shift value: %d\n", fmt->ff_clustshift);

      ret = -EINVAL;
      goto errout;
    }

  if (fmt->ff_rootdirentries != 0 &&
      (fmt->ff_rootdirentries < 16 || fmt->ff_rootdirentries > 32767))
    {
      ferr("ERROR: Invalid number of root dir entries: %d\n",
           fmt->ff_rootdirentries);

      ret = -EINVAL;
      goto errout;
    }

  if (fmt->ff_rsvdseccount != 0 && (fmt->ff_rsvdseccount < 1 ||
      fmt->ff_rsvdseccount > 32767))
    {
      ferr("ERROR: Invalid number of reserved sectors: %d\n",
           fmt->ff_rsvdseccount);

      ret = -EINVAL;
      goto errout;
    }
#endif

  /* Find the inode of the block driver identified by 'source' */

  var.fv_fd = open(pathname, O_RDWR);
  if (var.fv_fd < 0)
    {
      ret = -errno;
      ferr("ERROR: Failed to open %s: %d\n", pathname, ret);
      goto errout;
    }

  /* Determine the volume configuration based upon the input values and upon
   * the reported device geometry.
   */

  ret = mkfatfs_getgeometry(fmt, &var);
  if (ret < 0)
    {
      goto errout_with_driver;
    }

  /* Configure the file system */

  ret = mkfatfs_configfatfs(fmt, &var);
  if (ret < 0)
    {
      goto errout_with_driver;
    }

  /* Allocate a buffer that will be working sector memory */

  var.fv_sect = (FAR uint8_t *)malloc(var.fv_sectorsize);
  if (!var.fv_sect)
    {
      ferr("ERROR: Failed to allocate working buffers\n");
      goto errout_with_driver;
    }

  /* Write the filesystem to media */

  ret = mkfatfs_writefatfs(fmt, &var);

errout_with_driver:

  /* Close the driver */

  close(var.fv_fd);

errout:

  /* Release all allocated memory */

  if (var.fv_sect)
    {
      free(var.fv_sect);
    }

  /* Return any reported errors */

  if (ret < 0)
    {
      errno = -ret;
      return ERROR;
    }

  return OK;
}
