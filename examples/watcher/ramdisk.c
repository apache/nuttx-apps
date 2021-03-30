/****************************************************************************
 * examples/watcher/ramdisk.c
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

#include <sys/boardctl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h>
#include <nuttx/drivers/ramdisk.h>

#include "fsutils/mkfatfs.h"
#include "ramdisk.h"

/****************************************************************************
 * Private Definitions
 ****************************************************************************/

#define BUFFER_SIZE \
  (NSECTORS * SECTORSIZE)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct fat_format_s g_fmt = FAT_FORMAT_INITIALIZER;

/****************************************************************************
 * Public Data
 ****************************************************************************/

static const char g_source[] = MOUNT_DEVNAME;
static const char g_filesystemtype[] = "vfat";
static const char g_target[] = "/mnt/watcher";

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: create_ramdisk
 *
 * Description:
 *   Create a RAM disk of the specified size formatting with a FAT file
 *   system
 *
 * Input Parameters:
 *   None
 *
 * Return:
 *   Zero on success, a negated errno on failure.
 *
 ****************************************************************************/

int create_ramdisk(void)
{
  struct boardioc_mkrd_s desc;
  int ret;

  /* Create a RAMDISK device to manage */

  desc.minor = MINOR;                                   /* Minor device number of the RAM disk. */
  desc.nsectors = NSECTORS;                             /* The number of sectors in the RAM disk. */
  desc.sectsize = SECTORSIZE;                           /* The size of one sector in bytes. */
  desc.rdflags = RDFLAG_WRENABLED | RDFLAG_FUNLINK;     /* See ramdisk.h. */

  ret = boardctl(BOARDIOC_MKRD, (uintptr_t) & desc);
  if (ret < 0)
    {
      printf("create_ramdisk: Failed to create ramdisk at %s: %d\n",
             g_source, -ret);
      return ret;
    }

  /* Create a FAT filesystem on the ramdisk */

  ret = mkfatfs(g_source, &g_fmt);
  if (ret < 0)
    {
      printf("mkfatfs: Failed to create FAT filesystem on ramdisk at %s\n",
             g_source);
      return ret;
    }

  return 0;
}

int prepare_fs(void)
{
  int ret;
  ret = create_ramdisk();
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr,
              "create_ramdisk: failed on creating RAM disk \
              and formatting it: %d\n",
              errcode);
      ret = errcode;
      goto errout;
    }

  ret = mount(g_source, g_target, g_filesystemtype, 0, NULL);
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "mount: Failed to mount FS: %d\n", errcode);
      ret = errcode;
      goto errout;
    }

errout:
  return ret;
}
