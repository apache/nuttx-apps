/****************************************************************************
 * examples/bastest/bastest_main.c
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

#include <sys/mount.h>
#include <stdio.h>
#include <errno.h>

#include <nuttx/drivers/ramdisk.h>

#include "romfs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Check configuration.  This is not all of the configuration settings that
 * are required -- only the more obvious.
 */

#if CONFIG_NFILE_DESCRIPTORS < 1
#  error "You must provide file descriptors via CONFIG_NFILE_DESCRIPTORS in your configuration file"
#endif

#ifndef CONFIG_FS_ROMFS
#  error "You must select CONFIG_FS_ROMFS in your configuration file"
#endif

#ifdef CONFIG_DISABLE_MOUNTPOINT
#  error "You must not disable mountpoints via CONFIG_DISABLE_MOUNTPOINT in your configuration file"
#endif

/* Describe the ROMFS file system */

#define SECTORSIZE   512
#define NSECTORS(b)  (((b)+SECTORSIZE-1)/SECTORSIZE)
#define MOUNTPT      "/mnt/romfs"

#ifndef CONFIG_EXAMPLES_BASTEST_DEVMINOR
#  define CONFIG_EXAMPLES_BASTEST_DEVMINOR 0
#endif

#ifndef CONFIG_EXAMPLES_BASTEST_DEVPATH
#  define CONFIG_EXAMPLES_BASTEST_DEVPATH "/dev/ram0"
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

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int bastest_main(int argc, char *argv[])
#endif
{
  int ret;

  /* Create a ROM disk for the ROMFS filesystem */

  printf("Registering romdisk at /dev/ram%d\n", CONFIG_EXAMPLES_BASTEST_DEVMINOR);
  ret = romdisk_register(CONFIG_EXAMPLES_BASTEST_DEVMINOR, (FAR uint8_t *)romfs_img,
                         NSECTORS(romfs_img_len), SECTORSIZE);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: romdisk_register failed: %d\n", ret);
      return 1;
    }

  /* Mount the file system */

  printf("Mounting ROMFS filesystem at target=%s with source=%s\n",
         MOUNTPT, CONFIG_EXAMPLES_BASTEST_DEVPATH);

  ret = mount(CONFIG_EXAMPLES_BASTEST_DEVPATH, MOUNTPT, "romfs", MS_RDONLY, NULL);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: mount(%s,%s,romfs) failed: %s\n",
              CONFIG_EXAMPLES_BASTEST_DEVPATH, MOUNTPT, errno);
      return 1;
    }

  return 0;
}
