/****************************************************************************
 * examples/nxflat/nxflat_main.c
 *
 *   Copyright (C) 2009, 2011, 2017 Gregory Nutt. All rights reserved.
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

#include <sys/mount.h>
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

#include "tests/romfs.h"
#include "tests/dirlist.h"
#include "tests/symtab.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Check configuration.  This is not all of the configuration settings that
 * are required -- only the more obvious.
 */

#if CONFIG_NFILE_DESCRIPTORS < 1
#  error "You must provide file descriptors via CONFIG_NFILE_DESCRIPTORS in your configuration file"
#endif

#ifdef CONFIG_BINFMT_DISABLE
#  error "The binary loader is disabled (CONFIG_BINFMT_DISABLE)!"
#endif

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

/* Describe the ROMFS file system */

#define SECTORSIZE   512
#define NSECTORS(b)  (((b)+SECTORSIZE-1)/SECTORSIZE)
#define ROMFSDEV     "/dev/ram0"
#define MOUNTPT      "/mnt/romfs"

/* If CONFIG_DEBUG_FEATURES is enabled, use info/err instead of printf so
 * that the output will be synchronous with the debug output.
 */

#ifdef CONFIG_CPP_HAVE_VARARGS
#  ifdef CONFIG_DEBUG_INFO
#    define message(format, ...)    syslog(LOG_INFO, format, ##__VA_ARGS__)
#  else
#    define message(format, ...)    printf(format, ##__VA_ARGS__)
#  endif
#  ifdef CONFIG_DEBUG_ERROR
#    define errmsg(format, ...)     syslog(LOG_ERR, format, ##__VA_ARGS__)
#  else
#    define errmsg(format, ...)     fprintf(stderr, format, ##__VA_ARGS__)
#  endif
#else
#  ifdef CONFIG_DEBUG_INFO
#    define message                 _info
#  else
#    define message                 printf
#  endif
#  ifdef CONFIG_DEBUG_ERROR
#    define errmsg                  _err
#  else
#    define errmsg                  printf
#  endif
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char delimiter[] =
  "****************************************************************************";

#ifndef CONFIG_BINFMT_EXEPATH
static char fullpath[128];
#endif

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

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int nxflat_main(int argc, char *argv[])
#endif
{
  FAR char *args[1];
  int ret;
  int i;

  /* Create a ROM disk for the ROMFS filesystem */

  message("Registering romdisk\n");
  ret = romdisk_register(0, (FAR uint8_t *)romfs_img,
                         NSECTORS(romfs_img_len), SECTORSIZE);
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
      errmsg("ERROR: mount(%s,%s,romfs) failed: %s\n",
             ROMFSDEV, MOUNTPT, errno);
    }

#if defined(CONFIG_BINFMT_EXEPATH) && !defined(CONFIG_PATH_INITIAL)
  /* Does the system support the PATH variable?  Has the PATH variable
   * already been set?  If YES and NO, then set the PATH variable to
   * the ROMFS mountpoint.
   */

  (void)setenv("PATH", MOUNTPT, 1);
#endif

  /* Now excercise every progrm in the ROMFS file system */

  for (i = 0; dirlist[i]; i++)
    {
      FAR const char *filename;

      /* Output a seperated so that we can clearly discrinmate the output of
       * this program from the others.
       */

      testheader(dirlist[i]);

      /* If the binary loader does not support the PATH variable, then
       * create the full path to the executable program.  Otherwise,
       * use the relative path so that the binary loader will have to
       * search the PATH variable to find the executable.
       */

#ifdef CONFIG_BINFMT_EXEPATH
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

      args[0] = NULL;
      ret = exec(filename, args, g_nxflat_exports, NEXPORTS);
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
