/****************************************************************************
 * apps/boot/nxboot/loader/flash.h
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

#ifndef __BOOT_NXBOOT_LOADER_FLASH_H
#define __BOOT_NXBOOT_LOADER_FLASH_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stddef.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct flash_partition_info
{
  int size;
  int blocksize;
  int neraseblocks;
  int erasesize;
};

/****************************************************************************
 * Public Functions Prototypes
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

int flash_partition_open(const char *path);

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

int flash_partition_close(int fd);

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

int flash_partition_write(int fd, const void *buf, size_t count, off_t off);

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

int flash_partition_read(int fd, void *buf, size_t count, off_t off);

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

int flash_partition_erase(int fd);

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

int flash_partition_erase_first_sector(int fd);

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

int flash_partition_info(int fd, struct flash_partition_info *info);

#endif /* __BOOT_NXBOOT_LOADER_FLASH_H */
