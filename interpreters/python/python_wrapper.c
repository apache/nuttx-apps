/****************************************************************************
 * apps/interpreters/python/python_wrapper.c
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

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/boardctl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/drivers/ramdisk.h>

#include "romfs_cpython_modules.h"

#include "Python.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration settings */

#ifndef CONFIG_CPYTHON_ROMFS_RAMDEVNO
#  define CONFIG_CPYTHON_ROMFS_RAMDEVNO 1
#endif

#ifndef CONFIG_CPYTHON_ROMFS_SECTORSIZE
#  define CONFIG_CPYTHON_ROMFS_SECTORSIZE 64
#endif

#ifndef CONFIG_CPYTHON_ROMFS_MOUNTPOINT
#  define CONFIG_CPYTHON_ROMFS_MOUNTPOINT "/usr/local/lib"
#endif

#ifdef CONFIG_DISABLE_MOUNTPOINT
#  error "Mountpoint support is disabled"
#endif

#ifndef CONFIG_FS_ROMFS
#  error "ROMFS support not enabled"
#endif

#define NSECTORS(b)        (((b)+CONFIG_CPYTHON_ROMFS_SECTORSIZE-1)/CONFIG_CPYTHON_ROMFS_SECTORSIZE)
#define STR_RAMDEVNO(m)    #m
#define MKMOUNT_DEVNAME(m) "/dev/ram" STR_RAMDEVNO(m)
#define MOUNT_DEVNAME      MKMOUNT_DEVNAME(CONFIG_CPYTHON_ROMFS_RAMDEVNO)

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
 * Name: check_and_mount_romfs
 *
 * Description:
 *   Check if the ROMFS is already mounted, and if not, mount it.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   0 on success, 1 on failure
 *
 ****************************************************************************/

static int check_and_mount_romfs(void)
{
  int ret = OK;
  struct boardioc_romdisk_s desc;
  FILE *fp;
  char line[256];
  int is_mounted = 0;

  /* Check if the device is already mounted */

  fp = fopen("/proc/fs/mount", "r");
  if (fp == NULL)
    {
      printf("ERROR: Failed to open /proc/fs/mount\n");
      UNUSED(desc);
      return ret = ERROR;
    }

  while (fgets(line, sizeof(line), fp))
    {
      if (strstr(line, CONFIG_CPYTHON_ROMFS_MOUNTPOINT) != NULL)
        {
          is_mounted = 1;
          break;
        }
    }

  fclose(fp);

  if (is_mounted)
    {
      _info("Device is already mounted at %s\n",
            CONFIG_CPYTHON_ROMFS_MOUNTPOINT);
      UNUSED(desc);
      return ret;
    }

  /* Create a RAM disk */

  desc.minor    = CONFIG_CPYTHON_ROMFS_RAMDEVNO;            /* Minor device number of the ROM disk. */
  desc.nsectors = NSECTORS(romfs_cpython_modules_img_len);  /* The number of sectors in the ROM disk */
  desc.sectsize = CONFIG_CPYTHON_ROMFS_SECTORSIZE;          /* The size of one sector in bytes */
  desc.image    = (FAR uint8_t *)romfs_cpython_modules_img; /* File system image */

  ret = boardctl(BOARDIOC_ROMDISK, (uintptr_t)&desc);

  if (ret < 0)
    {
      printf("ERROR: Failed to create RAM disk: %s\n", strerror(errno));
      return 1;
    }

  /* Mount the test file system */

  _info("Mounting ROMFS filesystem at target=%s with source=%s\n",
        CONFIG_CPYTHON_ROMFS_MOUNTPOINT, MOUNT_DEVNAME);

  ret = mount(MOUNT_DEVNAME, CONFIG_CPYTHON_ROMFS_MOUNTPOINT, "romfs",
              MS_RDONLY, NULL);
  if (ret < 0)
    {
      printf("ERROR: Mount failed: %s\n", strerror(errno));
      return 1;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: python_wrapper_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;

  ret = check_and_mount_romfs();
  if (ret != 0)
    {
      return ret;
    }

  _pyruntime_early_init();

  setenv("PYTHONHOME", "/usr/local", 1);

  setenv("PYTHON_BASIC_REPL", "1", 1);

  return py_bytesmain(argc, argv);
}
