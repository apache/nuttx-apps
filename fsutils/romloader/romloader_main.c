/****************************************************************************
 * apps/fsutils/romloader/romloader_main.c
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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#ifdef CONFIG_FSUTILS_ROMLOADER_ROMFS
#  include <sys/boardctl.h>
#  include <nuttx/drivers/ramdisk.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_FSUTILS_ROMLOADER_ROMFS
#  define NSECTORS(b)  (((b) + CONFIG_FSUTILS_ROMLOADER_SECTORSIZE - 1) / \
                        CONFIG_FSUTILS_ROMLOADER_SECTORSIZE)
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Symbols from Auto-Generated Code
 ****************************************************************************/

#ifdef CONFIG_FSUTILS_ROMLOADER_ROMFS
extern const unsigned char romfs_img[];
extern const unsigned int romfs_img_len;
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * romloader_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
#ifdef CONFIG_FSUTILS_ROMLOADER_ROMFS
  struct boardioc_romdisk_s desc;

  /* Create a ROM disk for the ROMFS filesystem */

  printf("Registering romdisk at /dev/ram%d\n",
         CONFIG_FSUTILS_ROMLOADER_DEVMINOR);

  desc.minor    = CONFIG_FSUTILS_ROMLOADER_DEVMINOR;   /* Minor device number of the ROM disk. */
  desc.nsectors = NSECTORS(romfs_img_len);             /* The number of sectors in the ROM disk */
  desc.sectsize = CONFIG_FSUTILS_ROMLOADER_SECTORSIZE; /* The size of one sector in bytes */
  desc.image    = (FAR uint8_t *)romfs_img;            /* File system image */

  ret = boardctl(BOARDIOC_ROMDISK, (uintptr_t)&desc);
  if (ret < 0)
    {
      dprintf(STDERR_FILENO, "ERROR: romdisk_register failed: %s\n",
              strerror(errno));
      return 1;
    }

  /* Mount the ROMFS file system */

  printf("Mounting ROMFS filesystem at target=%s with source=%s\n",
         CONFIG_FSUTILS_ROMLOADER_MOUNTPOINT,
         CONFIG_FSUTILS_ROMLOADER_DEVPATH);

  ret = mount(CONFIG_FSUTILS_ROMLOADER_DEVPATH,
              CONFIG_FSUTILS_ROMLOADER_MOUNTPOINT,
              "romfs",
              MS_RDONLY,
              NULL);

  if (ret < 0)
    {
      dprintf(STDERR_FILENO,
              "ERROR: mount(%s,%s,romfs) failed: %s\n",
              CONFIG_FSUTILS_ROMLOADER_DEVPATH,
              CONFIG_FSUTILS_ROMLOADER_MOUNTPOINT,
              strerror(errno));

      return 1;
    }
#else
  /* Mount the CROMFS file system */

  printf("Mounting CROMFS filesystem at target=%s\n",
         CONFIG_FSUTILS_ROMLOADER_MOUNTPOINT);

  ret = mount(NULL,
              CONFIG_FSUTILS_ROMLOADER_MOUNTPOINT,
              "cromfs",
              MS_RDONLY,
              NULL);

  if (ret < 0)
    {
      dprintf(STDERR_FILENO,
              "ERROR: mount(%s,cromfs) failed: %s\n",
              CONFIG_FSUTILS_ROMLOADER_MOUNTPOINT,
              strerror(errno));

      return 1;
    }
#endif

  return 0;
}
