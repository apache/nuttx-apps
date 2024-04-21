/****************************************************************************
 * apps/uploadfiles/upload_main.c
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

#include <sys/mount.h>
#include <sys/boardctl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <nuttx/drivers/ramdisk.h>

#include "romfs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Check configuration.  This is not all of the configuration settings that
 * are required -- only the more obvious.
 */

#ifndef CONFIG_FS_ROMFS
#  error "You must select CONFIG_FS_ROMFS in menuconfig"
#endif

#ifdef CONFIG_DISABLE_MOUNTPOINT
#  error "You must not disable mountpoints via CONFIG_DISABLE_MOUNTPOINT menuconfig"
#endif

#ifndef CONFIG_BOARDCTL_ROMDISK
#  error "You must to enable CONFIG_BOARDCTL_ROMDISK in menuconfig"
#endif

/* Describe the ROMFS file system */

#define SECTORSIZE   512
#define NSECTORS(b)  (((b)+SECTORSIZE-1)/SECTORSIZE)
#define MOUNTPT      "/data"

#ifndef CONFIG_UPLOADFILES_DEVMINOR
#  define CONFIG_UPLOADFILES_DEVMINOR 0
#endif

#ifndef CONFIG_UPLOADFILES_DEVPATH
#  define CONFIG_UPLOADFILES_DEVPATH "/dev/ram0"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * bastest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  struct boardioc_romdisk_s desc;

  /* Create a ROM disk for the ROMFS filesystem */

  printf("Registering romdisk at /dev/ram%d\n",
          CONFIG_UPLOADFILES_DEVMINOR);

  desc.minor    = CONFIG_UPLOADFILES_DEVMINOR;     /* Minor device number of the ROM disk. */
  desc.nsectors = NSECTORS(romfs_img_len);         /* The number of sectors in the ROM disk */
  desc.sectsize = SECTORSIZE;                      /* The size of one sector in bytes */
  desc.image    = (FAR uint8_t *)romfs_img;        /* File system image */

  ret = boardctl(BOARDIOC_ROMDISK, (uintptr_t)&desc);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: romdisk_register failed: %s\n",
              strerror(errno));
      return 1;
    }

  /* Mount the file system */

  printf("Mounting ROMFS filesystem at target=%s with source=%s\n",
         MOUNTPT, CONFIG_UPLOADFILES_DEVPATH);

  ret = mount(CONFIG_UPLOADFILES_DEVPATH, MOUNTPT, "romfs",
              MS_RDONLY, NULL);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: mount(%s,%s,romfs) failed: %s\n",
              CONFIG_UPLOADFILES_DEVPATH, MOUNTPT, strerror(errno));
      return 1;
    }

  return 0;
}
