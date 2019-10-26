/****************************************************************************
 * apps/nshlib/nsh_romfsetc.c
 *
 *   Copyright (C) 2008-2012 Gregory Nutt. All rights reserved.
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
  ret = mount(MOUNT_DEVNAME, CONFIG_NSH_ROMFSMOUNTPT, "cromfs", MS_RDONLY, NULL);
#else
  ret = mount(MOUNT_DEVNAME, CONFIG_NSH_ROMFSMOUNTPT, "romfs", MS_RDONLY, NULL);
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
