/****************************************************************************
 * apps/graphics/traveler/trv_romfs.c
 * Mount the ROMFS demo world
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_GRAPHICS_TRAVELER_ROMFSDEMO

#include <sys/mount.h>
#include <sys/types.h>
#include <sys/boardctl.h>
#include <stdio.h>

#include <nuttx/drivers/ramdisk.h>

#include "trv_romfs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_FS_ROMFS
#  error You must select CONFIG_FS_ROMFS in your configuration file
#endif

#ifdef CONFIG_DISABLE_MOUNTPOINT
#  error You must not disable mountpoints via CONFIG_DISABLE_MOUNTPOINT
#endif

#ifndef CONFIG_BOARDCTL_ROMDISK
#  error You must select CONFIG_BOARDCTL_ROMDISK in your configuration file
#endif

/* Describe the ROMFS file system */

#define SECTORSIZE   512
#define NSECTORS(b)  (((b)+SECTORSIZE-1)/SECTORSIZE)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_mount_world
 *
 * Description:
 *   Mount the demo world as a ROMFS file system mounted at 'mountpoint'
 *
 ****************************************************************************/

int trv_mount_world(int minor, FAR const char *mountpoint)
{
  struct boardioc_romdisk_s desc;
  char devpath[16];
  int ret;

  /* Create a ROM disk for the ROMFS filesystem */

  desc.minor    = minor;                       /* Minor device number of the RAM disk. */
  desc.nsectors = NSECTORS(trv_romfs_img_len); /* The number of sectors in the RAM disk */
  desc.sectsize = SECTORSIZE;                  /* The size of one sector in bytes */
  desc.image    = trv_romfs_img;               /* File system image */

  ret = boardctl(BOARDIOC_ROMDISK, (uintptr_t)&desc);
  if (ret < 0)
    {
      return ret;
    }

  /* Get the ROM disk device name */

  snprintf(devpath, 16, "/dev/ram%d", minor);

  /* Mount the file system */

  ret = mount(devpath, mountpoint, "romfs", MS_RDONLY, NULL);
  return ret;
}

#endif /* CONFIG_GRAPHICS_TRAVELER_ROMFSDEMO */
