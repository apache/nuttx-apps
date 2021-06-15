/****************************************************************************
 * apps/examples/module/module_main.c
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
#include <nuttx/compiler.h>

#include <sys/mount.h>
#include <sys/stat.h>

#include <sys/boardctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/module.h>
#include <nuttx/symtab.h>

#if  defined(CONFIG_EXAMPLES_MODULE_ROMFS)
#  include <nuttx/drivers/ramdisk.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Check configuration.  This is not all of the configuration settings that
 * are required -- only the more obvious.
 */

#ifndef CONFIG_MODULE
#  error "You must select CONFIG_MODULE in your configuration file"
#endif

#if defined(CONFIG_EXAMPLES_MODULE_BUILTINFS)
#  if !defined(CONFIG_FS_ROMFS) && !defined(CONFIG_FS_CROMFS)
#    error "You must select CONFIG_FS_ROMFS or CONFIG_FS_CROMFS in your configuration file"
#  endif

#  ifdef CONFIG_DISABLE_MOUNTPOINT
#    error "You must not disable mountpoints via CONFIG_DISABLE_MOUNTPOINT in your configuration file"
#  endif

#  if defined(CONFIG_EXAMPLES_MODULE_ROMFS)
/* Describe the ROMFS file system */

#    define SECTORSIZE   64
#    define NSECTORS(b)  (((b)+SECTORSIZE-1)/SECTORSIZE)
#    define MOUNTPT      "/mnt/romfs"

#    ifndef CONFIG_EXAMPLES_MODULE_DEVMINOR
#      define CONFIG_EXAMPLES_MODULE_DEVMINOR 0
#    endif

#    ifndef CONFIG_EXAMPLES_MODULE_DEVPATH
#      define CONFIG_EXAMPLES_MODULE_DEVPATH "/dev/ram0"
#    endif

#  elif defined(CONFIG_EXAMPLES_MODULE_CROMFS)
/* Describe the CROMFS file system */

#    define MOUNTPT      "/mnt/cromfs"
#  endif

#  define BINDIR         MOUNTPT

#elif defined(CONFIG_EXAMPLES_MODULE_FSMOUNT)
/* Describe how to auto-mount the external file system */

#  define MOUNTPT        "/mnt/" CONFIG_EXAMPLES_MODULE_FSTYPE
#  define BINDIR         MOUNTPT

#else
/* Describe how to use the pre-mounted external file system */

#  define BINDIR         CONFIG_EXAMPLES_MODULE_BINDIR

#endif /* CONFIG_EXAMPLES_MODULE_BUILTINFS */

/****************************************************************************
 * Private data
 ****************************************************************************/

static const char g_write_string[] = "Hi there installed driver\n";

/****************************************************************************
 * Symbols from Auto-Generated Code
 ****************************************************************************/

#if defined(CONFIG_EXAMPLES_MODULE_ROMFS) || defined(CONFIG_EXAMPLES_MODULE_CROMFS)
extern const unsigned char romfs_img[];
extern const unsigned int romfs_img_len;
#endif

#ifdef CONFIG_BUILD_FLAT
extern const struct symtab_s g_mod_exports[];
extern const int g_mod_nexports;
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: module_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_BUILD_FLAT
  struct boardioc_symtab_s symdesc;
#endif
#if defined(CONFIG_EXAMPLES_MODULE_BUILTINFS) && \
    defined(CONFIG_EXAMPLES_MODULE_ROMFS)
  struct boardioc_romdisk_s desc;
#endif
#if defined(CONFIG_EXAMPLES_MODULE_FSMOUNT) && \
    defined(CONFIG_EXAMPLES_MODULE_FSREMOVEABLE)
  struct stat buf;
#endif
  FAR void *handle;
  char buffer[128];
  ssize_t nbytes;
  int ret;
  int fd;

#ifdef CONFIG_BUILD_FLAT
  /* Set the OS symbol table indirectly through the boardctl() */

  symdesc.symtab   = (FAR struct symtab_s *)g_mod_exports;
  symdesc.nsymbols = g_mod_nexports;
  ret = boardctl(BOARDIOC_OS_SYMTAB, (uintptr_t)&symdesc);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: boardctl(BOARDIOC_OS_SYMTAB) failed: %s\n",
              strerror(errno));
      exit(EXIT_FAILURE);
    }
#endif

#ifdef CONFIG_EXAMPLES_MODULE_BUILTINFS
#if defined(CONFIG_EXAMPLES_MODULE_ROMFS)
  /* Create a ROM disk for the ROMFS filesystem */

  printf("main: Registering romdisk at /dev/ram%d\n",
         CONFIG_EXAMPLES_MODULE_DEVMINOR);

  desc.minor    = CONFIG_EXAMPLES_MODULE_DEVMINOR;      /* Minor device number of the ROM disk. */
  desc.nsectors = NSECTORS(romfs_img_len);              /* The number of sectors in the ROM disk */
  desc.sectsize = SECTORSIZE;                           /* The size of one sector in bytes */
  desc.image    = (FAR uint8_t *)romfs_img;             /* File system image */

  ret = boardctl(BOARDIOC_ROMDISK, (uintptr_t)&desc);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: romdisk_register failed: %s\n",
              strerror(errno));
      exit(EXIT_FAILURE);
    }

  /* Mount the file system */

  printf("main: Mounting ROMFS filesystem at target=%s with source=%s\n",
         MOUNTPT, CONFIG_EXAMPLES_MODULE_DEVPATH);

  ret = mount(CONFIG_EXAMPLES_MODULE_DEVPATH, MOUNTPT, "romfs",
              MS_RDONLY, NULL);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: mount(%s,%s,romfs) failed: %s\n",
              CONFIG_EXAMPLES_MODULE_DEVPATH, MOUNTPT, strerror(errno));
      exit(EXIT_FAILURE);
    }

#elif defined(CONFIG_EXAMPLES_MODULE_CROMFS)
  /* Mount the CROMFS file system */

  printf("Mounting CROMFS filesystem at target=%s\n", MOUNTPT);

  ret = mount(NULL, MOUNTPT, "cromfs", MS_RDONLY, NULL);
  if (ret < 0)
    {
      errmsg("ERROR: mount(%s, cromfs) failed: %d\n", MOUNTPT, errno);
    }

#endif /* CONFIG_EXAMPLES_MODULE_ROMFS */
#else /* CONFIG_EXAMPLES_MODULE_BUILTINFS */
  /* An external file system is being used */

#if defined(CONFIG_EXAMPLES_MODULE_FSMOUNT)
#if defined(CONFIG_EXAMPLES_MODULE_FSREMOVEABLE)
  /* The file system is removable, wait until the block driver is available */

  do
    {
      ret = stat(CONFIG_EXAMPLES_MODULE_DEVPATH, &buf);
      if (ret < 0)
        {
          int errcode = errno;
          if (errcode == ENOENT)
            {
              printf("%s does not exist.  Waiting...\n",
                     CONFIG_EXAMPLES_MODULE_DEVPATH);
              sleep(1);
            }
          else
            {
              printf("ERROR: stat(%s) failed: %d  Aborting...\n",
                     CONFIG_EXAMPLES_MODULE_DEVPATH, errcode);
              exit(EXIT_FAILURE);
            }
        }
      else if (!S_ISBLK(buf.st_mode))
        {
          printf("ERROR: stat(%s) exists but is not a block driver: %04x\n",
                 CONFIG_EXAMPLES_MODULE_DEVPATH, buf.st_mode);
          exit(EXIT_FAILURE);
        }
    }
  while (ret < 0);
#endif /* CONFIG_EXAMPLES_MODULE_FSREMOVEABLE */

  /* Mount the external file system */

  printf("Mounting %s filesystem at target=%s\n",
         CONFIG_EXAMPLES_MODULE_FSTYPE, MOUNTPT);

  ret = mount(CONFIG_EXAMPLES_MODULE_DEVPATH, MOUNTPT,
              CONFIG_EXAMPLES_MODULE_FSTYPE, MS_RDONLY, NULL);
  if (ret < 0)
    {
      printf("ERROR: mount(%s, %s, %s) failed: %d\n",
             CONFIG_EXAMPLES_MODULE_DEVPATH, CONFIG_EXAMPLES_MODULE_FSTYPE,
             MOUNTPT, errno);
    }

#endif /* CONFIG_EXAMPLES_MODULE_FSMOUNT */
#endif /* CONFIG_EXAMPLES_MODULE_BUILTINFS */

  /* Install the character driver  */

  handle = insmod(BINDIR "/chardev", "chardev");
  if (handle == NULL)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: insmod(%s/chardev, chardev) failed: %d\n",
              BINDIR, errcode);
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

  printf("main: Read %d bytes\n", (int)nbytes);
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

  printf("main: Wrote %d bytes\n", (int)nbytes);
  lib_dumpbuffer("main: Bytes written", (FAR const uint8_t *)g_write_string,
                 nbytes);

  close(fd);
  ret = rmmod(handle);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: rmmod failed: %d\n", ret);
      exit(EXIT_FAILURE);
    }

  return EXIT_SUCCESS;
}
