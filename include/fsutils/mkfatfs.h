/****************************************************************************
 * apps/include/fsutils/mkfatfs.h
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

#ifndef __APPS_INCLUDE_FSUTILS_MKFATFS_H
#define __APPS_INCLUDE_FSUTILS_MKFATFS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MKFATFS_DEFAULT_NFATS        2     /* 2: Default number of FATs */
#define MKFATFS_DEFAULT_FATTYPE      0     /* 0: Autoselect FAT size */
#define MKFATFS_DEFAULT_CLUSTSHIFT   0xff  /* 0xff: Autoselect cluster size */
#define MKFATFS_DEFAULT_VOLUMELABEL  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' }
#define MKFATFS_DEFAULT_BKUPBOOT     0     /* 0: Determine sector number of the backup boot sector */
#define MKFATFS_DEFAULT_ROOTDIRENTS  0     /* 0: Autoselect number of root directory entries */
#define MKFATFS_DEFAULT_RSVDSECCOUNT 0     /* 0: Autoselect number reserved sectors (usually 32) */
#define MKFATFS_DEFAULT_HIDSEC       0     /* No hidden sectors */
#define MKFATFS_DEFAULT_VOLUMEID     0     /* No volume ID */
#define MKFATFS_DEFAULT_NSECTORS     0     /* 0: Use all sectors on device */

#define FAT_FORMAT_INITIALIZER \
{ \
  MKFATFS_DEFAULT_NFATS, \
  MKFATFS_DEFAULT_FATTYPE, \
  MKFATFS_DEFAULT_CLUSTSHIFT, \
  MKFATFS_DEFAULT_VOLUMELABEL, \
  MKFATFS_DEFAULT_BKUPBOOT, \
  MKFATFS_DEFAULT_ROOTDIRENTS, \
  MKFATFS_DEFAULT_RSVDSECCOUNT, \
  MKFATFS_DEFAULT_HIDSEC, \
  MKFATFS_DEFAULT_VOLUMEID, \
  MKFATFS_DEFAULT_NSECTORS \
}

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* These are input parameters for the format.  On return, these values may be
 * overwritten with actual values used in the format.
 */

struct fat_format_s
{
  uint8_t  ff_nfats;           /* Number of FATs */
  uint8_t  ff_fattype;         /* FAT size: 0 (autoselect), 12, 16, or 32 */
  uint8_t  ff_clustshift;      /* Log2 of sectors per cluster: 0-5, 0xff (autoselect) */
  uint8_t  ff_volumelabel[11]; /* Volume label */
  uint16_t ff_backupboot;      /* Sector number of the backup boot sector (0=use default) */
  uint16_t ff_rootdirentries;  /* Number of root directory entries */
  uint16_t ff_rsvdseccount;    /* Reserved sectors */
  uint32_t ff_hidsec;          /* Count of hidden sectors preceding fat */
  uint32_t ff_volumeid;        /* FAT volume id */
  uint32_t ff_nsectors;        /* Number of sectors from device to use: 0: Use all */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

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
 *   Zero (OK) on success;
 *   -1 (ERROR) on failure with errno set appropriately:
 *
 *   EINVAL - NULL block driver string, bad number of FATS in 'fmt', bad FAT
 *     size in 'fmt', bad cluster size in 'fmt'
 *   ENOENT - 'pathname' does not refer to anything in the filesystem.
 *   ENOTBLK - 'pathname' does not refer to a block driver
 *   EACCESS - block driver does not support write or geometry methods
 *
 * Assumptions:
 *   - The caller must assure that the block driver is not mounted and not in
 *     use when this function is called.  The result of formatting a mounted
 *     device is indeterminate (but likely not good).
 *
 ****************************************************************************/

int mkfatfs(FAR const char *pathname, FAR struct fat_format_s *fmt);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_INCLUDE_FSUTILS_MKFATFS_H */
