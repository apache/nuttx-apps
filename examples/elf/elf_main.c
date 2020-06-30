/****************************************************************************
 * examples/elf/elf_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
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

#ifdef CONFIG_BINFMT_DISABLE
#  error "The binary loader is disabled (CONFIG_BINFMT_DISABLE)!"
#endif

#ifndef CONFIG_ELF
#  error "You must select CONFIG_ELF in your configuration file"
#endif

#ifdef CONFIG_DISABLE_MOUNTPOINT
#  error "You must not disable mountpoints via CONFIG_DISABLE_MOUNTPOINT in your configuration file"
#endif

#if defined(CONFIG_EXAMPLES_ELF_ROMFS)
/* Describe the ROMFS file system */

#  define SECTORSIZE   512
#  define NSECTORS(b)  (((b) + SECTORSIZE - 1) / SECTORSIZE)
#  define MOUNTPT      "/mnt/romfs"

#  ifndef CONFIG_EXAMPLES_ELF_DEVMINOR
#    define CONFIG_EXAMPLES_ELF_DEVMINOR 0
#  endif

#  ifndef CONFIG_EXAMPLES_ELF_DEVPATH
#    define CONFIG_EXAMPLES_ELF_DEVPATH "/dev/ram0"
#  endif

#elif defined(CONFIG_EXAMPLES_ELF_CROMFS)
/* Describe the CROMFS file system */

#  define MOUNTPT      "/mnt/cromfs"

#elif defined(CONFIG_EXAMPLES_ELF_EXTERN) && defined(CONFIG_EXAMPLES_ELF_FSMOUNT)
/* Describe the external file system */

#  define MOUNTPT      "/mnt/" CONFIG_EXAMPLES_ELF_FSTYPE

#else
#  undef MOUNTPT
#endif

/* If CONFIG_DEBUG_FEATURES is enabled, use info/err instead of printf so
 * that the output will be synchronous with the debug output.
 */

#ifdef CONFIG_DEBUG_INFO
#  define message               _info
#else
#  define message               printf
#endif
#ifdef CONFIG_DEBUG_ERROR
#  define errmsg                _err
#else
#  define errmsg                printf
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static unsigned int g_mminitial;  /* Initial memory usage */
static unsigned int g_mmstep;     /* Memory Usage at beginning of test step */

static const char delimiter[] =
  "**************************************"
  "**************************************";

#ifndef CONFIG_LIB_ENVPATH
static char fullpath[128];
#endif

/****************************************************************************
 * Symbols from Auto-Generated Code
 ****************************************************************************/

#if defined(CONFIG_EXAMPLES_ELF_ROMFS) || defined(CONFIG_EXAMPLES_ELF_CROMFS)
extern const unsigned char romfs_img[];
extern const unsigned int romfs_img_len;
#elif !defined(CONFIG_EXAMPLES_ELF_EXTERN)
#  error "No file system selected"
#endif

extern const char *dirlist[];

extern const struct symtab_s g_elf_exports[];
extern const int g_elf_nexports;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mm_update
 ****************************************************************************/

static void mm_update(FAR unsigned int *previous, FAR const char *msg)
{
  struct mallinfo mmcurrent;

  /* Get the current memory usage */

  mmcurrent = mallinfo();

  /* Show the change from the previous time */

  printf("\nMemory Usage %s:\n", msg);
  printf("  Before: %8u After: %8u Change: %8d\n",
         *previous, mmcurrent.uordblks,
         (int)mmcurrent.uordblks - (int)*previous);

  /* Set up for the next test */

  *previous =  mmcurrent.uordblks;
}

/****************************************************************************
 * Name: mm_initmonitor
 ****************************************************************************/

static void mm_initmonitor(void)
{
  struct mallinfo mmcurrent;

  mmcurrent = mallinfo();

  g_mminitial = mmcurrent.uordblks;
  g_mmstep    = mmcurrent.uordblks;

  printf("Initial memory usage: %d\n", mmcurrent.uordblks);
}

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
 * Name: elf_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_EXAMPLES_ELF_FSREMOVEABLE
  struct stat buf;
#endif
  FAR char *args[1];
  int ret;
  int i;

  /* Initialize the memory monitor */

  mm_initmonitor();

#if defined(CONFIG_EXAMPLES_ELF_ROMFS)
#if defined(CONFIG_BUILD_FLAT)
  /* This example violates the portable POSIX interface by calling the OS
   * internal function romdisk_register() (aka ramdisk_register()).  We can
   * squeak by in with this violation in the FLAT build mode, but not in
   * other build modes.  In other build modes, the following logic must be
   * performed in the OS board initialization logic (where it really belongs
   * anyway).
   */

  /* Create a ROM disk for the ROMFS filesystem */

  message("Registering romdisk at /dev/ram%d\n",
          CONFIG_EXAMPLES_ELF_DEVMINOR);
  ret = romdisk_register(CONFIG_EXAMPLES_ELF_DEVMINOR,
                         (FAR uint8_t *)romfs_img,
                         NSECTORS(romfs_img_len), SECTORSIZE);
  if (ret < 0)
    {
      errmsg("ERROR: romdisk_register failed: %d\n", ret);
      exit(1);
    }

  mm_update(&g_mmstep, "after romdisk_register");
#endif

  /* Mount the ROMFS file system */

  message("Mounting ROMFS filesystem at target=%s with source=%s\n",
         MOUNTPT, CONFIG_EXAMPLES_ELF_DEVPATH);

  ret = mount(CONFIG_EXAMPLES_ELF_DEVPATH, MOUNTPT, "romfs",
              MS_RDONLY, NULL);
  if (ret < 0)
    {
      errmsg("ERROR: mount(%s,%s,romfs) failed: %s\n",
             CONFIG_EXAMPLES_ELF_DEVPATH, MOUNTPT, errno);
    }

#elif defined(CONFIG_EXAMPLES_ELF_CROMFS)
  /* Mount the CROMFS file system */

  message("Mounting CROMFS filesystem at target=%s\n", MOUNTPT);

  ret = mount(NULL, MOUNTPT, "cromfs", MS_RDONLY, NULL);
  if (ret < 0)
    {
      errmsg("ERROR: mount(%s, cromfs) failed: %d\n", MOUNTPT, errno);
    }
#elif defined(CONFIG_EXAMPLES_ELF_EXTERN)
  /* An external file system is being used */

#if defined(CONFIG_EXAMPLES_ELF_FSMOUNT)
#if defined(CONFIG_EXAMPLES_ELF_FSREMOVEABLE)
  /* The file system is removable, wait until the block driver is available */

  do
    {
      ret = stat(CONFIG_EXAMPLES_ELF_DEVPATH, &buf);
      if (ret < 0)
        {
          int errcode = errno;
          if (errcode == ENOENT)
            {
              printf("%s does not exist.  Waiting...\n",
                     CONFIG_EXAMPLES_ELF_DEVPATH);
              sleep(1);
            }
          else
            {
              printf("ERROR: stat(%s) failed: %d  Aborting...\n",
                     CONFIG_EXAMPLES_ELF_DEVPATH, errcode);
              exit(EXIT_FAILURE);
            }
        }
      else if (!S_ISBLK(buf.st_mode))
        {
          printf("ERROR: stat(%s) exists but is not a block driver: %04x\n",
                 CONFIG_EXAMPLES_ELF_DEVPATH, buf.st_mode);
          exit(EXIT_FAILURE);
        }
    }
  while (ret < 0);
#endif

  /* Mount the external file system */

  message("Mounting %s filesystem at target=%s\n",
          CONFIG_EXAMPLES_ELF_FSTYPE, MOUNTPT);

  ret = mount(CONFIG_EXAMPLES_ELF_DEVPATH, MOUNTPT,
              CONFIG_EXAMPLES_ELF_FSTYPE, MS_RDONLY, NULL);
  if (ret < 0)
    {
      errmsg("ERROR: mount(%s, %s, %s) failed: %d\n",
             CONFIG_EXAMPLES_ELF_DEVPATH, CONFIG_EXAMPLES_ELF_FSTYPE,
             MOUNTPT, errno);
    }
#endif
#else
#  Warning "No file system selected"
#endif

  mm_update(&g_mmstep, "after mount");

#if defined(CONFIG_LIB_ENVPATH) && defined(MOUNTPT)
  /* Does the system support the PATH variable?
   * If YES, then set the PATH variable to the ROMFS mountpoint.
   */

  setenv("PATH", MOUNTPT, 1);
#endif

  /* Now exercise every program in the ROMFS file system */

  for (i = 0; dirlist[i]; i++)
    {
      FAR const char *filename;

      /* Output a separator so that we can clearly discriminate the output of
       * this program from the others.
       */

      testheader(dirlist[i]);

      /* If the binary loader does not support the PATH variable, then
       * create the full path to the executable program.  Otherwise,
       * use the relative path so that the binary loader will have to
       * search the PATH variable to find the executable.
       */

#ifdef CONFIG_LIB_ENVPATH
      filename = dirlist[i];
#else
      snprintf(fullpath, 128, "%s/%s", MOUNTPT, dirlist[i]);
      filename = fullpath;
#endif

      /* Execute the ELF module.
       *
       * NOTE: The standard posix_spawn() interface would be more correct.
       * The non-standard exec() function is used because it provides a
       * simple way to pass the symbol table information needed to load the
       * program.  posix_spawn(), on the other hand, will assume that symbol
       * table information is available within the OS.
       */

      args[0] = NULL;
      ret = exec(filename, args, g_elf_exports, g_elf_nexports);

      mm_update(&g_mmstep, "after exec");

      if (ret < 0)
        {
          errmsg("ERROR: exec(%s) failed: %d\n", dirlist[i], errno);
        }
      else
        {
          message("Wait a bit for test completion\n");
          sleep(4);
        }

      mm_update(&g_mmstep, "after program execution");
    }

  mm_update(&g_mmstep, "End-of-Test");
  return 0;
}
