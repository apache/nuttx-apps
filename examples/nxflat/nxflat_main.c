/****************************************************************************
 * apps/examples/nxflat/nxflat_main.c
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
#include <sys/boardctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/symtab.h>
#include <nuttx/drivers/ramdisk.h>
#include <nuttx/binfmt/binfmt.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Check configuration.  This is not all of the configuration settings that
 * are required -- only the more obvious.
 */

#ifndef CONFIG_NXFLAT
#  error "You must select CONFIG_NXFLAT in your configuration file"
#endif

#ifndef CONFIG_FS_ROMFS
#  error "You must select CONFIG_FS_ROMFS in your configuration file"
#endif

#ifdef CONFIG_DISABLE_MOUNTPOINT
#  error "You must not disable mountpoints via CONFIG_DISABLE_MOUNTPOINT in your configuration file"
#endif

#ifdef CONFIG_BINFMT_DISABLE
#  error "You must not disable loadable modules via CONFIG_BINFMT_DISABLE in your configuration file"
#endif

#ifndef CONFIG_BOARDCTL_ROMDISK
#  error "CONFIG_BOARDCTL_ROMDISK should be enabled in the configuration file"
#endif
/* Describe the ROMFS file system */

#define SECTORSIZE   512
#define NSECTORS(b)  (((b)+SECTORSIZE-1)/SECTORSIZE)
#define ROMFSDEV     "/dev/ram0"
#define MOUNTPT      "/mnt/romfs"

/* If CONFIG_DEBUG_FEATURES is enabled, use info/err instead of printf so
 * that the output will be synchronous with the debug output.
 */

#ifdef CONFIG_DEBUG_INFO
#  define message                 _info
#else
#  define message                 printf
#endif
#ifdef CONFIG_DEBUG_ERROR
#  define errmsg                  _err
#else
#  define errmsg                  printf
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char delimiter[] =
  "**************************************"
  "**************************************";

#ifndef CONFIG_LIBC_ENVPATH
static char fullpath[128];
#endif

/****************************************************************************
 * Symbols from Auto-Generated Code
 ****************************************************************************/

extern const unsigned char romfs_img[];
extern const unsigned int romfs_img_len;

extern const char *dirlist[];

extern const struct symtab_s g_nxflat_exports[];
extern const int g_nxflat_nexports;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: testheader
 ****************************************************************************/

static inline void testheader(FAR const char *progname)
{
  message("\n%s\n* Executing %s\n%s\n\n", delimiter, progname, delimiter);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxflat_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR char *args[2];
  int ret;
  int i;
  struct boardioc_romdisk_s desc;

  /* Create a ROM disk for the ROMFS filesystem */

  message("Registering romdisk\n");

  desc.minor    = 0;                                    /* Minor device number of the ROM disk. */
  desc.nsectors = NSECTORS(romfs_img_len);              /* The number of sectors in the ROM disk */
  desc.sectsize = SECTORSIZE;                           /* The size of one sector in bytes */
  desc.image    = (FAR uint8_t *)romfs_img;             /* File system image */

  ret = boardctl(BOARDIOC_ROMDISK, (uintptr_t)&desc);
  if (ret < 0)
    {
      errmsg("ERROR: romdisk_register failed: %d\n", ret);
      exit(1);
    }

  /* Mount the file system */

  message("Mounting ROMFS filesystem at target=%s with source=%s\n",
         MOUNTPT, ROMFSDEV);

  ret = mount(ROMFSDEV, MOUNTPT, "romfs", MS_RDONLY, NULL);
  if (ret < 0)
    {
      errmsg("ERROR: mount(%s,%s,romfs) failed: %d\n",
             ROMFSDEV, MOUNTPT, errno);
    }

#if defined(CONFIG_LIBC_ENVPATH) && !defined(CONFIG_PATH_INITIAL)
  /* Does the system support the PATH variable?  Has the PATH variable
   * already been set?  If YES and NO, then set the PATH variable to
   * the ROMFS mountpoint.
   */

  setenv("PATH", MOUNTPT, 1);
#endif

  /* Now exercise every progrm in the ROMFS file system */

  for (i = 0; dirlist[i]; i++)
    {
      FAR const char *filename;

      /* Output a separated so that we can clearly discrinmate the output of
       * this program from the others.
       */

      testheader(dirlist[i]);

      /* If the binary loader does not support the PATH variable, then
       * create the full path to the executable program.  Otherwise,
       * use the relative path so that the binary loader will have to
       * search the PATH variable to find the executable.
       */

#ifdef CONFIG_LIBC_ENVPATH
      filename = dirlist[i];
#else
      snprintf(fullpath, 128, "%s/%s", MOUNTPT, dirlist[i]);
      filename = fullpath;
#endif

      /* Execute the NXFLAT module
       *
       * NOTE: The standard posix_spawn() interface would be more correct.
       * The non-standard exec() function is used because it provides a
       * simple way to pass the symbol table information needed to load the
       * program.  posix_spawn(), on the other hand, will assume that symbol
       * table information is available within the OS.
       */

      args[0] = (FAR char *)dirlist[i];
      args[1] = NULL;
      ret = exec(filename, args, g_nxflat_exports, g_nxflat_nexports);
      if (ret < 0)
        {
          errmsg("ERROR: exec(%s) failed: %d\n", dirlist[i], errno);
        }
      else
        {
          message("Wait a bit for test completion\n");
          sleep(4);
        }
    }

  message("End-of-Test.. Exit-ing\n");
  return 0;
}
