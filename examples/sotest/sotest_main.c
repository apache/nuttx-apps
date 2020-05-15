/****************************************************************************
 * examples/sotest/sotest_main.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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

#ifdef CONFIG_EXAMPLES_SOTEST_BUILTINFS
#  include <sys/mount.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <dlfcn.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/symtab.h>

#ifdef CONFIG_EXAMPLES_SOTEST_BUILTINFS
#  include <nuttx/drivers/ramdisk.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Check configuration.  This is not all of the configuration settings that
 * are required -- only the more obvious.
 */

#ifndef CONFIG_LIBC_DLFCN
#  error "You must select CONFIG_LIBC_DLFCN in your configuration file"
#endif

#ifdef CONFIG_EXAMPLES_SOTEST_BUILTINFS
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

#  ifndef CONFIG_EXAMPLES_SOTEST_DEVMINOR
#    define CONFIG_EXAMPLES_SOTEST_DEVMINOR 0
#  endif

#  ifndef CONFIG_EXAMPLES_SOTEST_DEVPATH
#    define CONFIG_EXAMPLES_SOTEST_DEVPATH "/dev/ram0"
#  endif
#else
#  define BINDIR       CONFIG_EXAMPLES_SOTEST_BINDIR
#endif /* CONFIG_EXAMPLES_SOTEST_BUILTINFS */

/****************************************************************************
 * Symbols from Auto-Generated Code
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_SOTEST_BUILTINFS
extern const unsigned char romfs_img[];
extern const unsigned int romfs_img_len;
#endif

extern const struct symtab_s g_sot_exports[];
extern const int g_sot_nexports;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sotest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#if CONFIG_MODLIB_MAXDEPEND > 0
  FAR void *handle1;
#endif
  FAR void *handle2;
  CODE void (*testfunc)(FAR const char *msg);
  FAR const char *msg;
  int ret;

  /* Set the shared library symbol table */

  ret = dlsymtab((FAR struct symtab_s *)g_sot_exports, g_sot_nexports);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: dlsymtab failed: %d\n", ret);
      exit(EXIT_FAILURE);
    }

#ifdef CONFIG_EXAMPLES_SOTEST_BUILTINFS
  /* Create a ROM disk for the ROMFS filesystem */

  printf("main: Registering romdisk at /dev/ram%d\n",
         CONFIG_EXAMPLES_SOTEST_DEVMINOR);

  ret = romdisk_register(CONFIG_EXAMPLES_SOTEST_DEVMINOR,
                         (FAR uint8_t *)romfs_img,
                         NSECTORS(romfs_img_len), SECTORSIZE);
  if (ret < 0)
    {
      /* This will happen naturally if we registered the ROM disk
       * previously.
       */

      if (ret != -EEXIST)
        {
          fprintf(stderr, "ERROR: romdisk_register failed: %d\n", ret);
          exit(EXIT_FAILURE);
        }

      printf("main: ROM disk already registered\n");
    }

  /* Mount the file system */

  printf("main: Mounting ROMFS filesystem at target=%s with source=%s\n",
         BINDIR, CONFIG_EXAMPLES_SOTEST_DEVPATH);

  ret = mount(CONFIG_EXAMPLES_SOTEST_DEVPATH, BINDIR, "romfs", MS_RDONLY,
              NULL);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: mount(%s,%s,romfs) failed: %s\n",
              CONFIG_EXAMPLES_SOTEST_DEVPATH, BINDIR, strerror(errno));
      exit(EXIT_FAILURE);
    }
#endif /* CONFIG_EXAMPLES_SOTEST_BUILTINFS */

#if CONFIG_MODLIB_MAXDEPEND > 0
  /* Install the first test shared library.  The first shared library only
   * verifies that symbols exported be one shared library can be used to
   * resolve undefined symbols in a second shared library.
   */

  /* Install the second test shared library  */

  handle1 = dlopen(BINDIR "/modprint", RTLD_NOW | RTLD_LOCAL);
  if (handle1 == NULL)
    {
      fprintf(stderr, "ERROR: dlopen(/modprint) failed\n");
      exit(EXIT_FAILURE);
    }
#endif

  /* Install the second test shared library  */

  handle2 = dlopen(BINDIR "/sotest", RTLD_NOW | RTLD_LOCAL);
  if (handle2 == NULL)
    {
      fprintf(stderr, "ERROR: dlopen(/sotest) failed\n");
      exit(EXIT_FAILURE);
    }

  /* Get symbols testfunc1 and msg1 from the second test shared library */

  testfunc = (CODE void (*)(FAR const char *))dlsym(handle2, "testfunc1");
  if (testfunc == NULL)
    {
      fprintf(stderr, "ERROR: Failed to get symbol \"testfunc1\"\n");
      exit(EXIT_FAILURE);
    }

  msg = (FAR const char *)dlsym(handle2, "g_msg1");
  if (msg == NULL)
    {
      fprintf(stderr, "ERROR: Failed to get symbol \"g_msg1\"\n");
      exit(EXIT_FAILURE);
    }

  /* Execute testfunc1 */

  testfunc(msg);

  /* Get symbols testfunc2 and msg2 */

  testfunc = (CODE void (*)(FAR const char *))dlsym(handle2, "testfunc2");
  if (testfunc == NULL)
    {
      fprintf(stderr, "ERROR: Failed to get symbol \"testfunc2\"\n");
      exit(EXIT_FAILURE);
    }

  msg = (FAR const char *)dlsym(handle2, "g_msg2");
  if (msg == NULL)
    {
      fprintf(stderr, "ERROR: Failed to get symbol \"g_msg2\"\n");
      exit(EXIT_FAILURE);
    }

  /* Execute testfunc2 */

  testfunc(msg);

  /* Get symbols testfunc3 and msg3 */

  testfunc = (CODE void (*)(FAR const char *))dlsym(handle2, "testfunc3");
  if (testfunc == NULL)
    {
      fprintf(stderr, "ERROR: Failed to get symbol \"testfunc3\"\n");
      exit(EXIT_FAILURE);
    }

  msg = (FAR const char *)dlsym(handle2, "g_msg3");
  if (msg == NULL)
    {
      fprintf(stderr, "ERROR: Failed to get symbol \"g_msg3\"\n");
      exit(EXIT_FAILURE);
    }

  /* Execute testfunc3 */

  testfunc(msg);

#if CONFIG_MODLIB_MAXDEPEND > 0
  /* This should fail because the second shared library depends on
   * the first.
   */

  ret = dlclose(handle1);
  if (ret == 0)
    {
      fprintf(stderr,
              "ERROR: dlclose(handle1) succeeded with a dependency\n");
      exit(EXIT_FAILURE);
    }
#endif

  /* Close the second shared library */

  ret = dlclose(handle2);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: rmmod(handle2) failed: %d\n", ret);
      exit(EXIT_FAILURE);
    }

#if CONFIG_MODLIB_MAXDEPEND > 0
  /* Now we should be able to close the first shared library. */

  ret = dlclose(handle1);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: rmmod(handle1) failed: %d\n", ret);
      exit(EXIT_FAILURE);
    }
#endif

#ifdef CONFIG_EXAMPLES_SOTEST_BUILTINFS
  ret = umount(BINDIR);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: umount(%s) failed: %d\n",
              BINDIR, errno);
      exit(EXIT_FAILURE);
    }
#endif /* CONFIG_EXAMPLES_SOTEST_BUILTINFS */

  return EXIT_SUCCESS;
}
