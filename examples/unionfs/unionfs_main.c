/****************************************************************************
 * apps/examples/unionfs/unionfs_main.c
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#include <nuttx/drivers/ramdisk.h>
#include <nuttx/fs/unionfs.h>

#include "romfs_atestdir.h"
#include "romfs_btestdir.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration settings */

#ifdef CONFIG_DISABLE_MOUNTPOINT
#  error "Mountpoint support is disabled"
#endif

#ifndef CONFIG_FS_ROMFS
#  error "ROMFS support not enabled"
#endif

#ifndef CONFIG_FS_UNIONFS
#  error "Union File System support not enabled"
#endif

#ifndef CONFIG_EXAMPLES_UNIONFS_RAMDEVNO_A
#  define CONFIG_EXAMPLES_UNIONFS_RAMDEVNO_A 1
#endif

#ifndef CONFIG_EXAMPLES_UNIONFS_RAMDEVNO_B
#  define CONFIG_EXAMPLES_UNIONFS_RAMDEVNO_B 2
#endif

#ifndef CONFIG_EXAMPLES_UNIONFS_SECTORSIZE
#  define CONFIG_EXAMPLES_UNIONFS_SECTORSIZE 64
#endif

#ifndef CONFIG_EXAMPLES_UNIONFS_MOUNTPT
#  define "/mnt/unionfs"
#endif

#ifndef CONFIG_EXAMPLES_UNIONFS_TMPA
#  define "/mnt/a"
#endif

#ifndef CONFIG_EXAMPLES_UNIONFS_TMPB
#  define "/mnt/b"
#endif

#define NSECTORS(b)        (((b)+CONFIG_EXAMPLES_UNIONFS_SECTORSIZE-1)/CONFIG_EXAMPLES_UNIONFS_SECTORSIZE)
#define STR_RAMDEVNO(m)    #m
#define MKMOUNT_DEVNAME(m) "/dev/ram" STR_RAMDEVNO(m)
#define MOUNT_DEVNAME_A    MKMOUNT_DEVNAME(CONFIG_EXAMPLES_UNIONFS_RAMDEVNO_A)
#define MOUNT_DEVNAME_B    MKMOUNT_DEVNAME(CONFIG_EXAMPLES_UNIONFS_RAMDEVNO_B)

#define SCRATCHBUFFER_SIZE 1024

/* Test directory stuff */

#define WRITABLE_MODE      (S_IWOTH|S_IWGRP|S_IWUSR)
#define READABLE_MODE      (S_IROTH|S_IRGRP|S_IRUSR)
#define EXECUTABLE_MODE    (S_IXOTH|S_IXGRP|S_IXUSR)

#define DIRECTORY_MODE     (S_IFDIR|READABLE_MODE|EXECUTABLE_MODE)
#define FILE_MODE          (S_IFREG|READABLE_MODE)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: unionfs_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  struct boardioc_romdisk_s desc;

  /* Create a RAM disk for file system 1 */

  desc.minor    = CONFIG_EXAMPLES_UNIONFS_RAMDEVNO_A;       /* Minor device number of the ROM disk. */
  desc.nsectors = NSECTORS(atestdir_img_len);               /* The number of sectors in the ROM disk */
  desc.sectsize = CONFIG_EXAMPLES_UNIONFS_SECTORSIZE;       /* The size of one sector in bytes */
  desc.image    = (FAR uint8_t *)atestdir_img;              /* File system image */

  ret = boardctl(BOARDIOC_ROMDISK, (uintptr_t)&desc);

  if (ret < 0)
    {
      printf("ERROR: Failed to create file system 1 RAM disk: %s\n",
             strerror(errno));
      return EXIT_FAILURE;
    }

  /* Mount test file system 1 */

  printf("Mounting ROMFS file system 1 at target=%s with source=%s\n",
         CONFIG_EXAMPLES_UNIONFS_TMPA, MOUNT_DEVNAME_A);

  ret = mount(MOUNT_DEVNAME_A, CONFIG_EXAMPLES_UNIONFS_TMPA, "romfs",
              MS_RDONLY, NULL);
  if (ret < 0)
    {
      printf("ERROR: File system 1 mount failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
    }

  /* Create a RAM disk for file system 2  */

  desc.minor    = CONFIG_EXAMPLES_UNIONFS_RAMDEVNO_B;      /* Minor device number of the ROM disk. */
  desc.nsectors = NSECTORS(btestdir_img_len);              /* The number of sectors in the ROM disk */
  desc.image    = (FAR uint8_t *)btestdir_img;             /* File system image */

  ret = boardctl(BOARDIOC_ROMDISK, (uintptr_t)&desc);

  if (ret < 0)
    {
      printf("ERROR: Failed to create file system 2 RAM disk: %s\n",
             strerror(errno));
      return EXIT_FAILURE;
    }

  /* Mount test file system 2 */

  printf("Mounting ROMFS file system 2 at target=%s with source=%s\n",
         CONFIG_EXAMPLES_UNIONFS_TMPB, MOUNT_DEVNAME_B);

  ret = mount(MOUNT_DEVNAME_B, CONFIG_EXAMPLES_UNIONFS_TMPB, "romfs",
              MS_RDONLY, NULL);
  if (ret < 0)
    {
      printf("ERROR: File system 2 mount failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
    }

  /* Now create and mount the union file system */

  ret = unionfs_mount(CONFIG_EXAMPLES_UNIONFS_TMPA, NULL,
                      CONFIG_EXAMPLES_UNIONFS_TMPB, "offset",
                      CONFIG_EXAMPLES_UNIONFS_MOUNTPT);
  if (ret < 0)
    {
      printf("ERROR: Failed to create the union file system: %d\n", ret);
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
