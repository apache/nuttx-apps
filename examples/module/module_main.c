/****************************************************************************
 * examples/module/module_main.c
 *
 *   Copyright (C) 2015, 2017 Gregory Nutt. All rights reserved.
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
#include <nuttx/compiler.h>

#ifdef CONFIG_EXAMPLES_MODULE_BUILTINFS
#  include <sys/mount.h>
#endif

#include <sys/boardctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/module.h>
#include <nuttx/binfmt/symtab.h>

#ifdef CONFIG_EXAMPLES_MODULE_BUILTINFS
#  include <nuttx/drivers/ramdisk.h>
#  include "drivers/romfs.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Check configuration.  This is not all of the configuration settings that
 * are required -- only the more obvious.
 */

#if CONFIG_NFILE_DESCRIPTORS < 1
#  error "You must provide file descriptors via CONFIG_NFILE_DESCRIPTORS in your configuration file"
#endif

#ifndef CONFIG_MODULE
#  error "You must select CONFIG_MODULE in your configuration file"
#endif

#ifdef CONFIG_EXAMPLES_MODULE_BUILTINFS
#  ifndef CONFIG_FS_ROMFS
#    error "You must select CONFIG_FS_ROMFS in your configuration file"
#  endif

#  ifdef CONFIG_DISABLE_MOUNTPOINT
#    error "You must not disable mountpoints via CONFIG_DISABLE_MOUNTPOINT in your configuration file"
#  endif

  /* Describe the ROMFS file system */

#  define SECTORSIZE   64
#  define NSECTORS(b)  (((b)+SECTORSIZE-1)/SECTORSIZE)
#  define BINDIR       "/mnt/romfs"

#  ifndef CONFIG_EXAMPLES_MODULE_DEVMINOR
#    define CONFIG_EXAMPLES_MODULE_DEVMINOR 0
#  endif

#  ifndef CONFIG_EXAMPLES_MODULE_DEVPATH
#    define CONFIG_EXAMPLES_MODULE_DEVPATH "/dev/ram0"
#  endif
#else
#  define BINDIR       CONFIG_EXAMPLES_MODULE_BINDIR
#endif /* CONFIG_EXAMPLES_MODULE_BUILTINFS */

/****************************************************************************
 * Private data
 ****************************************************************************/

static const char g_write_string[] = "Hi there, installed driver\n";

/****************************************************************************
 * Symbols from Auto-Generated Code
 ****************************************************************************/

extern const struct symtab_s mod_exports[];
extern const int mod_nexports;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: module_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int module_main(int argc, char *argv[])
#endif
{
  struct boardioc_symtab_s symdesc;
  FAR void *handle;
  char buffer[128];
  ssize_t nbytes;
  int ret;
  int fd;

  /* Set the OS symbol table indirectly through the boardctl() */

  symdesc.symtab   = (FAR struct symtab_s *)mod_exports;
  symdesc.nsymbols = mod_nexports;
  ret = boardctl(BOARDIOC_OS_SYMTAB, (uintptr_t)&symdesc);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: boardctl(BOARDIOC_OS_SYMTAB) failed: %d\n", ret);
      exit(EXIT_FAILURE);
    }

#ifdef CONFIG_EXAMPLES_MODULE_BUILTINFS
  /* Create a ROM disk for the ROMFS filesystem */

  printf("main: Registering romdisk at /dev/ram%d\n",
         CONFIG_EXAMPLES_MODULE_DEVMINOR);

  ret = romdisk_register(CONFIG_EXAMPLES_MODULE_DEVMINOR, (FAR uint8_t *)romfs_img,
                         NSECTORS(romfs_img_len), SECTORSIZE);
  if (ret < 0)
    {
      /* This will happen naturally if we registered the ROM disk previously. */

      if (ret != -EEXIST)
        {
          fprintf(stderr, "ERROR: romdisk_register failed: %d\n", ret);
          exit(EXIT_FAILURE);
        }

      printf("main: ROM disk already registered\n");
    }

  /* Mount the file system */

  printf("main: Mounting ROMFS filesystem at target=%s with source=%s\n",
         BINDIR, CONFIG_EXAMPLES_MODULE_DEVPATH);

  ret = mount(CONFIG_EXAMPLES_MODULE_DEVPATH, BINDIR, "romfs", MS_RDONLY, NULL);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: mount(%s,%s,romfs) failed: %s\n",
              CONFIG_EXAMPLES_MODULE_DEVPATH, BINDIR, errno);
      exit(EXIT_FAILURE);
    }
#endif /* CONFIG_EXAMPLES_MODULE_BUILTINFS */

  /* Install the character driver  */

  handle = insmod(BINDIR "/chardev", "chardev");
  if (handle == NULL)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: insmod failed: %d\n", errcode);
      exit(EXIT_FAILURE);
    }

  /* Open the installed character driver */

  fd = open("/dev/chardev", O_RDWR);
  if (fd < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open /dev/chardev: %d\n", errcode);
      exit(EXIT_FAILURE);
    }

  /* Read from the character driver */

  nbytes = read(fd, buffer, 128);
  if (nbytes < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Read from /dev/chardev failed: %d\n", errcode);
      close(fd);
      exit(EXIT_FAILURE);
    }

  printf("main: Read %d bytes: %d\n", (int)nbytes);
  lib_dumpbuffer("main: Bytes read", (FAR const uint8_t *)buffer, nbytes);

  /* Write to the character driver */

  nbytes = write(fd, g_write_string, strlen(g_write_string));
  if (nbytes < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Write to /dev/chardev failed: %d\n", errcode);
      close(fd);
      exit(EXIT_FAILURE);
    }

  printf("main: Wrote %d bytes: %d\n", (int)nbytes);
  lib_dumpbuffer("main: Bytes written", (FAR const uint8_t *)g_write_string, nbytes);

  close(fd);
  ret = rmmod(handle);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: rmmod failed: %d\n", ret);
      exit(EXIT_FAILURE);
    }

  return EXIT_SUCCESS;
}
