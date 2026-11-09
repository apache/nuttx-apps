/****************************************************************************
 * apps/testing/fs/diropen/diropen_main.c
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

/* open() of a directory, and of a file with O_DIRECTORY, as POSIX says.
 *
 * Cases for a directory D with the entry E:
 *   writeonly open(D, O_WRONLY) fails with EISDIR
 *   readwrite open(D, O_RDWR) fails with EISDIR
 *   readonly  open(D, O_RDONLY) gives a descriptor
 *   fstat     fstat() of the descriptor gives a directory
 *   openat    openat() of E relative to the descriptor opens E
 *   readdir   fdopendir() of the descriptor and readdir() find E
 *   odir      open(D, O_RDONLY | O_DIRECTORY), fdopendir() and readdir()
 *             find E
 *
 * Cases for a file F:
 *   notdir    open(F, O_RDONLY | O_DIRECTORY) fails with ENOTDIR
 *   opendir   opendir(F) fails with ENOTDIR
 *
 * In a writable directory, the test makes the directory D and the regular
 * file D/a.  It also checks that D/a still opens for read and for append.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/mount.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DIROPEN_TMPFS   "/diropen.tmpfs"
#define DIROPEN_DATA    "hello"
#define DIROPEN_DATALEN (sizeof(DIROPEN_DATA) - 1)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct diropen_s
{
  int pass;
  int fail;
  int skip;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void diropen_check(FAR struct diropen_s *t, FAR const char *path,
                          FAR const char *name, bool ok, int errcode)
{
  if (ok)
    {
      t->pass++;
      printf("diropen: PASS %s %s\n", path, name);
    }
  else
    {
      t->fail++;
      printf("diropen: FAIL %s %s: errno %d\n", path, name, errcode);
    }
}

static void diropen_skip(FAR struct diropen_s *t, FAR const char *path,
                         FAR const char *what, int errcode)
{
  t->skip++;
  printf("diropen: SKIP %s: %s: errno %d\n", path, what, errcode);
}

static bool diropen_finds(FAR DIR *dirp, FAR const char *name)
{
  FAR struct dirent *entry;
  bool found = false;

  while ((entry = readdir(dirp)) != NULL)
    {
      if (strcmp(entry->d_name, name) == 0)
        {
          found = true;
        }
    }

  return found;
}

static bool diropen_reads(int fd)
{
  char buf[DIROPEN_DATALEN + 1];

  return read(fd, buf, sizeof(buf)) == DIROPEN_DATALEN &&
         memcmp(buf, DIROPEN_DATA, DIROPEN_DATALEN) == 0;
}

/* The cases for the directory path with the entry */

static void diropen_dir(FAR struct diropen_s *t, FAR const char *path,
                        FAR const char *entry)
{
  struct stat buf;
  FAR DIR *dirp;
  bool ok;
  int fd;
  int fd2;

  errno = 0;
  fd = open(path, O_WRONLY | O_CLOEXEC);
  diropen_check(t, path, "writeonly", fd < 0 && errno == EISDIR, errno);
  if (fd >= 0)
    {
      close(fd);
    }

  errno = 0;
  fd = open(path, O_RDWR | O_CLOEXEC);
  diropen_check(t, path, "readwrite", fd < 0 && errno == EISDIR, errno);
  if (fd >= 0)
    {
      close(fd);
    }

  errno = 0;
  fd = open(path, O_RDONLY | O_CLOEXEC);
  diropen_check(t, path, "readonly", fd >= 0, errno);
  if (fd >= 0)
    {
      errno = 0;
      ok = fstat(fd, &buf) == 0 && S_ISDIR(buf.st_mode);
      diropen_check(t, path, "fstat", ok, errno);

      errno = 0;
      fd2 = openat(fd, entry, O_RDONLY | O_CLOEXEC);
      diropen_check(t, path, "openat", fd2 >= 0, errno);
      if (fd2 >= 0)
        {
          close(fd2);
        }

      errno = 0;
      dirp = fdopendir(fd);
      ok = dirp != NULL && diropen_finds(dirp, entry);
      diropen_check(t, path, "readdir", ok, errno);
      if (dirp != NULL)
        {
          closedir(dirp);
        }
      else
        {
          close(fd);
        }
    }

  errno = 0;
  fd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  dirp = fd < 0 ? NULL : fdopendir(fd);
  ok = dirp != NULL && diropen_finds(dirp, entry);
  diropen_check(t, path, "odir", ok, errno);
  if (dirp != NULL)
    {
      closedir(dirp);
    }
  else if (fd >= 0)
    {
      close(fd);
    }
}

/* The cases for the file path */

static void diropen_notdir(FAR struct diropen_s *t, FAR const char *path)
{
  FAR DIR *dirp;
  int fd;

  errno = 0;
  fd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  diropen_check(t, path, "notdir", fd < 0 && errno == ENOTDIR, errno);
  if (fd >= 0)
    {
      close(fd);
    }

  errno = 0;
  dirp = opendir(path);
  diropen_check(t, path, "opendir", dirp == NULL && errno == ENOTDIR,
                errno);
  if (dirp != NULL)
    {
      closedir(dirp);
    }
}

/* The cases in a new directory in the writable directory base */

static void diropen_fs(FAR struct diropen_s *t, FAR const char *base)
{
  char path[PATH_MAX];
  char file[PATH_MAX + 4];
  bool ok;
  int fd = -1;

  snprintf(path, sizeof(path), "%s/diropen.%d", base, getpid());
  snprintf(file, sizeof(file), "%s/a", path);

  errno = 0;
  if (mkdir(path, 0777) == 0)
    {
      fd = open(file, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
    }

  if (fd < 0 || write(fd, DIROPEN_DATA, DIROPEN_DATALEN) != DIROPEN_DATALEN)
    {
      diropen_check(t, base, "prepare", false, errno);
      if (fd >= 0)
        {
          close(fd);
        }

      unlink(file);
      rmdir(path);
      return;
    }

  close(fd);

  diropen_dir(t, path, "a");
  diropen_notdir(t, file);

  errno = 0;
  fd = open(file, O_RDONLY | O_CLOEXEC);
  ok = fd >= 0 && diropen_reads(fd);
  diropen_check(t, file, "read", ok, errno);
  if (fd >= 0)
    {
      close(fd);
    }

  errno = 0;
  fd = open(file, O_WRONLY | O_APPEND | O_CLOEXEC);
  ok = fd >= 0 && write(fd, "!", 1) == 1;
  diropen_check(t, file, "append", ok, errno);
  if (fd >= 0)
    {
      close(fd);
    }

  errno = 0;
  ok = unlink(file) == 0 && rmdir(path) == 0;
  diropen_check(t, path, "remove", ok, errno);
}

/* The cases on the pseudo file system */

static void diropen_pseudo(FAR struct diropen_s *t)
{
  char path[32];
  char sub[48];

  diropen_dir(t, "/dev", "null");
  diropen_dir(t, "/", "dev");
  diropen_notdir(t, "/dev/null");

  /* A new directory, when the pseudo file system can make one */

  snprintf(path, sizeof(path), "/diropen.%d", getpid());
  snprintf(sub, sizeof(sub), "%s/sub", path);
  if (mkdir(path, 0777) == 0 && mkdir(sub, 0777) == 0)
    {
      diropen_dir(t, path, "sub");
    }
  else
    {
      diropen_skip(t, path, "mkdir", errno);
    }

  rmdir(sub);
  rmdir(path);
}

/* The cases on a tmpfs, when the kernel has one */

static void diropen_tmpfs(FAR struct diropen_s *t)
{
  if (mount(NULL, DIROPEN_TMPFS, "tmpfs", 0, NULL) < 0)
    {
      diropen_skip(t, DIROPEN_TMPFS, "mount tmpfs", errno);
      return;
    }

  diropen_fs(t, DIROPEN_TMPFS);
  umount(DIROPEN_TMPFS);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct diropen_s t;
  int i;

  memset(&t, 0, sizeof(t));

  diropen_pseudo(&t);
  diropen_tmpfs(&t);

  /* The other file systems, for example a hostfs, v9fs or rpmsgfs mount */

  for (i = 1; i < argc; i++)
    {
      diropen_fs(&t, argv[i]);
    }

  printf("diropen: DONE pass %d fail %d skip %d\n", t.pass, t.fail, t.skip);
  return t.fail == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
