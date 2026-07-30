/****************************************************************************
 * apps/testing/fs/xipfs/xipfs_main.c
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
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/fs/ioctl.h>
#include <nuttx/fs/xipfs.h>

#ifdef CONFIG_FDPIC
#  include <elf.h>
#  include <spawn.h>

#  include "libshape_bin.h"
#  include "cxxuser_bin.h"
#  include "funcdesc_bin.h"
#  include "lazymod_bin.h"
#  include "libcounter_bin.h"
#  include "counteruser_bin.h"
#  include "callback_bin.h"
#  include "missingsym_bin.h"
#  include "manyneeded_bin.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MOUNTPT   CONFIG_TESTING_FS_XIPFS_MOUNTPT
#define MTDDEV    CONFIG_TESTING_FS_XIPFS_MTD

#define PATH(name) MOUNTPT "/" name

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_passed;
static int g_failed;
static FAR uint8_t *g_buffer;
static size_t g_bufsize = 16384;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void report(FAR const char *name, bool ok, FAR const char *detail)
{
  if (ok)
    {
      g_passed++;
      printf("  PASS  %s\n", name);
    }
  else
    {
      g_failed++;
      printf("  FAIL  %s: %s\n", name, detail ? detail : "");
    }
}

#define CHECK(name, cond, detail) report((name), (cond), (detail))

/****************************************************************************
 * Name: remount
 *
 * Description:
 *   Discard every scrap of in-RAM filesystem state and mount again from the
 *   medium.  Because the rammtd buffer survives, this is exactly what a
 *   reboot looks like to the filesystem, which is what makes the power loss
 *   sweep below meaningful without restarting the simulator.
 *
 ****************************************************************************/

static int remount(void)
{
  int ret;

  ret = umount(MOUNTPT);
  if (ret < 0 && errno != ENOENT && errno != EINVAL)
    {
      printf("    umount failed: %d\n", errno);
      return -errno;
    }

  ret = mount(MTDDEV, MOUNTPT, "xipfs", 0, NULL);
  if (ret < 0)
    {
      return -errno;
    }

  return 0;
}

/****************************************************************************
 * Name: fill_pattern
 ****************************************************************************/

static void fill_pattern(FAR uint8_t *buf, size_t len, uint8_t seed)
{
  size_t i;

  for (i = 0; i < len; i++)
    {
      buf[i] = (uint8_t)(seed + (i * 7));
    }
}

/****************************************************************************
 * Name: create_file
 *
 * Description:
 *   Create a file of a known size.  ftruncate is how the exact size is
 *   declared up front, which is what lets the filesystem reserve the exact
 *   contiguous extent rather than over-reserving and trimming.
 *
 ****************************************************************************/

static int create_file(FAR const char *path, size_t len, uint8_t seed)
{
  ssize_t nwritten;
  int fd;
  int ret = 0;

  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd < 0)
    {
      return -errno;
    }

  if (ftruncate(fd, len) < 0)
    {
      ret = -errno;
      goto out;
    }

  fill_pattern(g_buffer, len, seed);

  nwritten = write(fd, g_buffer, len);
  if (nwritten != (ssize_t)len)
    {
      ret = nwritten < 0 ? -errno : -EIO;
    }

out:
  if (close(fd) < 0 && ret == 0)
    {
      ret = -errno;
    }

  return ret;
}

/****************************************************************************
 * Name: verify_file
 ****************************************************************************/

static bool verify_file(FAR const char *path, size_t len, uint8_t seed)
{
  FAR uint8_t *expect;
  ssize_t nread;
  bool ok = false;
  int fd;

  expect = malloc(len);
  if (expect == NULL)
    {
      return false;
    }

  fill_pattern(expect, len, seed);

  fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      free(expect);
      return false;
    }

  nread = read(fd, g_buffer, len + 1);
  if (nread == (ssize_t)len && memcmp(g_buffer, expect, len) == 0)
    {
      ok = true;
    }

  close(fd);
  free(expect);
  return ok;
}

/****************************************************************************
 * Name: extent_info
 ****************************************************************************/

static int extent_info(FAR const char *path,
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
  if (ret < 0)
    {
      ret = -errno;
    }

  close(fd);
  return ret;
}

/****************************************************************************
 * Name: run_defrag
 ****************************************************************************/

/* Defragmentation acts on the volume, so it is issued on a descriptor for
 * the mountpoint directory rather than for a file inside it.  That matters
 * beyond tidiness: a descriptor for a file inside the volume would hold
 * that file open, and an open extent cannot be relocated, so the pass would
 * be obstructed by the very act of asking for it.
 */

static int run_defrag(uint32_t max_ms,
                      FAR struct xipfs_defrag_result_s *result)
{
  struct xipfs_defrag_arg_s arg;
  int fd;
  int ret;

  memset(&arg, 0, sizeof(arg));
  arg.max_ms = max_ms;

  fd = open(MOUNTPT, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (fd < 0)
    {
      return -errno;
    }

  ret = ioctl(fd, XIPFSIOC_DEFRAG, (unsigned long)(uintptr_t)&arg);
  if (ret < 0)
    {
      ret = -errno;
    }
  else
    {
      *result = arg.result;
    }

  close(fd);
  return ret;
}

/****************************************************************************
 * Name: arm_injector
 *
 * Description:
 *   Arm the fault injector to fail the n'th flash write or erase from now
 *   on, and every one after it.  'mode' chooses whether the failing
 *   operation leaves the medium untouched (XIPFS_FAULT_CLEAN) or half-done,
 *   the way a real cut does (XIPFS_FAULT_TORN).  It is a volume command, so
 *   it goes through the mountpoint directory like run_defrag, which also
 *   means it can be armed on a volume with no files in it.
 *
 ****************************************************************************/

static int arm_injector(int countdown, uint8_t mode)
{
  struct xipfs_fault_s fault;
  int fd;
  int ret;

  fault.count = countdown;
  fault.mode  = mode;

  fd = open(MOUNTPT, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (fd < 0)
    {
      return -errno;
    }

  ret = ioctl(fd, XIPFSIOC_FAULTINJECT, (unsigned long)(uintptr_t)&fault);
  if (ret < 0)
    {
      ret = -errno;
    }

  close(fd);
  return ret;
}

/****************************************************************************
 * Test: basic VFS operations and the contiguity invariant
 ****************************************************************************/

static void test_basic(void)
{
  struct xipfs_extent_info_s info;
  struct stat st;
  FAR DIR *dirp;
  FAR struct dirent *de;
  int found = 0;
  int ret;

  printf("\n-- basic VFS --\n");

  ret = create_file(PATH("basic1.bin"), 100, 0x11);
  CHECK("create small file", ret == 0, strerror(-ret));

  ret = create_file(PATH("basic2.bin"), 9000, 0x22);
  CHECK("create multi-block file", ret == 0, strerror(-ret));

  CHECK("read back small", verify_file(PATH("basic1.bin"), 100, 0x11), "");
  CHECK("read back multi-block",
        verify_file(PATH("basic2.bin"), 9000, 0x22), "");

  ret = stat(PATH("basic2.bin"), &st);
  CHECK("stat size", ret == 0 && st.st_size == 9000, "wrong size");

  /* The core invariant: one contiguous, erase-block aligned extent whose
   * block count is exactly what the file size requires.
   */

  ret = extent_info(PATH("basic2.bin"), &info);
  CHECK("extent is contiguous and exactly sized",
        ret == 0 && info.nblocks ==
        (info.size + info.erasesize - 1) / info.erasesize,
        "extent size mismatch");

  dirp = opendir(MOUNTPT);
  if (dirp != NULL)
    {
      while ((de = readdir(dirp)) != NULL)
        {
          if (strcmp(de->d_name, "basic1.bin") == 0 ||
              strcmp(de->d_name, "basic2.bin") == 0)
            {
              found++;
            }
        }

      closedir(dirp);
    }

  CHECK("readdir lists both files", found == 2, "missing entries");

  ret = unlink(PATH("basic1.bin"));
  CHECK("unlink", ret == 0, strerror(errno));
  CHECK("unlinked file is gone", stat(PATH("basic1.bin"), &st) < 0, "");

  /* Survives a remount: the directory is on the medium, not in RAM */

  ret = remount();
  CHECK("remount", ret == 0, strerror(-ret));
  CHECK("data survives remount",
        verify_file(PATH("basic2.bin"), 9000, 0x22), "");
  CHECK("delete survives remount",
        stat(PATH("basic1.bin"), &st) < 0, "");

  unlink(PATH("basic2.bin"));
}

/****************************************************************************
 * Test: execute-in-place mapping
 *
 * This is the test that proves the entire premise.  A mapping must land
 * inside the flash window and must not cost RAM proportional to the file.
 ****************************************************************************/

static void test_xip(void)
{
  struct xipfs_extent_info_s info;
  struct mallinfo before;
  struct mallinfo after;
  const size_t len = 12288;
  FAR void *addr;
  FAR uint8_t *expect;
  int fd;
  int ret;

  printf("\n-- XIP mapping --\n");

  ret = create_file(PATH("xip.bin"), len, 0x33);
  CHECK("create module image", ret == 0, strerror(-ret));

  ret = extent_info(PATH("xip.bin"), &info);
  CHECK("media is memory mapped", ret == 0 && info.xipaddr != 0,
        "no XIP base");

  if (info.xipaddr == 0)
    {
      return;
    }

  fd = open(PATH("xip.bin"), O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      CHECK("open for mapping", false, strerror(errno));
      return;
    }

  before = mallinfo();
  addr = mmap(NULL, len, PROT_READ | PROT_EXEC,
              MAP_SHARED | MAP_XIP_STRICT, fd, 0);
  after = mallinfo();

  CHECK("strict XIP mmap succeeds", addr != MAP_FAILED, strerror(errno));

  if (addr != MAP_FAILED)
    {
      /* The returned pointer must BE the flash address, not a copy of it */

      CHECK("mapping points into flash",
            (uintptr_t)addr == info.xipaddr, "not the extent address");

      expect = malloc(len);
      if (expect != NULL)
        {
          fill_pattern(expect, len, 0x33);
          CHECK("mapped bytes match file contents",
                memcmp(addr, expect, len) == 0, "content mismatch");
          free(expect);
        }

      /* A RAM copy would have cost at least the file size.  Allow slack
       * for the mapping bookkeeping itself.
       */

      CHECK("mapping does not cost RAM proportional to the file",
            (after.uordblks - before.uordblks) < (int)(len / 2),
            "RAM grew by about the file size");

      ret = extent_info(PATH("xip.bin"), &info);
      CHECK("mapping takes a pin", ret == 0 && info.pincount == 1,
            "pin not taken");

      ret = munmap(addr, len);
      CHECK("munmap", ret == 0, strerror(errno));

      ret = extent_info(PATH("xip.bin"), &info);
      CHECK("munmap drops the pin", ret == 0 && info.pincount == 0,
            "pin not released");
    }

  close(fd);
  unlink(PATH("xip.bin"));
}

/****************************************************************************
 * Test: multiple concurrent mappings share one pin count
 ****************************************************************************/

static void test_multimap(void)
{
  struct xipfs_extent_info_s info;
  const size_t len = 8192;
  FAR void *addr[3];
  int fd[3];
  int i;
  int ret;

  printf("\n-- concurrent mappings --\n");

  ret = create_file(PATH("multi.bin"), len, 0x44);
  CHECK("create", ret == 0, strerror(-ret));

  for (i = 0; i < 3; i++)
    {
      fd[i] = open(PATH("multi.bin"), O_RDONLY | O_CLOEXEC);
      addr[i] = mmap(NULL, len, PROT_READ, MAP_SHARED | MAP_XIP_STRICT,
                     fd[i], 0);
    }

  ret = extent_info(PATH("multi.bin"), &info);
  CHECK("three mappings give a pin count of three",
        ret == 0 && info.pincount == 3, "wrong pin count");

  /* All three must resolve to the same flash address: they alias the same
   * extent rather than each getting a private copy.
   */

  CHECK("mappings alias the same flash address",
        addr[0] == addr[1] && addr[1] == addr[2], "addresses differ");

  /* Within one task, all three mappings share an address, so a single
   * munmap of that address releases all of them.  That is munmap's range
   * semantics -- the VFS unmaps every mapping the range covers -- and it
   * only arises because true XIP makes the addresses identical.  It is not
   * the multi-instance case: separate module instances are separate tasks
   * with separate mapping lists, which test_crosstask_pins covers.
   */

  munmap(addr[0], len);
  ret = extent_info(PATH("multi.bin"), &info);
  CHECK("unmapping an aliased address releases that task's mappings",
        ret == 0 && info.pincount == 0, "pin leaked");

  for (i = 0; i < 3; i++)
    {
      close(fd[i]);
    }

  unlink(PATH("multi.bin"));
}

/****************************************************************************
 * Test: pins are refcounted across tasks
 *
 * This is the multi-instance case that matters: two tasks each holding a
 * live XIP mapping of one module.  One of them dying must not release the
 * other's pin, or the surviving instance would be executing out of blocks
 * the defragmenter now considers movable.
 ****************************************************************************/

static int holder_task(int argc, FAR char *argv[])
{
  const size_t len = 8192;
  FAR void *addr;
  int fd;

  fd = open(PATH("shared.bin"), O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  addr = mmap(NULL, len, PROT_READ, MAP_SHARED | MAP_XIP_STRICT, fd, 0);
  close(fd);

  if (addr == MAP_FAILED)
    {
      return EXIT_FAILURE;
    }

  /* Exit holding the mapping */

  return EXIT_SUCCESS;
}

static void test_crosstask_pins(void)
{
  struct xipfs_extent_info_s info;
  const size_t len = 8192;
  FAR void *addr;
  int status;
  pid_t pid;
  int fd;
  int ret;

  printf("\n-- cross-task pin refcounting --\n");

  ret = create_file(PATH("shared.bin"), len, 0xaa);
  CHECK("create", ret == 0, strerror(-ret));

  fd = open(PATH("shared.bin"), O_RDONLY | O_CLOEXEC);
  addr = mmap(NULL, len, PROT_READ, MAP_SHARED | MAP_XIP_STRICT, fd, 0);
  CHECK("parent maps", addr != MAP_FAILED, strerror(errno));

  pid = task_create("holder", SCHED_PRIORITY_DEFAULT, 4096, holder_task,
                    NULL);
  if (pid > 0)
    {
      waitpid(pid, &status, 0);
      usleep(100000);

      /* The child's teardown must have dropped exactly its own pin */

      ret = extent_info(PATH("shared.bin"), &info);
      CHECK("a dying task does not release another task's pin",
            ret == 0 && info.pincount == 1, "wrong pin count");
    }

  if (addr != MAP_FAILED)
    {
      munmap(addr, len);
    }

  ret = extent_info(PATH("shared.bin"), &info);
  CHECK("last holder releases the extent",
        ret == 0 && info.pincount == 0, "pin leaked");

  close(fd);
  unlink(PATH("shared.bin"));
}

/****************************************************************************
 * Test: a task that dies without unmapping must still release its pin
 ****************************************************************************/

static int leaky_mapper(int argc, FAR char *argv[])
{
  const size_t len = 8192;
  FAR void *addr;
  int fd;

  fd = open(PATH("leak.bin"), O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  addr = mmap(NULL, len, PROT_READ, MAP_SHARED | MAP_XIP_STRICT, fd, 0);
  if (addr == MAP_FAILED)
    {
      close(fd);
      return EXIT_FAILURE;
    }

  /* Deliberately exit with the mapping still live and no munmap call.  A
   * module that faults or is killed does exactly this, and the pin has to
   * come back anyway or the extent is immovable forever.
   */

  close(fd);
  return EXIT_SUCCESS;
}

static void test_teardown_release(void)
{
  struct xipfs_extent_info_s info;
  int status;
  pid_t pid;
  int ret;

  printf("\n-- pin release on task exit --\n");

  ret = create_file(PATH("leak.bin"), 8192, 0x55);
  CHECK("create", ret == 0, strerror(-ret));

  pid = task_create("leaky", SCHED_PRIORITY_DEFAULT, 4096, leaky_mapper,
                    NULL);
  CHECK("spawn mapper task", pid > 0, strerror(errno));

  if (pid > 0)
    {
      waitpid(pid, &status, 0);

      /* Give the exiting task's group teardown a chance to run */

      usleep(100000);

      ret = extent_info(PATH("leak.bin"), &info);
      CHECK("pin released although the task never called munmap",
            ret == 0 && info.pincount == 0, "pin leaked on task exit");
    }

  unlink(PATH("leak.bin"));
}

/****************************************************************************
 * Test: directories
 *
 * Directories are records in the metadata generation, not names with
 * separators in them, so what is checked is that they behave like a real
 * tree: they can be empty, they survive a remount empty, rmdir refuses a
 * directory that still holds something, and a path through a file or a
 * missing directory fails the way POSIX says rather than springing into
 * existence.
 ****************************************************************************/

static void test_dirs(void)
{
  struct stat st;
  FAR struct dirent *de;
  FAR DIR *dirp;
  int binseen = 0;
  int etcseen = 0;
  bool sawtop = false;
  bool sawhello = false;
  bool sawworld = false;
  bool badentry = false;
  int fd;
  int ret;

  printf("\n-- directories --\n");

  ret = mkdir(PATH("bin"), 0755);
  CHECK("mkdir", ret == 0, strerror(errno));

  ret = mkdir(PATH("bin"), 0755);
  CHECK("mkdir of an existing directory is refused",
        ret < 0 && errno == EEXIST, strerror(errno));

  /* An empty directory is a real thing here, and it has to survive a
   * remount with nothing inside it.  This is the whole difference from
   * synthesising directories out of file names.
   */

  ret = stat(PATH("bin"), &st);
  CHECK("an empty directory exists", ret == 0 && S_ISDIR(st.st_mode),
        strerror(errno));

  ret = remount();
  CHECK("remount", ret == 0, strerror(-ret));

  ret = stat(PATH("bin"), &st);
  CHECK("an empty directory survives a remount",
        ret == 0 && S_ISDIR(st.st_mode), strerror(errno));

  /* Files below it, and a directory two levels down */

  ret = create_file(PATH("bin/hello"), 100, 1);
  CHECK("create a file in a directory", ret == 0, strerror(-ret));

  ret = create_file(PATH("bin/world"), 100, 2);
  CHECK("create a second one beside it", ret == 0, strerror(-ret));

  ret = mkdir(PATH("etc"), 0755);
  ret |= mkdir(PATH("etc/conf"), 0755);
  CHECK("mkdir two levels down", ret == 0, strerror(errno));

  ret = create_file(PATH("etc/conf/net"), 100, 3);
  CHECK("create a file two levels down", ret == 0, strerror(-ret));

  ret = create_file(PATH("top.bin"), 100, 4);
  CHECK("create one at the root", ret == 0, strerror(-ret));

  /* A path through a directory that does not exist is an error, not an
   * invitation to create it.
   */

  fd = open(PATH("nodir/x"), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  CHECK("a path through a missing directory fails",
        fd < 0 && errno == ENOENT, strerror(errno));
  if (fd >= 0)
    {
      close(fd);
    }

  fd = open(PATH("top.bin/deeper"),
            O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  CHECK("a path through a file fails", fd < 0 && errno == ENOTDIR,
        strerror(errno));
  if (fd >= 0)
    {
      close(fd);
    }

  /* Listing: the root holds two directories and a file, once each */

  dirp = opendir(MOUNTPT);
  while (dirp != NULL && (de = readdir(dirp)) != NULL)
    {
      if (strcmp(de->d_name, "bin") == 0 && de->d_type == DTYPE_DIRECTORY)
        {
          binseen++;
        }
      else if (strcmp(de->d_name, "etc") == 0 &&
               de->d_type == DTYPE_DIRECTORY)
        {
          etcseen++;
        }
      else if (strcmp(de->d_name, "top.bin") == 0 &&
               de->d_type == DTYPE_FILE)
        {
          sawtop = true;
        }
    }

  if (dirp != NULL)
    {
      closedir(dirp);
    }

  CHECK("the root lists its directories and its files",
        binseen == 1 && etcseen == 1 && sawtop, "missing entry");

  dirp = opendir(PATH("bin"));
  CHECK("opendir on a subdirectory", dirp != NULL, strerror(errno));

  while (dirp != NULL && (de = readdir(dirp)) != NULL)
    {
      if (strchr(de->d_name, '/') != NULL || de->d_type != DTYPE_FILE)
        {
          badentry = true;
        }
      else if (strcmp(de->d_name, "hello") == 0)
        {
          sawhello = true;
        }
      else if (strcmp(de->d_name, "world") == 0)
        {
          sawworld = true;
        }
    }

  if (dirp != NULL)
    {
      closedir(dirp);
    }

  CHECK("it lists its own entries, by name",
        sawhello && sawworld && !badentry, "unexpected entries");

  ret = stat(PATH("bin/hello"), &st);
  CHECK("stat of a file in a directory",
        ret == 0 && S_ISREG(st.st_mode) && st.st_size == 100,
        strerror(errno));

  /* The VFS passes a trailing separator through, and it names the same
   * directory.
   */

  dirp = opendir(PATH("etc/conf/"));
  ret = stat(PATH("etc/conf/"), &st);
  CHECK("a trailing separator names the same directory",
        dirp != NULL && ret == 0 && S_ISDIR(st.st_mode), strerror(errno));

  if (dirp != NULL)
    {
      closedir(dirp);
    }

  /* "." and ".." are refused rather than interpreted, because an entry by
   * either name could never be reached again.
   */

  ret = mkdir(PATH("etc/.."), 0755);
  CHECK("a dot-dot component is refused", ret < 0 && errno == EINVAL,
        strerror(errno));

  /* The type rules, in both directions */

  /* A read-only open of a directory is not an error: the VFS turns the
   * EISDIR the file system answers into a directory descriptor, which is
   * what makes fdopendir and the volume ioctls work.  Opening one for
   * writing has nowhere to go, so that is where EISDIR surfaces.
   */

  fd = open(PATH("bin"), O_RDONLY | O_CLOEXEC);
  ret = fd >= 0 ? fstat(fd, &st) : -1;
  CHECK("a read-only open of a directory yields a directory",
        fd >= 0 && ret == 0 && S_ISDIR(st.st_mode), strerror(errno));
  if (fd >= 0)
    {
      close(fd);
    }

  fd = open(PATH("bin"), O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
  CHECK("opening a directory for writing is refused",
        fd < 0 && errno == EISDIR, strerror(errno));
  if (fd >= 0)
    {
      close(fd);
    }

  ret = unlink(PATH("bin"));
  CHECK("unlinking a directory is refused", ret < 0 && errno == EISDIR,
        strerror(errno));

  ret = rmdir(PATH("top.bin"));
  CHECK("rmdir of a file is refused", ret < 0 && errno == ENOTDIR,
        strerror(errno));

  ret = rmdir(PATH("bin"));
  CHECK("rmdir of a directory with contents is refused",
        ret < 0 && errno == ENOTEMPTY, strerror(errno));

  /* Emptying it makes it removable, and removing it takes nothing else */

  unlink(PATH("bin/hello"));
  unlink(PATH("bin/world"));

  ret = rmdir(PATH("bin"));
  CHECK("rmdir of an emptied directory", ret == 0, strerror(errno));

  ret = stat(PATH("bin"), &st);
  CHECK("it is gone", ret < 0 && errno == ENOENT, strerror(errno));

  ret = stat(PATH("etc/conf/net"), &st);
  CHECK("removing one directory leaves the others alone",
        ret == 0 && S_ISREG(st.st_mode), strerror(errno));

  /* And the whole tree is durable */

  ret = remount();
  CHECK("remount after all of it", ret == 0, strerror(-ret));

  ret = stat(PATH("etc/conf"), &st);
  CHECK("the tree is the same after a remount",
        ret == 0 && S_ISDIR(st.st_mode) &&
        verify_file(PATH("etc/conf/net"), 100, 3), "tree changed");

  unlink(PATH("etc/conf/net"));
  rmdir(PATH("etc/conf"));
  rmdir(PATH("etc"));
  unlink(PATH("top.bin"));
}

/****************************************************************************
 * Test: defragmentation
 ****************************************************************************/

static void test_defrag(void)
{
  struct xipfs_defrag_result_s result;
  struct xipfs_extent_info_s info;
  const size_t small = 4096;
  size_t largest_before;
  FAR void *addr;
  int fd;
  int ret;
  int i;
  char path[64];

  printf("\n-- defragmentation --\n");

  /* Lay down an alternating pattern, then punch out every other file.  The
   * result is free space that is plentiful in total but useless in any
   * single run, which is precisely the situation defrag exists for.
   */

  for (i = 0; i < 10; i++)
    {
      snprintf(path, sizeof(path), PATH("frag%d.bin"), i);
      if (create_file(path, small, (uint8_t)i) < 0)
        {
          break;
        }
    }

  for (i = 0; i < 10; i += 2)
    {
      snprintf(path, sizeof(path), PATH("frag%d.bin"), i);
      unlink(path);
    }

  ret = run_defrag(0, &result);
  CHECK("defrag runs", ret == 0, strerror(-ret));
  largest_before = result.largest_free_run;

  /* Nothing is open, because the pass was asked for through the mountpoint
   * directory, so it should have run to completion rather than reporting an
   * obstruction.
   */

  CHECK("defrag through the mountpoint directory completes",
        result.reason == XIPFS_DEFRAG_DONE, "unexpected reason");

  /* Surviving files must be byte-identical after being physically moved */

  for (i = 1; i < 10; i += 2)
    {
      snprintf(path, sizeof(path), PATH("frag%d.bin"), i);
      if (!verify_file(path, small, (uint8_t)i))
        {
          break;
        }
    }

  CHECK("relocated files keep their contents", i >= 10, "content changed");

  /* Everything must still be there after a remount: each relocation
   * committed a new generation naming the new location.
   */

  ret = remount();
  CHECK("remount after defrag", ret == 0, strerror(-ret));

  for (i = 1; i < 10; i += 2)
    {
      snprintf(path, sizeof(path), PATH("frag%d.bin"), i);
      if (!verify_file(path, small, (uint8_t)i))
        {
          break;
        }
    }

  CHECK("relocations survive remount", i >= 10, "content lost");

  /* An open file is still an obstruction, and must be reported as one.
   * The pass only stops on it when the open extent is the only candidate
   * for the hole it is trying to fill, because otherwise it just moves
   * something else instead.  So arrange exactly that: punch out the
   * second-highest file and hold open the highest, which is then the only
   * extent above the hole.
   */

    {
      uint32_t beststart = 0;
      uint32_t nextstart = 0;
      char lastpath[64];
      char holepath[64];

      lastpath[0] = '\0';
      holepath[0] = '\0';

      for (i = 1; i < 10; i += 2)
        {
          snprintf(path, sizeof(path), PATH("frag%d.bin"), i);
          if (extent_info(path, &info) < 0)
            {
              continue;
            }

          if (lastpath[0] == '\0' || info.start_block > beststart)
            {
              nextstart = beststart;
              strlcpy(holepath, lastpath, sizeof(holepath));
              beststart = info.start_block;
              strlcpy(lastpath, path, sizeof(lastpath));
            }
          else if (holepath[0] == '\0' || info.start_block > nextstart)
            {
              nextstart = info.start_block;
              strlcpy(holepath, path, sizeof(holepath));
            }
        }

      if (lastpath[0] != '\0' && holepath[0] != '\0')
        {
          unlink(holepath);

          fd = open(lastpath, O_RDONLY | O_CLOEXEC);
          if (fd >= 0)
            {
              ret = run_defrag(0, &result);
              CHECK("an open file is reported as blocking, not as done",
                    ret == 0 &&
                    result.reason == XIPFS_DEFRAG_BLOCKED_OPEN,
                    "unexpected reason");
              close(fd);
            }

          /* Put it back so the pinned test below has the same layout to
           * work with as before.
           */

          create_file(holepath, small, 1);
        }
    }

  /* A pinned extent must be skipped, and defrag must say so rather than
   * silently reporting success.
   */

  snprintf(path, sizeof(path), PATH("frag1.bin"));
  fd = open(path, O_RDONLY | O_CLOEXEC);
  addr = mmap(NULL, small, PROT_READ, MAP_SHARED | MAP_XIP_STRICT, fd, 0);

  if (addr != MAP_FAILED)
    {
      ret = extent_info(path, &info);

      /* Fragment again below the pinned extent so defrag has a reason to
       * want to move it.
       */

      for (i = 3; i < 10; i += 2)
        {
          snprintf(path, sizeof(path), PATH("frag%d.bin"), i);
          unlink(path);
        }

      ret = run_defrag(0, &result);
      CHECK("defrag with a pinned extent still returns cleanly", ret == 0,
            strerror(-ret));

      snprintf(path, sizeof(path), PATH("frag1.bin"));
      ret = extent_info(path, &info);
      CHECK("pinned extent was not relocated",
            ret == 0 && (uintptr_t)addr == info.xipaddr,
            "pinned extent moved under a live mapping");

      munmap(addr, small);
      close(fd);
    }

  /* After releasing the pin, a further pass should be able to do at least
   * as well as before.
   */

  ret = run_defrag(0, &result);
  CHECK("defrag after unpinning",
        ret == 0 && result.largest_free_run >= largest_before,
        "free space did not recover");

  for (i = 0; i < 10; i++)
    {
      snprintf(path, sizeof(path), PATH("frag%d.bin"), i);
      unlink(path);
    }
}

/****************************************************************************
 * Test: power loss at every step of a create
 *
 * The medium survives; only the filesystem's RAM state is discarded, which
 * is exactly what a reboot does.  For every flash operation in turn, fail
 * at that operation and assert that what comes back is a consistent
 * filesystem -- never a torn directory and never a half written file.
 *
 * The sweep runs twice.  In XIPFS_FAULT_CLEAN the failing operation leaves
 * the medium untouched.  In XIPFS_FAULT_TORN it leaves it half done -- half
 * a page programmed, half a sector erased -- which is what a real cut leaves
 * and what makes the mount-time CRC actually have to reject a half-formed
 * generation rather than an obviously blank one.  Both must yield the same
 * two invariants, because the commit writes the directory body before the
 * header that validates it: a torn body is never reachable behind a written
 * header.
 ****************************************************************************/

static void powerloss_create(uint8_t mode, FAR const char *modename)
{
  const int max_ops = 16;
  struct stat st;
  bool all_ok = true;
  bool saw_failure = false;
  char detail[80];
  char name[80];
  int ret;
  int n;

  printf("\n-- power loss during create (%s) --\n", modename);

  for (n = 0; n < max_ops; n++)
    {
      ret = remount();
      if (ret < 0)
        {
          snprintf(detail, sizeof(detail),
                   "remount failed before injection %d: %d", n, ret);
          all_ok = false;
          break;
        }

      /* A file that must survive every injected failure untouched */

      unlink(PATH("keep.bin"));
      unlink(PATH("victim.bin"));

      if (create_file(PATH("keep.bin"), 2048, 0x66) < 0)
        {
          snprintf(detail, sizeof(detail), "baseline create failed at %d",
                   n);
          all_ok = false;
          break;
        }

      /* Arm the injector: fail the n'th flash write or erase from now on */

      if (arm_injector(n, mode) < 0)
        {
          all_ok = false;
          strlcpy(detail, "cannot open to arm injector", sizeof(detail));
          break;
        }

      /* This create will be cut short at some arbitrary point */

      ret = create_file(PATH("victim.bin"), 5000, 0x77);
      if (ret < 0)
        {
          saw_failure = true;
        }

      /* Now "reboot" */

      ret = remount();
      if (ret < 0)
        {
          snprintf(detail, sizeof(detail),
                   "mount failed after injection %d: %d", n, ret);
          all_ok = false;
          break;
        }

      /* Invariant 1: the pre-existing file is untouched.  A commit that
       * tore would most likely take the whole directory with it.
       */

      if (!verify_file(PATH("keep.bin"), 2048, 0x66))
        {
          snprintf(detail, sizeof(detail),
                   "pre-existing file corrupted by injection %d", n);
          all_ok = false;
          break;
        }

      /* Invariant 2: the interrupted file is either absent or complete.
       * There is no valid state in between.
       */

      if (stat(PATH("victim.bin"), &st) == 0)
        {
          if (st.st_size != 5000 ||
              !verify_file(PATH("victim.bin"), 5000, 0x77))
            {
              snprintf(detail, sizeof(detail),
                       "partially written file visible after injection %d",
                       n);
              all_ok = false;
              break;
            }
        }
    }

  snprintf(name, sizeof(name),
           "filesystem stays consistent across every injection point (%s)",
           modename);
  CHECK(name, all_ok, detail);

  snprintf(name, sizeof(name),
           "injection actually interrupted some operations (%s)", modename);
  CHECK(name, saw_failure,
        "no operation was ever cut short; the sweep proved nothing");

  remount();
  unlink(PATH("keep.bin"));
  unlink(PATH("victim.bin"));
}

static void test_powerloss(void)
{
  powerloss_create(XIPFS_FAULT_CLEAN, "clean");
}

static void test_powerloss_torn(void)
{
  powerloss_create(XIPFS_FAULT_TORN, "torn");
}

/****************************************************************************
 * Test: power loss at every step of an unlink
 *
 * An unlink is a single metadata commit followed by returning the blocks to
 * the allocator, so the file is either still there in full or entirely gone.
 * What makes it worth sweeping is not the file being deleted but the two
 * files either side of it: if a torn commit were to leave the directory
 * naming an extent whose blocks the allocator has already handed back, the
 * next create would be given live blocks and would quietly overwrite a
 * neighbour.  That is why each iteration ends by allocating again and then
 * re-checking the survivors.
 ****************************************************************************/

static void test_powerloss_unlink(void)
{
  const int max_ops = 16;
  const size_t len = 3000;
  struct stat st;
  bool all_ok = true;
  bool saw_failure = false;
  bool saw_survival = false;
  bool saw_deletion = false;
  bool exhausted = false;
  char detail[80];
  int quiet = 0;
  int ret;
  int n;

  printf("\n-- power loss during unlink --\n");
  detail[0] = '\0';

  for (n = 0; n < max_ops; n++)
    {
      ret = remount();
      if (ret < 0)
        {
          snprintf(detail, sizeof(detail),
                   "remount failed before injection %d: %d", n, ret);
          all_ok = false;
          break;
        }

      unlink(PATH("ul_a.bin"));
      unlink(PATH("ul_b.bin"));
      unlink(PATH("ul_c.bin"));
      unlink(PATH("ul_new.bin"));

      if (create_file(PATH("ul_a.bin"), len, 0xa1) < 0 ||
          create_file(PATH("ul_b.bin"), len, 0xb2) < 0 ||
          create_file(PATH("ul_c.bin"), len, 0xc3) < 0)
        {
          snprintf(detail, sizeof(detail),
                   "baseline create failed at %d", n);
          all_ok = false;
          break;
        }

      if (arm_injector(n, XIPFS_FAULT_CLEAN) < 0)
        {
          strlcpy(detail, "cannot arm injector", sizeof(detail));
          all_ok = false;
          break;
        }

      /* Delete the middle file.  This is the operation being cut short. */

      if (unlink(PATH("ul_b.bin")) < 0)
        {
          saw_failure = true;
          quiet = 0;
        }
      else
        {
          /* The injector was armed further out than this operation reaches.
           * Two of those in a row means the sweep has walked off the end of
           * the unlink and later points would only be testing the creates
           * that follow it.
           */

          if (++quiet >= 2)
            {
              exhausted = true;
            }
        }

      /* Reboot */

      ret = remount();
      if (ret < 0)
        {
          snprintf(detail, sizeof(detail),
                   "mount failed after injection %d: %d", n, ret);
          all_ok = false;
          break;
        }

      /* Invariant 1: the neighbours are untouched.  They were never part of
       * the operation, so nothing about a torn unlink may reach them.
       */

      if (!verify_file(PATH("ul_a.bin"), len, 0xa1) ||
          !verify_file(PATH("ul_c.bin"), len, 0xc3))
        {
          snprintf(detail, sizeof(detail),
                   "neighbour corrupted by injection %d", n);
          all_ok = false;
          break;
        }

      /* Invariant 2: the target is either wholly gone or wholly intact.
       * A directory entry surviving with unreadable or truncated contents
       * would mean the commit and the free happened in the wrong order.
       */

      if (stat(PATH("ul_b.bin"), &st) == 0)
        {
          saw_survival = true;

          if (st.st_size != (off_t)len ||
              !verify_file(PATH("ul_b.bin"), len, 0xb2))
            {
              snprintf(detail, sizeof(detail),
                       "unlink left a damaged file at injection %d", n);
              all_ok = false;
              break;
            }
        }
      else
        {
          saw_deletion = true;
        }

      /* Invariant 3: the recovered allocator agrees with the recovered
       * directory.  Filling the volume and re-checking the survivors is what
       * would catch blocks that were freed while still referenced -- the new
       * file would be placed on top of a live extent.
       */

      if (create_file(PATH("ul_new.bin"), len, 0x5e) == 0 &&
          !verify_file(PATH("ul_new.bin"), len, 0x5e))
        {
          snprintf(detail, sizeof(detail),
                   "new file unreadable after injection %d", n);
          all_ok = false;
          break;
        }

      if (!verify_file(PATH("ul_a.bin"), len, 0xa1) ||
          !verify_file(PATH("ul_c.bin"), len, 0xc3))
        {
          snprintf(detail, sizeof(detail),
                   "allocator reissued live blocks after injection %d", n);
          all_ok = false;
          break;
        }

      if (stat(PATH("ul_b.bin"), &st) == 0 &&
          !verify_file(PATH("ul_b.bin"), len, 0xb2))
        {
          snprintf(detail, sizeof(detail),
                   "surviving target clobbered after injection %d", n);
          all_ok = false;
          break;
        }

      if (exhausted)
        {
          break;
        }
    }

  CHECK("unlink stays consistent across every injection point", all_ok,
        detail);
  CHECK("unlink injection actually interrupted some operations", saw_failure,
        "no unlink was ever cut short; the sweep proved nothing");

  /* Both outcomes must be reachable.  If the sweep only ever saw one of
   * them the commit point is not where it is documented to be, and the
   * other half of the invariant went untested.
   */

  CHECK("both unlink outcomes observed across the sweep",
        saw_survival && saw_deletion,
        saw_survival ? "no injection ever completed the delete"
                     : "no injection ever preserved the file");

  /* Say plainly whether the sweep covered the whole operation or merely the
   * first max_ops of it, so a green result is not mistaken for coverage it
   * does not have on a medium with a different geometry.
   */

  printf("    swept %d injection points, %s\n", n,
         exhausted ? "covering the whole unlink"
                   : "stopping at the cap before the unlink ran out of ops");

  remount();
  unlink(PATH("ul_a.bin"));
  unlink(PATH("ul_b.bin"));
  unlink(PATH("ul_c.bin"));
  unlink(PATH("ul_new.bin"));
}

/****************************************************************************
 * Test: power loss at every step of a defragmentation pass
 *
 * This is the sweep that matters most, because defrag is the only operation
 * that moves data a user already owns.  Everywhere else an interrupted
 * operation can at worst discard something that was never complete; here a
 * relocation that tore could destroy a file that was safe before the pass
 * began.
 *
 * So the invariant is absolute rather than two-sided: defrag never creates
 * or removes a file, so after a power loss at any point during it, every
 * file must still be present and byte-identical.  There is no acceptable
 * "or absent" case.
 ****************************************************************************/

static void test_powerloss_defrag(void)
{
  /* Each relocation is an erase of the destination, a page-at-a-time copy,
   * a metadata commit and an erase of the vacated blocks.  How many flash
   * operations that comes to depends entirely on the medium's page and
   * erase geometry, so rather than guess, the sweep walks injection points
   * until two in a row fail to reach the injector at all -- meaning it has
   * walked off the end of the pass -- and reports whether it got that far
   * before the cap.  On rammtd the whole pass is about fifty operations;
   * a medium with smaller pages will take more, and the cap is what keeps
   * the suite's runtime bounded there.
   */

  const int max_ops = 128;
  const int nfiles = 10;
  const size_t small = 4096;
  struct xipfs_defrag_result_s result;
  bool all_ok = true;
  bool saw_failure = false;
  bool saw_progress = false;
  bool exhausted = false;
  char detail[80];
  char path[64];
  int quiet = 0;
  int ret;
  int n;
  int i;

  printf("\n-- power loss during defrag --\n");
  detail[0] = '\0';

  for (n = 0; n < max_ops && all_ok; n++)
    {
      ret = remount();
      if (ret < 0)
        {
          snprintf(detail, sizeof(detail),
                   "remount failed before injection %d: %d", n, ret);
          all_ok = false;
          break;
        }

      /* Rebuild the same fragmented layout every iteration so that a given
       * injection point means the same thing each time round.
       */

      for (i = 0; i < nfiles; i++)
        {
          snprintf(path, sizeof(path), PATH("pd%d.bin"), i);
          unlink(path);
        }

      for (i = 0; i < nfiles; i++)
        {
          snprintf(path, sizeof(path), PATH("pd%d.bin"), i);
          if (create_file(path, small, (uint8_t)(0x40 + i)) < 0)
            {
              snprintf(detail, sizeof(detail),
                       "baseline create %d failed at injection %d", i, n);
              all_ok = false;
              break;
            }
        }

      if (!all_ok)
        {
          break;
        }

      /* Punch out every other file: plenty of free space in total, none of
       * it in a single run.  This is what gives defrag work to do.
       */

      for (i = 0; i < nfiles; i += 2)
        {
          snprintf(path, sizeof(path), PATH("pd%d.bin"), i);
          unlink(path);
        }

      if (arm_injector(n, XIPFS_FAULT_CLEAN) < 0)
        {
          strlcpy(detail, "cannot arm injector", sizeof(detail));
          all_ok = false;
          break;
        }

      /* Cut power partway through the pass.  Note that defrag reports a
       * clean stop rather than an error when a relocation fails, so the
       * media error surfaces in the reason code, not the return value.
       */

      ret = run_defrag(0, &result);
      if (ret < 0)
        {
          snprintf(detail, sizeof(detail),
                   "defrag returned an error at injection %d: %d", n, ret);
          all_ok = false;
          break;
        }

      if (result.reason == XIPFS_DEFRAG_ERROR)
        {
          saw_failure = true;
          quiet = 0;
        }
      else if (++quiet >= 2)
        {
          /* The pass ran to completion without ever tripping the injector,
           * twice over: every operation in it has now been swept.
           */

          exhausted = true;
        }

      if (result.extents_moved > 0)
        {
          saw_progress = true;
        }

      /* Rebuilding a ten file layout against real flash takes a couple of
       * seconds per injection point, and the sweep is over a hundred of
       * them.  Without a heartbeat the suite looks wedged for minutes on
       * hardware, so a silent run cannot be told from a hung one.
       */

      putchar('.');
      fflush(stdout);

      /* Reboot */

      ret = remount();
      if (ret < 0)
        {
          snprintf(detail, sizeof(detail),
                   "mount failed after injection %d: %d", n, ret);
          all_ok = false;
          break;
        }

      /* The invariant: every survivor is still there, still whole.  Whether
       * it was relocated, half relocated or never touched must be
       * indistinguishable from the outside.
       */

      for (i = 1; i < nfiles; i += 2)
        {
          snprintf(path, sizeof(path), PATH("pd%d.bin"), i);
          if (!verify_file(path, small, (uint8_t)(0x40 + i)))
            {
              snprintf(detail, sizeof(detail),
                       "file %d lost or corrupted by injection %d", i, n);
              all_ok = false;
              break;
            }
        }

      if (!all_ok)
        {
          break;
        }

      /* And the recovered allocator must not believe the blocks under a
       * survivor are free.  A relocation that committed but was interrupted
       * before erasing its source leaves stale copies behind; those blocks
       * are unreferenced and reusable, but the blocks under the new location
       * are not, and a mount that confused the two would hand them out here.
       */

      unlink(PATH("pd_new.bin"));
      if (create_file(PATH("pd_new.bin"), small, 0x5f) == 0)
        {
          if (!verify_file(PATH("pd_new.bin"), small, 0x5f))
            {
              snprintf(detail, sizeof(detail),
                       "new file unreadable after injection %d", n);
              all_ok = false;
              break;
            }

          for (i = 1; i < nfiles; i += 2)
            {
              snprintf(path, sizeof(path), PATH("pd%d.bin"), i);
              if (!verify_file(path, small, (uint8_t)(0x40 + i)))
                {
                  snprintf(detail, sizeof(detail),
                           "allocator reissued live blocks after "
                           "injection %d", n);
                  all_ok = false;
                  break;
                }
            }
        }

      if (exhausted)
        {
          break;
        }
    }

  putchar('\n');

  CHECK("no file is ever lost to a power loss during defrag", all_ok,
        detail);
  CHECK("defrag injection actually interrupted some relocations",
        saw_failure, "no relocation was ever cut short; the sweep "
                     "proved nothing");
  CHECK("defrag made real progress at some injection points", saw_progress,
        "no extent was ever relocated; the sweep never reached a move");

  printf("    swept %d injection points, %s\n", n,
         exhausted ? "covering every operation of the whole pass"
                   : "stopping at the cap partway through the pass");

  remount();
  for (i = 0; i < nfiles; i++)
    {
      snprintf(path, sizeof(path), PATH("pd%d.bin"), i);
      unlink(path);
    }

  unlink(PATH("pd_new.bin"));
}

/****************************************************************************
 * Test: strict versus permissive mapping
 ****************************************************************************/

static void test_strict(void)
{
  const size_t len = 4096;
  FAR void *addr;
  int fd;
  int ret;

  printf("\n-- strict mapping semantics --\n");

  ret = create_file(PATH("strict.bin"), len, 0x88);
  CHECK("create", ret == 0, strerror(-ret));

  fd = open(PATH("strict.bin"), O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      CHECK("open", false, strerror(errno));
      return;
    }

  /* Beyond the end of the file: strict must refuse rather than quietly
   * hand back something that is not what was asked for.
   */

  addr = mmap(NULL, len * 4, PROT_READ, MAP_SHARED | MAP_XIP_STRICT, fd, 0);
  CHECK("strict mapping past end of file is refused", addr == MAP_FAILED,
        "oversized mapping accepted");

  addr = mmap(NULL, len, PROT_READ, MAP_SHARED | MAP_XIP_STRICT, fd, 0);
  CHECK("strict mapping of a valid range succeeds", addr != MAP_FAILED,
        strerror(errno));

  if (addr != MAP_FAILED)
    {
      munmap(addr, len);
    }

  close(fd);
  unlink(PATH("strict.bin"));
}

/****************************************************************************
 * Test: the write-once model is enforced rather than half-implemented
 ****************************************************************************/

static void test_writeonce(void)
{
  const size_t len = 1024;
  ssize_t nwritten;
  int fd;
  int ret;

  printf("\n-- write-once enforcement --\n");

  ret = create_file(PATH("once.bin"), len, 0x99);
  CHECK("create", ret == 0, strerror(-ret));

  fd = open(PATH("once.bin"), O_WRONLY | O_CLOEXEC);
  CHECK("reopening a closed file for writing is refused", fd < 0,
        "file was writable after close");
  if (fd >= 0)
    {
      close(fd);
    }

  fd = open(PATH("once.bin"), O_WRONLY | O_APPEND | O_CLOEXEC);
  CHECK("append is refused", fd < 0, "file was appendable");
  if (fd >= 0)
    {
      close(fd);
    }

  /* A seek away from the append point during a create would leave a hole,
   * which an exactly sized extent cannot represent.
   */

  fd = open(PATH("hole.bin"),
            O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd >= 0)
    {
      ftruncate(fd, len);
      ret = lseek(fd, 512, SEEK_SET);
      CHECK("seeking while creating is refused", ret < 0, "seek allowed");

      nwritten = write(fd, g_buffer, 16);
      CHECK("sequential write after a rejected seek still works",
            nwritten == 16, "write failed");
      close(fd);
    }

  unlink(PATH("once.bin"));
  unlink(PATH("hole.bin"));
}

/****************************************************************************
 * Test: power loss during mkdir and rmdir
 *
 * A directory is a record in the metadata generation, so creating or
 * removing one is the same single-generation commit that create and unlink
 * are.  That is the claim; this sweeps it.  For every flash operation in
 * turn, fail at that one and assert the volume comes back as a tree with the
 * directory either wholly there or wholly absent -- never a record whose
 * parent has gone, which is what a partially applied change would look like
 * and what mount would then have to reject.
 ****************************************************************************/

static void test_powerloss_dirs(void)
{
  const int max_ops = 12;
  const size_t len = 2000;
  struct stat st;
  bool all_ok = true;
  bool saw_failure = false;
  bool saw_created = false;
  bool saw_absent = false;
  char detail[80];
  int quiet = 0;
  int ret;
  int n;

  printf("\n-- power loss during mkdir and rmdir --\n");
  detail[0] = '\0';

  for (n = 0; n < max_ops; n++)
    {
      /* Baseline: a directory holding a file, plus a file beside it, so a
       * torn commit has neighbours to damage and a child to orphan.
       */

      ret = remount();
      if (ret < 0)
        {
          snprintf(detail, sizeof(detail),
                   "remount failed before injection %d: %d", n, ret);
          all_ok = false;
          break;
        }

      unlink(PATH("d_keep/inner.bin"));
      rmdir(PATH("d_keep"));
      rmdir(PATH("d_new"));
      unlink(PATH("d_file.bin"));

      if (mkdir(PATH("d_keep"), 0755) < 0 ||
          create_file(PATH("d_keep/inner.bin"), len, 0xd1) < 0 ||
          create_file(PATH("d_file.bin"), len, 0xd2) < 0)
        {
          snprintf(detail, sizeof(detail), "baseline failed at %d", n);
          all_ok = false;
          break;
        }

      if (arm_injector(n, XIPFS_FAULT_TORN) < 0)
        {
          strlcpy(detail, "cannot arm injector", sizeof(detail));
          all_ok = false;
          break;
        }

      /* The operation being cut short */

      if (mkdir(PATH("d_new"), 0755) < 0)
        {
          saw_failure = true;
          quiet = 0;
        }
      else if (++quiet >= 2)
        {
          break;    /* Walked off the end of the mkdir */
        }

      ret = remount();
      if (ret < 0)
        {
          snprintf(detail, sizeof(detail),
                   "mount failed after injection %d: %d", n, ret);
          all_ok = false;
          break;
        }

      /* The new directory is either there or not, and either way the tree
       * that was already committed is untouched and still walkable.
       */

      if (stat(PATH("d_new"), &st) == 0)
        {
          saw_created = true;

          if (!S_ISDIR(st.st_mode))
            {
              snprintf(detail, sizeof(detail),
                       "mkdir left a non-directory at injection %d", n);
              all_ok = false;
              break;
            }
        }
      else
        {
          saw_absent = true;
        }

      if (stat(PATH("d_keep"), &st) != 0 || !S_ISDIR(st.st_mode) ||
          !verify_file(PATH("d_keep/inner.bin"), len, 0xd1) ||
          !verify_file(PATH("d_file.bin"), len, 0xd2))
        {
          snprintf(detail, sizeof(detail),
                   "committed tree damaged by injection %d", n);
          all_ok = false;
          break;
        }

      /* And the same for a removal cut short.  The directory must not come
       * back empty-but-present while its child is gone, nor vice versa.
       */

      if (arm_injector(n, XIPFS_FAULT_TORN) < 0)
        {
          strlcpy(detail, "cannot arm injector", sizeof(detail));
          all_ok = false;
          break;
        }

      rmdir(PATH("d_new"));

      ret = remount();
      if (ret < 0)
        {
          snprintf(detail, sizeof(detail),
                   "mount failed after rmdir injection %d: %d", n, ret);
          all_ok = false;
          break;
        }

      if (stat(PATH("d_keep"), &st) != 0 ||
          !verify_file(PATH("d_keep/inner.bin"), len, 0xd1))
        {
          snprintf(detail, sizeof(detail),
                   "rmdir injection %d damaged another directory", n);
          all_ok = false;
          break;
        }
    }

  arm_injector(-1, XIPFS_FAULT_CLEAN);

  CHECK("mkdir and rmdir stay consistent across every injection point",
        all_ok, detail);
  CHECK("injection actually interrupted some directory operations",
        saw_failure, "nothing was ever cut short");
  CHECK("both outcomes observed across the sweep",
        saw_created && saw_absent, "only one outcome seen");

  unlink(PATH("d_keep/inner.bin"));
  rmdir(PATH("d_keep"));
  rmdir(PATH("d_new"));
  unlink(PATH("d_file.bin"));
}

#ifdef CONFIG_FDPIC

/****************************************************************************
 * Test: FDPIC modules loaded out of the filesystem
 *
 * These are here rather than in the demo because the properties they check
 * are the ones that fail *quietly*.  A loader that skips DT_INIT_ARRAY runs
 * a C++ module perfectly happily with every global left as zero; one that
 * skips DT_JMPREL loads a module that hard-faults only once it calls out;
 * one that mis-sizes the descriptor pool corrupts the heap. None of those
 * announce themselves, so they need something that asserts.
 *
 * The module exit status is the channel: each module checks its own
 * invariants, and the C++ one returns a bitmask so that all four of its
 * properties are evaluated on every run instead of the first failure hiding
 * the rest.
 ****************************************************************************/

/* Kept in step with ../../../examples/fdpicxip/modules/cxxuser.cpp */

#define USER_FAIL_OWN_CTOR  0x01
#define USER_FAIL_LIB_CTOR  0x02
#define USER_FAIL_ORDER     0x04
#define USER_FAIL_PRIVATE   0x08

/* Kept in step with ../../../examples/fdpicxip/modules/callback.c */

#define CB_FAIL_QSORT       0x01
#define CB_FAIL_PTHREAD     0x02
#define CB_FAIL_SIGNAL      0x04
#define CB_FAIL_TASK        0x08
#define CB_FAIL_ONCE        0x10
#define CB_FAIL_SPAWN       0x20
#define CB_FAIL_SCANDIR     0x40
#define CB_FAIL_SIGACTION   0x80

/* The mq checks run as a second invocation ("callback mq") and report in
 * their own byte, because waitpid only preserves the low eight bits of an
 * exit status and the checks above already fill them.
 */

#define CB_FAIL_MQ_THREAD    0x01
#define CB_FAIL_MQ_SIGNAL    0x02
#define CB_FAIL_TIMER_THREAD 0x04

static int stage_blob(FAR const char *path, FAR const unsigned char *data,
                      unsigned int len)
{
  ssize_t nwritten;
  int fd;

  unlink(path);

  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd < 0)
    {
      return -errno;
    }

  /* Declare the size up front so the extent is reserved exactly */

  if (ftruncate(fd, len) < 0)
    {
      close(fd);
      return -errno;
    }

  nwritten = write(fd, data, len);
  close(fd);

  return (nwritten == (ssize_t)len) ? 0 : -EIO;
}

/****************************************************************************
 * Name: run_module
 *
 * Description:
 *   Spawn a module and return its exit status, or a negative errno if it
 *   never got as far as exiting.
 *
 *   Note that binfmt reports a loader failure as -ENOENT, because at the
 *   dispatch layer "the loader refused it" and "not my format" are the same
 *   answer.  A spawn failure here therefore says nothing about *why*; the
 *   reason only reaches the syslog.
 *
 ****************************************************************************/

static int run_module(FAR const char *path, FAR char *const argv[])
{
  int status;
  pid_t pid;
  int ret;

  ret = posix_spawn(&pid, path, NULL, NULL, argv, NULL);
  if (ret != 0)
    {
      return -ret;
    }

  if (waitpid(pid, &status, 0) < 0)
    {
      return -errno;
    }

  if (!WIFEXITED(status))
    {
      return -ECHILD;
    }

  return WEXITSTATUS(status);
}

/****************************************************************************
 * Name: read_marker
 *
 * Description:
 *   Read back the integer a module's destructor left behind, or -1.
 *
 ****************************************************************************/

static int read_marker(FAR const char *path)
{
  char buf[16];
  ssize_t nread;
  int fd;

  fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      return -1;
    }

  nread = read(fd, buf, sizeof(buf) - 1);
  close(fd);

  if (nread <= 0)
    {
      return -1;
    }

  buf[nread] = '\0';
  return atoi(buf);
}

static void test_fdpic(void)
{
  struct xipfs_extent_info_s info;
  FAR unsigned char *corrupt;
  char marker[2][64];
  char result[2][64];
  char seed[2][8];
  FAR char *args[5];
  pid_t pid[2];
  int fails;
  bool ok;
  int ret;
  int i;

  printf("\n-- FDPIC modules --\n");

  /* A module whose imported function pointers are R_ARM_FUNCDESC: the
   * loader has to manufacture a descriptor for each out of the pool behind
   * the writable segment.  Nothing else in the tree emits one.  It calls
   * through every one of them, because a descriptor with the wrong entry or
   * the wrong GOT half is invisible until it is branched through.
   */

  ret = stage_blob(PATH("funcdesc"), g_funcdesc, g_funcdesc_len);
  CHECK("stage the FUNCDESC module", ret == 0, strerror(-ret));

  args[0] = (FAR char *)"funcdesc";
  args[1] = (FAR char *)"7";
  args[2] = NULL;

  ret = run_module(PATH("funcdesc"), args);
  CHECK("descriptors manufactured by the loader are callable",
        ret == 0, ret < 0 ? strerror(-ret) : "module reported failure");

  unlink(PATH("funcdesc"));

  /* A module linked without -z now, so every import is in DT_JMPREL and
   * there is no DT_REL at all.  Before the loader walked that table this
   * loaded cleanly and then hard-faulted on its first call out.
   */

  ret = stage_blob(PATH("lazymod"), g_lazymod, g_lazymod_len);
  CHECK("stage the DT_JMPREL module", ret == 0, strerror(-ret));

  args[0] = (FAR char *)"lazymod";
  args[1] = (FAR char *)"42";
  args[2] = NULL;

  ret = run_module(PATH("lazymod"), args);
  CHECK("a module bound entirely through DT_JMPREL runs",
        ret == 0, ret < 0 ? strerror(-ret) : "module reported failure");

  unlink(PATH("lazymod"));

  /* Function pointers handed the other way: from the module into firmware
   * that will branch to them later.
   *
   * In FDPIC such a pointer is the address of a descriptor sitting in the
   * module's writable segment, so firmware that treats it as a code address
   * jumps into RAM data.  Only entry points that resolve the descriptor can
   * accept one.  qsort and bsearch always did; pthread_create, signal and
   * task_create looked usable and took the board down instead.
   *
   * The module reports a bitmask, so all four are evaluated per run.
   */

  ret = stage_blob(PATH("callback"), g_callback, g_callback_len);
  CHECK("stage the callback module", ret == 0, strerror(-ret));

  args[0] = (FAR char *)"callback";
  args[1] = NULL;

  fails = run_module(PATH("callback"), args);

  CHECK("the callback module runs at all", fails >= 0,
        fails < 0 ? strerror(-fails) : "");

  if (fails < 0)
    {
      fails = CB_FAIL_QSORT | CB_FAIL_PTHREAD | CB_FAIL_SIGNAL |
              CB_FAIL_TASK | CB_FAIL_ONCE | CB_FAIL_SPAWN |
              CB_FAIL_SCANDIR | CB_FAIL_SIGACTION;
    }

  CHECK("qsort() calls back into a module comparison function",
        (fails & CB_FAIL_QSORT) == 0, "callback did not run");
  CHECK("pthread_create() accepts a module start routine",
        (fails & CB_FAIL_PTHREAD) == 0, "thread did not reach module code");
  CHECK("signal() accepts a module handler",
        (fails & CB_FAIL_SIGNAL) == 0, "handler did not run");
  CHECK("task_create() accepts a module entry point",
        (fails & CB_FAIL_TASK) == 0, "task did not reach module code");
  CHECK("sigaction() accepts a module handler",
        (fails & CB_FAIL_SIGACTION) == 0, "handler did not run");
  CHECK("pthread_once() accepts a module init routine",
        (fails & CB_FAIL_ONCE) == 0, "init routine did not run");
  CHECK("task_spawn() accepts a module entry point",
        (fails & CB_FAIL_SPAWN) == 0, "task did not reach module code");

  /* scandir passes its comparison function on to qsort, which resolves it
   * too.  Exercising filter and comparison together is what would catch the
   * comparison function being resolved twice.
   */

  CHECK("scandir() accepts a module filter and comparison function",
        (fails & CB_FAIL_SCANDIR) == 0, "filter did not run");

  /* mq_notify runs as a second invocation: its two outcomes report in their
   * own byte, since the checks above already fill the eight bits waitpid
   * preserves.  Both SIGEV_THREAD callbacks run on a work-queue worker with
   * the module's data base installed around the call, so they reach the
   * module's own globals; SIGEV_SIGNAL is caught by a sigaction handler.
   */

  args[1] = (FAR char *)"mq";
  args[2] = NULL;
  fails = run_module(PATH("callback"), args);

  if (fails < 0)
    {
      fails = CB_FAIL_MQ_THREAD | CB_FAIL_MQ_SIGNAL | CB_FAIL_TIMER_THREAD;
    }

  CHECK("mq_notify(SIGEV_THREAD) reaches a module callback on the worker",
        (fails & CB_FAIL_MQ_THREAD) == 0, "callback did not reach module");
  CHECK("mq_notify(SIGEV_SIGNAL) reaches a module sigaction handler",
        (fails & CB_FAIL_MQ_SIGNAL) == 0, "handler did not run");
  CHECK("timer_create(SIGEV_THREAD) reaches a module callback on the worker",
        (fails & CB_FAIL_TIMER_THREAD) == 0, "callback did not reach");

  unlink(PATH("callback"));

  /* A module the loader must refuse.
   *
   * The OS/ABI byte is the FDPIC marker, and it is the only thing that
   * distinguishes a module from any other ELF shared object.  Clearing it
   * should make the loader decline rather than march on and try to relocate
   * something that is not a module -- and a loader that accepts anything is
   * a loader whose acceptance means nothing, so this is worth asserting.
   *
   * Built by corrupting a module that is known to load, so nothing but the
   * marker differs.
   */

  corrupt = malloc(g_funcdesc_len);
  if (corrupt != NULL)
    {
      /* xipfs is write-once, so the edit happens on the way in.  EI_OSABI
       * is byte 7; clearing it to ELFOSABI_NONE leaves a file that is a
       * perfectly valid ELF shared object and simply not a module.
       */

      memcpy(corrupt, g_funcdesc, g_funcdesc_len);
      corrupt[EI_OSABI] = ELFOSABI_NONE;

      ret = stage_blob(PATH("corrupt"), corrupt, g_funcdesc_len);
      free(corrupt);
    }
  else
    {
      ret = -ENOMEM;
    }

  CHECK("stage a module with the FDPIC marker cleared", ret == 0,
        strerror(-ret));

  if (ret == 0)
    {
      args[0] = (FAR char *)"corrupt";
      args[1] = NULL;

      CHECK("a module without the FDPIC marker is refused",
            run_module(PATH("corrupt"), args) < 0, "it loaded anyway");
    }

  unlink(PATH("corrupt"));

  /* A leaf library -- one that calls nothing outside itself.
   *
   * With no external calls there is no PLT and so no DT_PLTGOT, and the
   * loader has to fall back to PT_DYNAMIC vaddr + memsz to find the GOT.
   * That fallback is inferred from the linker's layout rather than from the
   * ABI document, which makes it the least certain thing in the loader.
   * libshape below does not exercise it -- it calls syslog, so it has a
   * PLT.  If the fallback were wrong the library could not reach its own
   * globals, and the total would come back wrong.
   */

  ret = stage_blob(PATH("libcounter.so"), g_libcounter, g_libcounter_len);
  CHECK("stage the leaf library", ret == 0, strerror(-ret));

  ret = stage_blob(PATH("counteruser"), g_counteruser, g_counteruser_len);
  CHECK("stage its consumer", ret == 0, strerror(-ret));

  args[0] = (FAR char *)"counteruser";
  args[1] = (FAR char *)"2";
  args[2] = NULL;

  ret = run_module(PATH("counteruser"), args);
  CHECK("a leaf library with no DT_PLTGOT reaches its own data",
        ret == 0, ret < 0 ? strerror(-ret) : "wrong total");

  unlink(PATH("counteruser"));
  unlink(PATH("libcounter.so"));

  /* C++: a shared library and a module, both with global objects.  Two
   * instances run concurrently, because private-per-instance library state
   * is only observable while they overlap.
   */

  ret = stage_blob(PATH("libshape.so"), g_libshape, g_libshape_len);
  CHECK("stage the C++ shared library", ret == 0, strerror(-ret));

  ret = stage_blob(PATH("cxxuser"), g_cxxuser, g_cxxuser_len);
  CHECK("stage the C++ module", ret == 0, strerror(-ret));

  for (i = 0; i < 2; i++)
    {
      snprintf(seed[i], sizeof(seed[i]), "%d", i + 1);
      snprintf(marker[i], sizeof(marker[i]), PATH("dtor%d"), i + 1);
      snprintf(result[i], sizeof(result[i]), PATH("res%d"), i + 1);
      unlink(marker[i]);
      unlink(result[i]);

      args[0] = (FAR char *)"cxxuser";
      args[1] = seed[i];
      args[2] = marker[i];
      args[3] = result[i];
      args[4] = NULL;

      if (posix_spawn(&pid[i], PATH("cxxuser"), NULL, NULL, args, NULL) != 0)
        {
          pid[i] = -1;
        }
    }

  CHECK("two C++ instances spawn", pid[0] > 0 && pid[1] > 0,
        "posix_spawn failed");

  usleep(150000);

  ret = extent_info(PATH("libshape.so"), &info);
  CHECK("one flash copy of the library, pinned once per instance",
        ret == 0 && info.pincount == 2, "wrong pin count");

  /* Wait for both, but do not read the exit status from waitpid().
   *
   * waitpid() only keeps the status of an already-exited child when
   * CONFIG_SCHED_CHILD_STATUS is set, which needs CONFIG_SCHED_HAVE_PARENT
   * -- neither of which this test should require.  Both instances run for
   * the same 300ms, so the second has always exited by the time the first
   * waitpid() returns, and its status is simply gone.  Each instance
   * therefore records its own result in a file, which survives both the
   * task and the unload.  waitpid() here is only a barrier, and may fail.
   */

  for (i = 0; i < 2; i++)
    {
      int status;

      if (pid[i] > 0)
        {
          waitpid(pid[i], &status, 0);
        }
    }

  fails = 0;
  ok    = true;

  for (i = 0; i < 2; i++)
    {
      ret = read_marker(result[i]);
      if (ret < 0)
        {
          ok = false;
        }
      else
        {
          fails |= ret;
        }
    }

  CHECK("both C++ instances ran to completion", ok,
        "an instance left no result");

  CHECK("the module's own global constructor ran",
        (fails & USER_FAIL_OWN_CTOR) == 0, "global left as .bss");
  CHECK("the library's global constructor ran",
        (fails & USER_FAIL_LIB_CTOR) == 0, "global left as .bss");
  CHECK("the library was constructed before the module needing it",
        (fails & USER_FAIL_ORDER) == 0, "wrong DT_NEEDED order");
  CHECK("each instance has its own copy of the library's data",
        (fails & USER_FAIL_PRIVATE) == 0, "library state was shared");

  /* Destructors run on unload, which happens as each task is reaped --
   * after waitpid() has already returned.
   */

  usleep(200000);

  ok = true;
  for (i = 0; i < 2; i++)
    {
      if (read_marker(marker[i]) != (i + 1) * 3)
        {
          ok = false;
        }
    }

  CHECK("destructors ran on unload, each with its instance's own state",
        ok, "marker missing or wrong");

  ret = extent_info(PATH("libshape.so"), &info);
  CHECK("the library's pins return to zero once both instances are gone",
        ret == 0 && info.pincount == 0, "pin leaked");

  for (i = 0; i < 2; i++)
    {
      unlink(marker[i]);
      unlink(result[i]);
    }

  unlink(PATH("cxxuser"));
  unlink(PATH("libshape.so"));
}

/****************************************************************************
 * Name: dup_blob
 *
 * Description:
 *   A private, writable copy of an embedded module image, for mutating into
 *   a malformed one.  The original is const .rodata and must not be touched.
 *
 ****************************************************************************/

static FAR unsigned char *dup_blob(FAR const unsigned char *src,
                                   unsigned int len)
{
  FAR unsigned char *copy = malloc(len);

  if (copy != NULL)
    {
      memcpy(copy, src, len);
    }

  return copy;
}

/****************************************************************************
 * Name: corrupt_dyn
 *
 * Description:
 *   Rewrite the value of one PT_DYNAMIC tag in place.  Returns true if the
 *   tag was present.  This reaches the dynamic array straight through the
 *   program header's file offset, so it needs no vaddr-to-file mapping.
 *
 ****************************************************************************/

static bool corrupt_dyn(FAR unsigned char *buf, unsigned int len,
                        int32_t tag, uint32_t newval)
{
  FAR const Elf32_Ehdr *eh = (FAR const Elf32_Ehdr *)buf;
  FAR const Elf32_Phdr *ph;
  unsigned int i;

  if (len < sizeof(Elf32_Ehdr) ||
      eh->e_phoff + (uint32_t)eh->e_phnum * sizeof(Elf32_Phdr) > len)
    {
      return false;
    }

  ph = (FAR const Elf32_Phdr *)(buf + eh->e_phoff);
  for (i = 0; i < eh->e_phnum; i++)
    {
      if (ph[i].p_type == PT_DYNAMIC &&
          ph[i].p_offset + ph[i].p_filesz <= len)
        {
          FAR Elf32_Dyn *dyn = (FAR Elf32_Dyn *)(buf + ph[i].p_offset);
          unsigned int n = ph[i].p_filesz / sizeof(Elf32_Dyn);
          unsigned int j;

          for (j = 0; j < n && dyn[j].d_tag != DT_NULL; j++)
            {
              if (dyn[j].d_tag == tag)
                {
                  dyn[j].d_un.d_val = newval;
                  return true;
                }
            }
        }
    }

  return false;
}

/****************************************************************************
 * Name: expect_refused
 *
 * Description:
 *   Stage an image and assert the loader declines it.  A refusal is any
 *   negative return: binfmt collapses "the loader rejected it" and "not my
 *   format" into one answer (see run_module), which is all this needs to
 *   know -- the point is that a malformed image never loads and runs.
 *
 ****************************************************************************/

static void expect_refused(FAR const char *what,
                           FAR const unsigned char *img, unsigned int len)
{
  FAR char *args[2];
  int ret;

  ret = stage_blob(PATH("reject"), img, len);
  if (ret < 0)
    {
      CHECK(what, false, "could not stage the mutant");
      return;
    }

  args[0] = (FAR char *)"reject";
  args[1] = NULL;

  CHECK(what, run_module(PATH("reject"), args) < 0, "it loaded anyway");
  unlink(PATH("reject"));
}

/****************************************************************************
 * Test: the loader refuses malformed modules
 *
 * A loader that accepts anything is a loader whose acceptance means nothing.
 * The paths above are what the loader does when it succeeds; these are its
 * rejections.  Each is synthesised by mutating a module that is known to
 * load, so nothing but the defect under test differs, and each must come
 * back *refused* rather than loaded-and-then-faulting -- the latter is the
 * failure mode that takes the board down instead of printing FAIL.
 ****************************************************************************/

static void test_fdpic_reject(void)
{
  FAR unsigned char *img;
  int ret;

  printf("\n-- FDPIC loader rejections --\n");

  /* A 64-bit class byte: the loader is 32-bit only. */

  img = dup_blob(g_funcdesc, g_funcdesc_len);
  if (img != NULL)
    {
      img[EI_CLASS] = ELFCLASS64;
      expect_refused("a non-32-bit ELF class is refused", img,
                     g_funcdesc_len);
      free(img);
    }

  /* Not a shared object.  An FDPIC module is ET_DYN; anything else is not a
   * module even with the FDPIC marker set.
   */

  img = dup_blob(g_funcdesc, g_funcdesc_len);
  if (img != NULL)
    {
      ((FAR Elf32_Ehdr *)img)->e_type = ET_EXEC;
      expect_refused("a non-ET_DYN object is refused", img, g_funcdesc_len);
      free(img);
    }

  /* Wrong machine.  A module built for another architecture must be turned
   * away before any of its relocations are applied.
   */

  img = dup_blob(g_funcdesc, g_funcdesc_len);
  if (img != NULL)
    {
      ((FAR Elf32_Ehdr *)img)->e_machine = EM_X86_64;
      expect_refused("a module for the wrong machine is refused", img,
                     g_funcdesc_len);
      free(img);
    }

  /* RELA PLT relocations.  Nothing in the loader reads RELA; an object
   * claiming that layout for its PLT must be refused, not walked as REL --
   * the entries are 12 bytes rather than 8, so misreading them would apply
   * garbage.  lazymod carries its imports in DT_JMPREL, so it has the
   * DT_PLTREL tag to flip.
   */

  img = dup_blob(g_lazymod, g_lazymod_len);
  if (img != NULL)
    {
      if (corrupt_dyn(img, g_lazymod_len, DT_PLTREL, DT_RELA))
        {
          expect_refused("a module with RELA PLT relocations is refused",
                         img, g_lazymod_len);
        }
      else
        {
          CHECK("a module with RELA PLT relocations is refused", false,
                "lazymod has no DT_PLTREL to corrupt");
        }

      free(img);
    }

  /* A named dependency that is not there.  counteruser needs libcounter.so;
   * with the library absent the loader must fail the whole load rather than
   * leave the module's imports dangling.
   */

  ret = stage_blob(PATH("counteruser"), g_counteruser, g_counteruser_len);
  if (ret == 0)
    {
      FAR char *args[3];

      unlink(PATH("libcounter.so"));

      args[0] = (FAR char *)"counteruser";
      args[1] = (FAR char *)"2";
      args[2] = NULL;

      CHECK("a module whose needed library is absent is refused",
            run_module(PATH("counteruser"), args) < 0, "it loaded anyway");

      unlink(PATH("counteruser"));
    }
  else
    {
      CHECK("a module whose needed library is absent is refused", false,
            "could not stage counteruser");
    }

  /* An import the firmware does not export.  The module links cleanly -- the
   * symbol is just an undefined import -- so the loader is the only thing
   * that can catch it, when resolution fails rather than at link time.  This
   * is a different path from the absent-library case above: the dependency
   * resolves, the symbol inside it does not.
   */

  expect_refused("a module importing an unexported symbol is refused",
                 g_missingsym, g_missingsym_len);

  /* More than FDPIC_MAX_NEEDED (8) DT_NEEDED entries.  The module is refused
   * while its dynamic section is parsed, before any dependency is loaded, so
   * the nine libraries it was linked against need not be present.
   */

  expect_refused("a module with too many DT_NEEDED entries is refused",
                 g_manyneeded, g_manyneeded_len);
}
#endif /* CONFIG_FDPIC */

/****************************************************************************
 * Name: want
 *
 * Description:
 *   Whether a named section should run.  With no argument everything does.
 *
 *   The selector exists because the power loss sweeps dominate the runtime
 *   -- they are minutes, everything else is seconds -- so iterating on any
 *   other section means waiting for work that is not being changed.
 *
 ****************************************************************************/

static bool want(FAR const char *name, int argc, FAR char *argv[])
{
  return argc < 2 || strcmp(argv[1], name) == 0;
}

#define SECTION(name, fn)             \
  do                                  \
    {                                 \
      if (want((name), argc, argv))   \
        {                             \
          fn();                       \
        }                             \
    }                                 \
  while (0)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  printf("XIPFS test suite on %s (%s)\n", MOUNTPT, MTDDEV);

  /* In a flat build the counters keep their values from the previous run,
   * so a second invocation would report the first one's results too.
   */

  g_passed = 0;
  g_failed = 0;

  g_buffer = malloc(g_bufsize);
  if (g_buffer == NULL)
    {
      printf("ERROR: out of memory\n");
      return EXIT_FAILURE;
    }

  if (remount() < 0)
    {
      printf("ERROR: cannot mount %s; is the board bringup configured?\n",
             MOUNTPT);
      free(g_buffer);
      return EXIT_FAILURE;
    }

  SECTION("basic",     test_basic);
  SECTION("writeonce", test_writeonce);
  SECTION("strict",    test_strict);
  SECTION("xip",       test_xip);
  SECTION("multimap",  test_multimap);
  SECTION("crosstask", test_crosstask_pins);
  SECTION("teardown",  test_teardown_release);
#ifdef CONFIG_FDPIC
  SECTION("fdpic",     test_fdpic);
  SECTION("reject",    test_fdpic_reject);
#endif
  SECTION("dirs",      test_dirs);
  SECTION("defrag",    test_defrag);
  SECTION("powerloss", test_powerloss);
  SECTION("powertorn", test_powerloss_torn);
  SECTION("unlink",    test_powerloss_unlink);
  SECTION("dirloss",   test_powerloss_dirs);
  SECTION("defraglos", test_powerloss_defrag);

  if (g_passed == 0 && g_failed == 0)
    {
      printf("\nno section matched '%s'\n", argv[1]);
      free(g_buffer);
      return EXIT_FAILURE;
    }

  printf("\n==== %d passed, %d failed ====\n", g_passed, g_failed);

  free(g_buffer);
  return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
