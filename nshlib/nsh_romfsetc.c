/****************************************************************************
 * apps/nshlib/nsh_romfsetc.c
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
#include <debug.h>
#include <errno.h>

#include <nuttx/drivers/ramdisk.h>

#include "nsh.h"

#ifdef CONFIG_NSH_ROMFSETC

/* Should we use the default ROMFS image?  Or a custom, board-specific
 * ROMFS image?
 */

#ifdef CONFIG_NSH_ARCHROMFS
#  include <arch/board/nsh_romfsimg.h>
#elif defined(CONFIG_NSH_CUSTOMROMFS)
#  include CONFIG_NSH_CUSTOMROMFS_HEADER
#else /* if defined(CONFIG_NSH_DEFAULTROMFS) */
#  include "nsh_romfsimg.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_DISABLE_MOUNTPOINT
#  error You must not disable mountpoints via CONFIG_DISABLE_MOUNTPOINT
#endif

#if defined(CONFIG_NSH_CROMFSETC)
#  ifndef CONFIG_FS_CROMFS
#    error You must select CONFIG_FS_CROMFS in your configuration file
#  endif
#else
#  ifndef CONFIG_FS_ROMFS
#    error You must select CONFIG_FS_ROMFS in your configuration file
#  endif
#  ifndef CONFIG_BOARDCTL_ROMDISK
#    error You must select CONFIG_BOARDCTL_ROMDISK in your configuration file
#  endif
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_romfsetc
 ****************************************************************************/

int nsh_romfsetc(void)
{
  int  ret;

#ifndef CONFIG_NSH_CROMFSETC
  struct boardioc_romdisk_s desc;

  /* Create a ROM disk for the /etc filesystem */

  desc.minor    = CONFIG_NSH_ROMFSDEVNO;     /* Minor device number of the RAM disk. */
  desc.nsectors = NSECTORS(romfs_img_len);   /* The number of sectors in the RAM disk */
  desc.sectsize = CONFIG_NSH_ROMFSSECTSIZE;  /* The size of one sector in bytes */
  desc.image    = romfs_img;                 /* File system image */

  ret = boardctl(BOARDIOC_ROMDISK, (uintptr_t)&desc);
  if (ret < 0)
    {
      ferr("ERROR: boardctl(BOARDIOC_ROMDISK) failed: %d\n", -ret);
      return ERROR;
    }
#endif

  /* Mount the file system */

  finfo("Mounting ROMFS filesystem at target=%s with source=%s\n",
        CONFIG_NSH_ROMFSMOUNTPT, MOUNT_DEVNAME);

#if defined(CONFIG_NSH_CROMFSETC)
  ret = mount(MOUNT_DEVNAME, CONFIG_NSH_ROMFSMOUNTPT, "cromfs", MS_RDONLY,
              NULL);
#else
  ret = mount(MOUNT_DEVNAME, CONFIG_NSH_ROMFSMOUNTPT, "romfs", MS_RDONLY,
              NULL);
#endif
  if (ret < 0)
    {
      ferr("ERROR: mount(%s,%s,romfs) failed: %d\n",
           MOUNT_DEVNAME, CONFIG_NSH_ROMFSMOUNTPT, errno);
      return ERROR;
    }

  return OK;
}

#endif /* CONFIG_NSH_ROMFSETC */
