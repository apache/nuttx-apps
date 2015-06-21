/****************************************************************************
 * examples/unionfs/unionfs_main.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
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

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#include <nuttx/fs/ramdisk.h>
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

#if CONFIG_NFILE_DESCRIPTORS < 4
#  error "Not enough file descriptors"
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

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int unionfs_main(int argc, char *argv[])
#endif
{
   int  ret;

  /* Create a RAM disk for file system 1 */

  ret = romdisk_register(CONFIG_EXAMPLES_UNIONFS_RAMDEVNO_A, atestdir_img,
                         NSECTORS(atestdir_img_len),
                         CONFIG_EXAMPLES_UNIONFS_SECTORSIZE);
  if (ret < 0)
    {
      printf("ERROR: Failed to create file system 1 RAM disk\n");
      return EXIT_FAILURE;
    }

  /* Mount test file system 1 */

  printf("Mounting ROMFS file system 1 at target=%s with source=%s\n",
         CONFIG_EXAMPLES_UNIONFS_TMPA, MOUNT_DEVNAME_A);

  ret = mount(MOUNT_DEVNAME_A, CONFIG_EXAMPLES_UNIONFS_TMPA, "romfs",
              MS_RDONLY, NULL);
  if (ret < 0)
    {
      printf("ERROR: File system 1 mount failed: %d\n", errno);
      return EXIT_FAILURE;
    }

  /* Create a RAM disk for file system 2 */

  ret = romdisk_register(CONFIG_EXAMPLES_UNIONFS_RAMDEVNO_B, btestdir_img,
                         NSECTORS(btestdir_img_len),
                         CONFIG_EXAMPLES_UNIONFS_SECTORSIZE);
  if (ret < 0)
    {
      printf("ERROR: Failed to register file system 1: %d\n", ret);
      return EXIT_FAILURE;
    }

  /* Mount test file system 2 */

  printf("Mounting ROMFS file system 2 at target=%s with source=%s\n",
         CONFIG_EXAMPLES_UNIONFS_TMPB, MOUNT_DEVNAME_B);

  ret = mount(MOUNT_DEVNAME_B, CONFIG_EXAMPLES_UNIONFS_TMPB, "romfs",
              MS_RDONLY, NULL);
  if (ret < 0)
    {
      printf("ERROR: Failed to register file system 1: %d\n", ret);
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
