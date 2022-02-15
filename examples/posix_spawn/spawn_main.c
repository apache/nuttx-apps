/****************************************************************************
 * apps/examples/posix_spawn/spawn_main.c
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
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <spawn.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/drivers/ramdisk.h>
#include <nuttx/symtab.h>

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

#ifndef CONFIG_FS_ROMFS
#  error "You must select CONFIG_FS_ROMFS in your configuration file"
#endif

#ifdef CONFIG_DISABLE_MOUNTPOINT
#  error "You must not disable mountpoints via CONFIG_DISABLE_MOUNTPOINT in your configuration file"
#endif

#ifdef CONFIG_BINFMT_DISABLE
#  error "You must not disable loadable modules via CONFIG_BINFMT_DISABLE in your configuration file"
#endif

#ifndef CONFIG_BOARDCTL
#  error "This configuration requires CONFIG_BOARDCTL"
#endif

#ifndef CONFIG_BOARDCTL_APP_SYMTAB
#  error "You must enable the symobol table interface with CONFIG_BOARDCTL_APP_SYMTAB"
#endif

/* Describe the ROMFS file system */

#define SECTORSIZE   512
#define NSECTORS(b)  (((b)+SECTORSIZE-1)/SECTORSIZE)
#define MOUNTPT      "/mnt/romfs"

#ifndef CONFIG_EXAMPLES_POSIXSPAWN_DEVMINOR
#  define CONFIG_EXAMPLES_POSIXSPAWN_DEVMINOR 0
#endif

#ifndef CONFIG_EXAMPLES_POSIXSPAWN_DEVPATH
#  define CONFIG_EXAMPLES_POSIXSPAWN_DEVPATH "/dev/ram0"
#endif

/* If CONFIG_DEBUG_FEATURES is enabled, use info/err instead of printf so
 * that the output will be synchronous with the debug output.
 */

#ifdef CONFIG_DEBUG_FEATURES
#  define message                 _info
#  define errmsg                  _err
#else
#  define message                 printf
#  define errmsg                  printf
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static unsigned int g_mminitial;  /* Initial memory usage */
static unsigned int g_mmstep;     /* Memory Usage at beginning of test step */

static const char delimiter[] =
  "**************************************"
  "**************************************";
static const char g_data[]     = "testdata.txt";

static char fullpath[128];

static char * const g_hello_argv[5] =
{
  "hello", "Argument 1", "Argument 2", "Argument 3", NULL
};

static char * const g_redirect_argv[2] =
{
  "redirect", NULL
};

/****************************************************************************
 * Symbols from Auto-Generated Code
 ****************************************************************************/

extern const unsigned char romfs_img[];
extern const unsigned int romfs_img_len;

extern const struct symtab_s g_spawn_exports[];
extern const int g_spawn_nexports;

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
 * Name: spawn_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct boardioc_symtab_s symdesc;
  posix_spawn_file_actions_t file_actions;
  posix_spawnattr_t attr;
  FAR const char *filepath;
  pid_t pid;
  int ret;
  struct boardioc_romdisk_s desc;

  /* Initialize the memory monitor */

  mm_initmonitor();

  /* Create a ROM disk for the ROMFS filesystem */

  desc.minor    = CONFIG_EXAMPLES_POSIXSPAWN_DEVMINOR;  /* Minor device number of the ROM disk. */
  desc.nsectors = NSECTORS(romfs_img_len);              /* The number of sectors in the ROM disk */
  desc.sectsize = SECTORSIZE;                           /* The size of one sector in bytes */
  desc.image    = (FAR uint8_t *)romfs_img;             /* File system image */

  message("Registering romdisk at /dev/ram%d\n",
          CONFIG_EXAMPLES_POSIXSPAWN_DEVMINOR);

  ret = boardctl(BOARDIOC_ROMDISK, (uintptr_t)&desc);

  if (ret < 0)
    {
      errmsg("ERROR: romdisk_register failed: %s\n", strerror(errno));
      exit(1);
    }

  mm_update(&g_mmstep, "after romdisk_register");

  /* Mount the file system */

  message("Mounting ROMFS filesystem at target=%s with source=%s\n",
          MOUNTPT, CONFIG_EXAMPLES_POSIXSPAWN_DEVPATH);

  ret = mount(CONFIG_EXAMPLES_POSIXSPAWN_DEVPATH, MOUNTPT, "romfs",
              MS_RDONLY, NULL);
  if (ret < 0)
    {
      errmsg("ERROR: mount(%s,%s,romfs) failed: %s\n",
             CONFIG_EXAMPLES_POSIXSPAWN_DEVPATH, MOUNTPT, strerror(errno));
    }

  mm_update(&g_mmstep, "after mount");

  /* Does the system support the PATH variable?  Has the PATH variable
   * already been set?  If YES and NO, then set the PATH variable to
   * the ROMFS mountpoint.
   */

#if defined(CONFIG_LIBC_ENVPATH) && !defined(CONFIG_PATH_INITIAL)
  setenv("PATH", MOUNTPT, 1);
#endif

  /* Make sure that we are using our symbol tablee */

  symdesc.symtab   = (FAR struct symtab_s *)g_spawn_exports; /* Discard 'const' */
  symdesc.nsymbols = g_spawn_nexports;
  boardctl(BOARDIOC_APP_SYMTAB, (uintptr_t)&symdesc);

  /**************************************************************************
   * Case 1: Simple program with arguments
   **************************************************************************/

  /* Output a separator so that we can clearly discriminate the output of
   * this program from the others.
   */

  testheader(g_hello_argv[0]);

  /* Initialize the attributes file actions structure */

  ret = posix_spawn_file_actions_init(&file_actions);
  if (ret != 0)
    {
      errmsg("ERROR: posix_spawn_file_actions_init failed: %d\n", ret);
    }

  posix_spawn_file_actions_dump(&file_actions);

  ret = posix_spawnattr_init(&attr);
  if (ret != 0)
    {
      errmsg("ERROR: posix_spawnattr_init failed: %d\n", ret);
    }

  posix_spawnattr_dump(&attr);

  mm_update(&g_mmstep, "after file_action/attr init");

  /* If the binary loader does not support the PATH variable, then
   * create the full path to the executable program.  Otherwise,
   * use the relative path so that the binary loader will have to
   * search the PATH variable to find the executable.
   */

#ifdef CONFIG_LIBC_ENVPATH
  filepath = g_hello_argv[0];
#else
  snprintf(fullpath, 128, "%s/%s", MOUNTPT, g_hello_argv[0]);
  filepath = fullpath;
#endif

  /* Execute the program */

  mm_update(&g_mmstep, "before posix_spawn");

  ret = posix_spawn(&pid, filepath, &file_actions,
                    &attr, g_hello_argv, NULL);
  if (ret != 0)
    {
      errmsg("ERROR: posix_spawn failed: %d\n", ret);
    }

  sleep(4);
  mm_update(&g_mmstep, "after posix_spawn");

  /* Free attributes and file actions */

  ret = posix_spawn_file_actions_destroy(&file_actions);
  if (ret != 0)
    {
      errmsg("ERROR: posix_spawn_file_actions_destroy failed: %d\n", ret);
    }

  posix_spawn_file_actions_dump(&file_actions);

  ret = posix_spawnattr_destroy(&attr);
  if (ret != 0)
    {
      errmsg("ERROR: posix_spawnattr_destroy failed: %d\n", ret);
    }

  posix_spawnattr_dump(&attr);

  mm_update(&g_mmstep, "after file_action/attr destruction");

  /**************************************************************************
   * Case 2: Simple program with redirection of stdin to a file input
   **************************************************************************/

  /* Output a separator so that we can clearly discriminate the output of
   * this program from the others.
   */

  testheader(g_redirect_argv[0]);

  /* Initialize the attributes file actions structure */

  ret = posix_spawn_file_actions_init(&file_actions);
  if (ret != 0)
    {
      errmsg("ERROR: posix_spawn_file_actions_init failed: %d\n", ret);
    }

  posix_spawn_file_actions_dump(&file_actions);

  ret = posix_spawnattr_init(&attr);
  if (ret != 0)
    {
      errmsg("ERROR: posix_spawnattr_init failed: %d\n", ret);
    }

  posix_spawnattr_dump(&attr);

  mm_update(&g_mmstep, "after file_action/attr init");

  /* Set up to close stdin (0) and open testdata.txt as the program input */

  ret = posix_spawn_file_actions_addclose(&file_actions, 0);
  if (ret != 0)
    {
      errmsg("ERROR: posix_spawn_file_actions_addclose failed: %d\n", ret);
    }

  posix_spawn_file_actions_dump(&file_actions);

  snprintf(fullpath, 128, "%s/%s", MOUNTPT, g_data);
  ret = posix_spawn_file_actions_addopen(&file_actions, 0, fullpath,
                                         O_RDONLY, 0644);
  if (ret != 0)
    {
      errmsg("ERROR: posix_spawn_file_actions_addopen failed: %d\n", ret);
    }

  posix_spawn_file_actions_dump(&file_actions);

  mm_update(&g_mmstep, "after adding file_actions");

  /* If the binary loader does not support the PATH variable, then
   * create the full path to the executable program.  Otherwise,
   * use the relative path so that the binary loader will have to
   * search the PATH variable to find the executable.
   */

#ifdef CONFIG_LIBC_ENVPATH
  filepath = g_redirect_argv[0];
#else
  snprintf(fullpath, 128, "%s/%s", MOUNTPT, g_redirect_argv[0]);
  filepath = fullpath;
#endif

  /* Execute the program */

  mm_update(&g_mmstep, "before posix_spawn");

  ret = posix_spawn(&pid, filepath, &file_actions,
                    &attr, g_redirect_argv, NULL);
  if (ret != 0)
    {
      errmsg("ERROR: posix_spawn failed: %d\n", ret);
    }

  sleep(2);
  mm_update(&g_mmstep, "after posix_spawn");

  /* Free attributes and file actions */

  ret = posix_spawn_file_actions_destroy(&file_actions);
  if (ret != 0)
    {
      errmsg("ERROR: posix_spawn_file_actions_destroy failed: %d\n", ret);
    }

  posix_spawn_file_actions_dump(&file_actions);

  ret = posix_spawnattr_destroy(&attr);
  if (ret != 0)
    {
      errmsg("ERROR: posix_spawnattr_destroy failed: %d\n", ret);
    }

  posix_spawnattr_dump(&attr);

  mm_update(&g_mmstep, "after file_action/attr destruction");

  /* Clean-up */

  mm_update(&g_mmstep, "End-of-Test");
  return 0;
}
