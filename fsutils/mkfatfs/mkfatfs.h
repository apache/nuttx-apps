/****************************************************************************
 * apps/fsutils/mkfatfs/mkfatfs.h
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

#ifndef __APPS_FSUTILS_MKFATFS_MKFATFS_H
#define __APPS_FSUTILS_MKFATFS_MKFATFS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Only the "hard drive" media type is used */

#define FAT_DEFAULT_MEDIA_TYPE         0xf8

/*  Default hard driver geometry */

#define FAT_DEFAULT_SECPERTRK          63
#define FAT_DEFAULT_NUMHEADS           255

/* FSINFO is always at this sector */

#define FAT_DEFAULT_FSINFO_SECTOR      1

/* FAT32 foot cluster number */

#define FAT32_DEFAULT_ROOT_CLUSTER     2

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure (plus the user-provided struct fat_format_s) describes
 * the format FAT file system.  All "global" variables used in the format
 * logic are contained in this structure so that is possible to format two
 * block devices concurrently.
 */

struct fat_var_s
{
  int            fv_fd;             /* File descriptor of open block driver */
  uint8_t        fv_jump[3];        /* 3-byte boot jump instruction */
  uint8_t        fv_sectshift;      /* Log2 of fv_sectorsize */
  uint8_t        fv_nrootdirsects;  /* Number of root directory sectors */
  uint8_t        fv_fattype;        /* FAT size: 0 (not determined), 12, 16, or 32 */
  uint16_t       fv_bootcodesize;   /* Size of array at fv_bootcode */
  uint32_t       fv_createtime;     /* Creation time */
  uint32_t       fv_sectorsize;     /* Size of one hardware sector */
  uint32_t       fv_nfatsects;      /* Number of sectors in each FAT */
  uint32_t       fv_nclusters;      /* Number of clusters */
  uint8_t       *fv_sect;           /* Allocated working sector buffer */
  uint8_t        fv_bootcodepatch;  /* FAT16/FAT32 Bootcode offset patch */
  const uint8_t *fv_bootcodeblob;   /* Points to boot code to put into MBR */
};

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

struct fat_format_s; /* Forward reference */

/****************************************************************************
 * Name: mkfatfs_configfatfs
 *
 * Description:
 *   Based on the geometry of the block device and upon the caller-selected
 *   values, configure the FAT filesystem for the device.
 *
 * Input:
 *    fmt  - Caller specified format parameters
 *    var  - Holds disk geometry data.  Also, the location to return FAT
 *           configuration data
 *
 * Return:
 *    Zero on success; negated errno on failure
 *
 ****************************************************************************/

int mkfatfs_configfatfs(FAR struct fat_format_s *fmt,
                        FAR struct fat_var_s *var);

/****************************************************************************
 * Name: mkfatfs_writefat
 *
 * Description:
 *   Write the configured fat filesystem to the block device
 *
 * Input:
 *    fmt  - Caller specified format parameters
 *    var  - Other format parameters that are not caller specifiable. (Most
 *           set by mkfatfs_configfatfs()).
 *
 * Return:
 *    Zero on success; negated errno on failure
 *
 ****************************************************************************/

int mkfatfs_writefatfs(FAR struct fat_format_s *fmt,
                       FAR struct fat_var_s *var);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_FSUTILS_MKFATFS_MKFATFS_H */
