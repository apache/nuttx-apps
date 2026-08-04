/****************************************************************************
 * apps/examples/fdpicxip/fdpicxip_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
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
 * End to end demonstration of the FDPIC module loader over a writable
 * execute-in-place file system:
 *
 *   1. write a module into xipfs at run time, the way a download would,
 *   2. confirm the file system can hand out a direct flash pointer for it,
 *   3. run it, in some cases as two concurrent instances,
 *   4. confirm the shared text was pinned in place while they ran and
 *      released afterwards.
 *
 * The point of FDPIC here is that text stays in flash and only the writable
 * segment is copied, once per running instance -- so a second instance, or
 * a second module sharing a library, costs data and no code.
 *
 * This demonstrates; it does not assert.  The assertions live in the fdpic
 * and reject sections of apps/testing/fs/xipfs.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <nuttx/fs/ioctl.h>
#include <nuttx/fs/xipfs.h>

#include "qsorter_bin.h"
#include "libcounter_bin.h"
#include "user_bin.h"
#include "libshape_bin.h"
#include "cxxuser_bin.h"
#include "lazymod_bin.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MOUNTPT  CONFIG_EXAMPLES_FDPICXIP_MOUNTPT

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: extent_info_path
 *
 * Description:
 *   Where a file physically lies: its extent, its flash address, and how
 *   many live mappings are pinning it in place.
 *
 ****************************************************************************/

static int extent_info_path(FAR const char *path,
                            FAR struct xipfs_extent_info_s *info)
{
  int fd;
  int ret;

  fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      return -errno;
    }

  ret = ioctl(fd, XIPFSIOC_EXTENTINFO, (unsigned long)(uintptr_t)info);
  close(fd);

  return ret < 0 ? -errno : 0;
}

/****************************************************************************
 * Shared library demonstration
 *
 * Stages a library and a module that needs it, then runs two instances.
 * Both share one mapped copy of each object's code, executed in place from
 * flash.  Each instance gets its own copy of the module's data, because
 * exec() loads the module afresh; the library is opened with dlopen() and
 * so there is one of it, data included.  The totals therefore interleave,
 * and each instance checks only that its own adds all landed.
 ****************************************************************************/

static int stage_blob(FAR const char *path, FAR const unsigned char *data,
                      unsigned int len)
{
  ssize_t n;
  int fd;

  unlink(path);

  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0755);
  if (fd < 0)
    {
      return -errno;
    }

  if (ftruncate(fd, len) < 0)
    {
      close(fd);
      return -errno;
    }

  n = write(fd, data, len);
  close(fd);

  return (n == (ssize_t)len) ? 0 : -EIO;
}

/****************************************************************************
 * Name: qsorter_main
 *
 * Description:
 *   The simplest case the loader has: one self-contained FDPIC module, no
 *   library behind it, run as two concurrent instances.  It is the FDPIC
 *   counterpart of what this demo does by default with NXFLAT, and the
 *   only subcommand that exercises a module with no second object -- solib
 *   and cxx both bring a library, jmprel is about how imports are bound.
 *
 *   The two instances overlap deliberately: each fills its array, sleeps,
 *   then sorts.  One shared copy of the text, a private copy of the data
 *   per instance -- shared data would show up as corrupted output, and the
 *   pin count while both are alive is what stops the defragmenter
 *   relocating code that is executing.
 *
 ****************************************************************************/

static int qsorter_main(void)
{
  struct xipfs_extent_info_s info;
  FAR char *args[3];
  pid_t pid[2];
  char seed[8];
  int status;
  int ret;
  int i;

  syslog(LOG_INFO,
         "\n=== FDPIC module executed in place from xipfs ===\n\n");

  ret = stage_blob(MOUNTPT "/qsorter", g_qsorter_nxf, g_qsorter_nxf_len);
  if (ret < 0)
    {
      syslog(LOG_INFO, "staging the module failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  syslog(LOG_INFO, "staged qsorter (%u bytes)\n", g_qsorter_nxf_len);

  if (extent_info_path(MOUNTPT "/qsorter", &info) == 0)
    {
      syslog(LOG_INFO, "extent: block %" PRIu32 " x%" PRIu32 ", size %"
             PRIu32 ", flash addr 0x%08lx\n",
             info.start_block, info.nblocks, info.size,
             (unsigned long)info.xipaddr);

      if (info.xipaddr == 0)
        {
          syslog(LOG_INFO,
                 "ERROR: filesystem cannot expose a flash pointer; the "
                 "module would have to be copied to RAM\n");
          return EXIT_FAILURE;
        }
    }

  syslog(LOG_INFO, "\nspawning two concurrent instances...\n\n");

  for (i = 0; i < 2; i++)
    {
      snprintf(seed, sizeof(seed), "%d", i + 1);
      args[0] = (FAR char *)"qsorter";
      args[1] = seed;
      args[2] = NULL;

      ret = posix_spawn(&pid[i], MOUNTPT "/qsorter", NULL, NULL, args, NULL);
      if (ret != 0)
        {
          syslog(LOG_INFO, "spawn %d failed: %d\n", i + 1, ret);
          return EXIT_FAILURE;
        }
    }

  usleep(150000);

  if (extent_info_path(MOUNTPT "/qsorter", &info) == 0)
    {
      syslog(LOG_INFO, "[while running] pins on the shared text = %lu\n\n",
             (unsigned long)info.pincount);
    }

  for (i = 0; i < 2; i++)
    {
      waitpid(pid[i], &status, 0);
    }

  usleep(100000);

  if (extent_info_path(MOUNTPT "/qsorter", &info) == 0)
    {
      syslog(LOG_INFO, "\n[after exit]    pins on the shared text = %lu\n",
             (unsigned long)info.pincount);
    }

  syslog(LOG_INFO, "\n=== done ===\n");
  return EXIT_SUCCESS;
}

static int solib_main(void)
{
  struct xipfs_extent_info_s li;
  struct xipfs_extent_info_s ui;
  FAR char *args[3];
  pid_t pid[2];
  char seed[8];
  int status;
  int ret;
  int i;

  syslog(LOG_INFO, "\n=== FDPIC shared library, executed in place ===\n\n");

  ret = stage_blob(MOUNTPT "/libcounter.so", g_libcounter, g_libcounter_len);
  if (ret < 0)
    {
      syslog(LOG_INFO, "staging the library failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  ret = stage_blob(MOUNTPT "/user", g_user, g_user_len);
  if (ret < 0)
    {
      syslog(LOG_INFO, "staging the module failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  syslog(LOG_INFO, "staged libcounter.so (%u bytes) and user (%u bytes)\n",
         g_libcounter_len, g_user_len);

  if (extent_info_path(MOUNTPT "/libcounter.so", &li) == 0 &&
      extent_info_path(MOUNTPT "/user", &ui) == 0)
    {
      syslog(LOG_INFO, "  libcounter.so text at 0x%08lx\n",
             (unsigned long)li.xipaddr);
      syslog(LOG_INFO, "  user          text at 0x%08lx\n",
             (unsigned long)ui.xipaddr);
    }

  syslog(LOG_INFO, "\nspawning two instances, each bumping by its own"
         " seed...\n\n");

  for (i = 0; i < 2; i++)
    {
      snprintf(seed, sizeof(seed), "%d", i + 1);
      args[0] = (FAR char *)"user";
      args[1] = seed;
      args[2] = NULL;

      ret = posix_spawn(&pid[i], MOUNTPT "/user", NULL, NULL, args, NULL);
      if (ret != 0)
        {
          syslog(LOG_INFO, "spawn %d failed: %d\n", i + 1, ret);
          return EXIT_FAILURE;
        }
    }

  usleep(150000);

  if (extent_info_path(MOUNTPT "/libcounter.so", &li) == 0)
    {
      syslog(LOG_INFO, "[while running] pins on the library's shared text"
             " = %lu\n\n", (unsigned long)li.pincount);
    }

  for (i = 0; i < 2; i++)
    {
      waitpid(pid[i], &status, 0);
    }

  usleep(100000);

  if (extent_info_path(MOUNTPT "/libcounter.so", &li) == 0)
    {
      syslog(LOG_INFO, "\n[after exit]    pins on the library's shared text"
             " = %lu\n", (unsigned long)li.pincount);
    }

  syslog(LOG_INFO, "\n=== done ===\n");
  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: cxx_main
 *
 * Description:
 *   The same shape as solib_main(), in C++, and with one thing extra to
 *   prove: that global constructors ran.
 *
 *   That is worth a demo of its own because of how it fails.  A loader that
 *   ignores DT_INIT_ARRAY still loads a C++ module, still resolves every
 *   symbol, and still runs it -- the globals are simply left as .bss and
 *   every field reads zero.  Nothing faults and nothing is logged; the
 *   module just answers wrong.  So both objects here carry a magic number
 *   that only a constructor writes, and the module reports whether it found
 *   them.
 *
 *   The ordering is checked too.  The module's constructor samples whether
 *   the library was already constructed when it ran, which is the only
 *   moment the answer is still interesting -- by the time main() is entered
 *   both have run either way.
 *
 ****************************************************************************/

static int cxx_main(void)
{
  struct xipfs_extent_info_s li;
  struct xipfs_extent_info_s ui;
  FAR char *args[3];
  pid_t pid[2];
  char seed[8];
  int status;
  int ret;
  int i;

  syslog(LOG_INFO, "\n=== C++ module and shared library, executed in"
         " place ===\n\n");

  ret = stage_blob(MOUNTPT "/libshape.so", g_libshape, g_libshape_len);
  if (ret < 0)
    {
      syslog(LOG_INFO, "staging the library failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  ret = stage_blob(MOUNTPT "/cxxuser", g_cxxuser, g_cxxuser_len);
  if (ret < 0)
    {
      syslog(LOG_INFO, "staging the module failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  syslog(LOG_INFO, "staged libshape.so (%u bytes) and cxxuser (%u bytes)\n",
         g_libshape_len, g_cxxuser_len);

  if (extent_info_path(MOUNTPT "/libshape.so", &li) == 0 &&
      extent_info_path(MOUNTPT "/cxxuser", &ui) == 0)
    {
      syslog(LOG_INFO, "  libshape.so text at 0x%08lx\n",
             (unsigned long)li.xipaddr);
      syslog(LOG_INFO, "  cxxuser     text at 0x%08lx\n",
             (unsigned long)ui.xipaddr);
    }

  syslog(LOG_INFO, "\nspawning two instances, each adding its own seed...\n"
         "(each instance's constructors run at load time, before main)\n\n");

  for (i = 0; i < 2; i++)
    {
      snprintf(seed, sizeof(seed), "%d", i + 1);
      args[0] = (FAR char *)"cxxuser";
      args[1] = seed;
      args[2] = NULL;

      ret = posix_spawn(&pid[i], MOUNTPT "/cxxuser", NULL, NULL, args, NULL);
      if (ret != 0)
        {
          syslog(LOG_INFO, "spawn %d failed: %d\n", i + 1, ret);
          return EXIT_FAILURE;
        }
    }

  usleep(150000);

  if (extent_info_path(MOUNTPT "/libshape.so", &li) == 0)
    {
      syslog(LOG_INFO, "[while running] pins on the library's shared text"
             " = %lu\n\n", (unsigned long)li.pincount);
    }

  /* Only a barrier.  The exit status is deliberately not read: waitpid()
   * retains it for an already-exited child only under
   * CONFIG_SCHED_CHILD_STATUS, and both instances run for the same time, so
   * the second is always gone by the time the first wait returns.  Checking
   * `status` after a failed wait reads whatever the previous iteration left
   * there, which looks like a pass.  The suite in apps/testing/fs/xipfs
   * asserts the real result via files; this demo reports through the log.
   */

  ret = EXIT_SUCCESS;

  for (i = 0; i < 2; i++)
    {
      waitpid(pid[i], &status, 0);
    }

  /* The destructors run on unload, which happens as each task is reaped --
   * after waitpid() has returned, so give them a moment to be logged.
   */

  usleep(100000);

  if (extent_info_path(MOUNTPT "/libshape.so", &li) == 0)
    {
      syslog(LOG_INFO, "\n[after exit]    pins on the library's shared text"
             " = %lu\n", (unsigned long)li.pincount);
    }

  syslog(LOG_INFO, "\n=== %s ===\n",
         ret == EXIT_SUCCESS ? "done" : "FAILED");
  return ret;
}

/****************************************************************************
 * Name: jmprel_main
 *
 * Description:
 *   Load a module whose imports the linker put in DT_JMPREL.
 *
 *   Every other module here is linked with -z now, which leaves the imported
 *   descriptors in DT_REL.  This one empties BINDNOW in modules/Makefile, so
 *   it has no DT_REL at all -- both of its imports are in the PLT relocation
 *   table.
 *
 *   There is no lazy resolver in the loader, so a DT_JMPREL table that went
 *   unwalked was not deferred work: the descriptors stayed unrelocated and
 *   the module's first call into the firmware branched to whatever the
 *   unrelocated word held.  It loaded cleanly and printed nothing, then
 *   took an INVSTATE UsageFault that escalated to a HardFault -- no
 *   console, no crash dump.  If this demo prints its line, both tables are
 *   being bound.
 *
 ****************************************************************************/

static int jmprel_main(void)
{
  FAR char *args[3];
  pid_t pid;
  int status;
  int ret;

  syslog(LOG_INFO, "\n=== FDPIC module bound through DT_JMPREL ===\n\n");

  ret = stage_blob(MOUNTPT "/lazymod", g_lazymod, g_lazymod_len);
  if (ret < 0)
    {
      syslog(LOG_INFO, "staging the module failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  syslog(LOG_INFO, "staged lazymod (%u bytes), all imports in DT_JMPREL\n\n",
         g_lazymod_len);

  args[0] = (FAR char *)"lazymod";
  args[1] = (FAR char *)"42";
  args[2] = NULL;

  ret = posix_spawn(&pid, MOUNTPT "/lazymod", NULL, NULL, args, NULL);
  if (ret != 0)
    {
      syslog(LOG_INFO, "spawn failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  waitpid(pid, &status, 0);

  if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)
    {
      syslog(LOG_INFO, "\n=== FAILED: module did not exit cleanly ===\n");
      return EXIT_FAILURE;
    }

  syslog(LOG_INFO, "\n=== done: the module reached the firmware and came"
         " back ===\n");
  return EXIT_SUCCESS;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  if (argc > 1 && strcmp(argv[1], "solib") == 0)
    {
      return solib_main();
    }

  if (argc > 1 && strcmp(argv[1], "cxx") == 0)
    {
      return cxx_main();
    }

  if (argc > 1 && strcmp(argv[1], "jmprel") == 0)
    {
      return jmprel_main();
    }

  if (argc > 1 && strcmp(argv[1], "qsort") != 0)
    {
      fprintf(stderr, "usage: fdpicxip [qsort|solib|cxx|jmprel]\n");
      return EXIT_FAILURE;
    }

  return qsorter_main();
}
