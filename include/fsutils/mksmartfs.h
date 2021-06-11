/****************************************************************************
 * apps/include/fsutils/mksmartfs.h
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

#ifndef __APPS_INCLUDE_FSUTILS_MKSMARTFS_H
#define __APPS_INCLUDE_FSUTILS_MKSMARTFS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

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
 *   Zero (OK) on success;
 *   -1 (ERROR) on failure with errno set appropriately:
 *
 *   EINVAL - NULL block driver string
 *   ENOENT - 'pathname' does not refer to anything in the filesystem.
 *   ENOTBLK - 'pathname' does not refer to a block driver
 *   EFTYPE - the block driver hasn't been formatted yet
 *
 ****************************************************************************/

int issmartfs(FAR const char *pathname);

/****************************************************************************
 * Name: mksmartfs
 *
 * Description:
 *   Make a SMART (Sector Mapped Allocation for Really Tiny) Flash file
 *   system image on the specified block device (must be a SMART device).
 *
 * Inputs:
 *   pathname - the full path to a registered block driver
 *   nrootdirs - the number of Root Directory entries to support
 *               on this device (supports multiple mount points).
 *
 * Return:
 *   Zero (OK) on success;
 *   -1 (ERROR) on failure with errno set appropriately:
 *
 *   EINVAL - NULL block driver string
 *   ENOENT - 'pathname' does not refer to anything in the filesystem.
 *   ENOTBLK - 'pathname' does not refer to a block driver
 *   EACCESS - block driver does not support write or geometry methods or
 *             is not a SMART device
 *
 * Assumptions:
 *   - The caller must assure that the block driver is not mounted and not in
 *     use when this function is called.  The result of formatting a mounted
 *     device is indeterminate (but likely not good).
 *
 ****************************************************************************/

#ifdef CONFIG_SMARTFS_MULTI_ROOT_DIRS
int mksmartfs(FAR const char *pathname, uint16_t sectorsize,
              uint8_t nrootdirs);
#else
int mksmartfs(FAR const char *pathname, uint16_t sectorsize);
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_INCLUDE_FSUTILS_MKSMARTFS_H */
