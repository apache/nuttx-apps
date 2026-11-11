/****************************************************************************
 * apps/testing/fs/inotest/inotest_main.c
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

/* Check the file serial numbers (st_ino, d_ino) and the device numbers
 * (st_dev) of the entries of some directories:
 *
 * - st_ino of each directory and of each entry is not 0 and does not
 *   change between two calls;
 * - readdir() gives in d_ino the st_ino that lstat() gives, except for a
 *   mount point;
 * - open() and fstat() give the st_dev and st_ino that lstat() gives;
 * - two entries of a directory have different st_dev and st_ino, except
 *   for hard links;
 * - a subdirectory has other st_dev and st_ino than the directory and its
 *   parent directories (else find and ftw() see a loop).
 *
 * The test goes one level into the subdirectories on the same device.  It
 * also makes files in a tmpfs (or in a directory given with -w) and checks
 * that a file keeps its number when it is renamed.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/mount.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define INOTEST_MAXENT   64    /* Entries checked in one directory */
#define INOTEST_MAXPAR   16    /* Parent directories that are compared */
#define INOTEST_DEPTH    1     /* Levels below each directory */
#define INOTEST_NFILES   5     /* Names in inotest_files() */
#define INOTEST_TMPFS    "/inotest"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct inotest_s
{
  int pass;
  int fail;
  int skip;
};

/* The identity of a file */

struct inotest_id_s
{
  dev_t dev;
  ino_t ino;
};

/* The state of one directory of the walk */

struct inotest_dir_s
{
  char path[PATH_MAX];
  struct inotest_id_s ids[INOTEST_MAXENT];
  struct inotest_id_s par[INOTEST_MAXPAR];
  int npar;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void inotest_fail(FAR struct inotest_s *t, FAR const char *path,
                         FAR const char *fmt, ...)
{
  va_list ap;

  printf("inotest: FAIL %s: ", path);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
  t->fail++;
}

static void inotest_check(FAR struct inotest_s *t, bool ok,
                          FAR const char *path, FAR const char *what,
                          unsigned long got, unsigned long want)
{
  if (ok)
    {
      t->pass++;
    }
  else
    {
      inotest_fail(t, path, "%s %lu, want %lu", what, got, want);
    }
}

static void inotest_nonzero(FAR struct inotest_s *t, FAR const char *path,
                            FAR const char *what, unsigned long value)
{
  if (value != 0)
    {
      t->pass++;
    }
  else
    {
      inotest_fail(t, path, "%s 0", what);
    }
}

static bool inotest_same(FAR const struct inotest_id_s *id,
                         FAR const struct stat *st)
{
  return id->dev == st->st_dev && id->ino == st->st_ino;
}

/* lstat() twice: the numbers must not change */

static int inotest_lstat(FAR struct inotest_s *t, FAR const char *path,
                         FAR struct stat *st)
{
  struct stat st2;

  if (lstat(path, st) < 0 || lstat(path, &st2) < 0)
    {
      return -errno;
    }

  inotest_check(t, st2.st_dev == st->st_dev, path, "st_dev again",
                st2.st_dev, st->st_dev);
  inotest_check(t, st2.st_ino == st->st_ino, path, "st_ino again",
                st2.st_ino, st->st_ino);
  return 0;
}

/* open() and fstat() give the numbers of lstat() */

static void inotest_fstat(FAR struct inotest_s *t, FAR const char *path,
                          FAR const struct stat *st)
{
  struct stat fst;
  int fd;

  fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
  if (fd < 0)
    {
      /* A file that this user cannot open */

      t->skip++;
      return;
    }

  if (fstat(fd, &fst) < 0)
    {
      inotest_fail(t, path, "fstat: errno %d", errno);
    }
  else
    {
      inotest_check(t, fst.st_dev == st->st_dev, path, "fstat st_dev",
                    fst.st_dev, st->st_dev);
      inotest_check(t, fst.st_ino == st->st_ino, path, "fstat st_ino",
                    fst.st_ino, st->st_ino);
    }

  close(fd);
}

/* Check the entries of the directory d->path, which has the numbers of
 * dst.  d->par holds the numbers of its parent directories.
 */

static void inotest_walk(FAR struct inotest_s *t,
                         FAR struct inotest_dir_s *d,
                         FAR const struct stat *dst, int depth)
{
  FAR struct inotest_dir_s *sub;
  FAR struct dirent *entry;
  FAR DIR *dirp;
  struct stat st;
  size_t len;
  int nent;
  int ret;
  int i;

  dirp = opendir(d->path);
  if (dirp == NULL && errno == EACCES)
    {
      /* A directory that this user cannot read */

      t->skip++;
      return;
    }
  else if (dirp == NULL)
    {
      inotest_fail(t, d->path, "opendir: errno %d", errno);
      return;
    }

  sub = NULL;
  if (depth > 0)
    {
      sub = malloc(sizeof(*sub));
      if (sub == NULL)
        {
          inotest_fail(t, d->path, "no memory");
          closedir(dirp);
          return;
        }
    }

  len = strlen(d->path);
  nent = 0;
  while (nent < INOTEST_MAXENT && (entry = readdir(dirp)) != NULL)
    {
      if (strcmp(entry->d_name, ".") == 0 ||
          strcmp(entry->d_name, "..") == 0)
        {
          continue;
        }

      snprintf(d->path + len, sizeof(d->path) - len, "%s%s",
               d->path[len - 1] == '/' ? "" : "/", entry->d_name);

      inotest_nonzero(t, d->path, "d_ino", entry->d_ino);

      ret = inotest_lstat(t, d->path, &st);
      if (ret == -ENOENT)
        {
          /* The entry is gone, for example a process that exited */

          t->skip++;
          d->path[len] = '\0';
          continue;
        }
      else if (ret < 0)
        {
          inotest_fail(t, d->path, "lstat: errno %d", -ret);
          d->path[len] = '\0';
          continue;
        }

      /* readdir() of a mount point gives the number of the directory
       * below it, as on Linux, and the mounted volume may have no numbers.
       */

      if (st.st_dev == dst->st_dev)
        {
          inotest_nonzero(t, d->path, "st_ino", st.st_ino);
          inotest_check(t, entry->d_ino == st.st_ino, d->path,
                        "d_ino is not st_ino: d_ino", entry->d_ino,
                        st.st_ino);
        }
      else
        {
          t->skip++;
        }

      /* Two entries are one file only if they are hard links */

      for (i = 0; i < nent; i++)
        {
          if (inotest_same(&d->ids[i], &st) &&
              (S_ISDIR(st.st_mode) || st.st_nlink < 2))
            {
              inotest_fail(t, d->path, "has the numbers of entry %d", i);
              break;
            }
        }

      if (i == nent)
        {
          t->pass++;
        }

      d->ids[nent].dev = st.st_dev;
      d->ids[nent].ino = st.st_ino;
      nent++;

      if (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode))
        {
          inotest_fstat(t, d->path, &st);
        }

      if (S_ISDIR(st.st_mode) && st.st_dev == dst->st_dev)
        {
          /* A subdirectory is not its directory or a parent directory */

          for (i = 0; i < d->npar; i++)
            {
              if (inotest_same(&d->par[i], &st))
                {
                  inotest_fail(t, d->path, "has the numbers of the parent "
                               "directory %d levels up", d->npar - i + 1);
                  break;
                }
            }

          if (i == d->npar && dst->st_ino == st.st_ino)
            {
              inotest_fail(t, d->path, "has the numbers of its directory");
            }
          else if (i == d->npar)
            {
              t->pass++;
            }

          if (sub != NULL && d->npar < INOTEST_MAXPAR)
            {
              strlcpy(sub->path, d->path, sizeof(sub->path));
              memcpy(sub->par, d->par, d->npar * sizeof(d->par[0]));
              sub->par[d->npar].dev = dst->st_dev;
              sub->par[d->npar].ino = dst->st_ino;
              sub->npar = d->npar + 1;
              inotest_walk(t, sub, &st, depth - 1);
            }
        }

      d->path[len] = '\0';
    }

  free(sub);
  closedir(dirp);
}

/* Check a directory and its entries.  It is compared with the directories
 * of its path.
 */

static void inotest_dir(FAR struct inotest_s *t, FAR const char *path,
                        bool optional)
{
  FAR struct inotest_dir_s *d;
  struct stat dst;
  struct stat st;
  FAR char *slash;
  int ret;
  int i;

  d = malloc(sizeof(*d));
  if (d == NULL)
    {
      inotest_fail(t, path, "no memory");
      return;
    }

  printf("inotest: %s\n", path);
  ret = inotest_lstat(t, path, &dst);
  if (ret < 0)
    {
      if (optional && ret == -ENOENT)
        {
          printf("inotest: SKIP %s: not found\n", path);
          t->skip++;
        }
      else
        {
          inotest_fail(t, path, "lstat: errno %d", -ret);
        }

      free(d);
      return;
    }

  inotest_nonzero(t, path, "st_ino", dst.st_ino);

  /* The parent directories, from the root down.  The directory is none of
   * them.
   */

  d->npar = 0;
  strlcpy(d->path, path, sizeof(d->path));
  if (path[0] == '/' && path[1] != '\0' && lstat("/", &st) == 0)
    {
      d->par[d->npar].dev = st.st_dev;
      d->par[d->npar].ino = st.st_ino;
      d->npar++;
    }

  slash = d->path;
  while ((slash = strchr(slash + 1, '/')) != NULL && slash[1] != '\0' &&
         d->npar < INOTEST_MAXPAR - 1)
    {
      *slash = '\0';
      if (lstat(d->path, &st) == 0)
        {
          d->par[d->npar].dev = st.st_dev;
          d->par[d->npar].ino = st.st_ino;
          d->npar++;
        }

      *slash = '/';
    }

  for (i = 0; i < d->npar; i++)
    {
      if (inotest_same(&d->par[i], &dst))
        {
          inotest_fail(t, path, "has the numbers of a parent directory");
          break;
        }
    }

  if (i == d->npar)
    {
      t->pass++;
    }

  inotest_walk(t, d, &dst, INOTEST_DEPTH);
  free(d);
}

/* Make two files and a directory in the directory path and check them */

static void inotest_files(FAR struct inotest_s *t, FAR const char *path)
{
  FAR const char *suffix[INOTEST_NFILES] =
  {
    "", "/a", "/b", "/c", "/d"
  };

  FAR char (*name)[PATH_MAX];
  struct stat st[3];
  struct stat st2;
  int fd;
  int i;
  int j;

  /* name[0] is a new directory, name[1] to name[3] are its entries and
   * name[4] is the new name of name[1].
   */

  name = malloc(INOTEST_NFILES * sizeof(*name));
  if (name == NULL)
    {
      inotest_fail(t, path, "no memory");
      return;
    }

  for (i = 0; i < INOTEST_NFILES; i++)
    {
      snprintf(name[i], sizeof(name[i]), "%s/inotest.%d%s", path, getpid(),
               suffix[i]);
    }

  if (mkdir(name[0], 0777) < 0)
    {
      inotest_fail(t, name[0], "mkdir: errno %d", errno);
      free(name);
      return;
    }

  for (i = 0; i < 2; i++)
    {
      fd = open(name[i + 1], O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0666);
      if (fd < 0)
        {
          inotest_fail(t, name[i + 1], "create: errno %d", errno);
          goto out;
        }

      if (fstat(fd, &st[i]) < 0)
        {
          inotest_fail(t, name[i + 1], "fstat: errno %d", errno);
          close(fd);
          goto out;
        }

      close(fd);
    }

  if (mkdir(name[3], 0777) < 0 || lstat(name[3], &st[2]) < 0)
    {
      inotest_fail(t, name[3], "mkdir: errno %d", errno);
      goto out;
    }

  for (i = 0; i < 3; i++)
    {
      for (j = 0; j < i; j++)
        {
          inotest_check(t, st[i].st_ino != st[j].st_ino, name[i + 1],
                        "st_ino of another file", st[i].st_ino,
                        st[j].st_ino);
        }
    }

  /* The checks of the entries */

  inotest_dir(t, name[0], false);

  /* A file keeps its number when it is renamed */

  if (rename(name[1], name[4]) < 0 || lstat(name[4], &st2) < 0)
    {
      inotest_fail(t, name[4], "rename: errno %d", errno);
    }
  else
    {
      inotest_check(t, st2.st_ino == st[0].st_ino, name[4],
                    "st_ino after rename", st2.st_ino, st[0].st_ino);
    }

out:
  unlink(name[1]);
  unlink(name[2]);
  unlink(name[4]);
  rmdir(name[3]);
  rmdir(name[0]);
  free(name);
}

static void inotest_usage(void)
{
  printf("Usage: inotest [-w dir] [dir ...]\n"
         "  Check st_ino, d_ino and st_dev of the entries of each dir\n"
         "  -w dir: make files in dir and check them\n"
         "  Default: / /dev /proc and files in a new tmpfs\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *wdir = NULL;
  struct inotest_s t;
#ifdef CONFIG_FS_TMPFS
  bool mounted = false;
#endif
  int opt;
  int i;

  memset(&t, 0, sizeof(t));
  while ((opt = getopt(argc, argv, "hw:")) != -1)
    {
      switch (opt)
        {
          case 'w':
            wdir = optarg;
            break;

          default:
            inotest_usage();
            return EXIT_FAILURE;
        }
    }

  if (optind < argc)
    {
      for (i = optind; i < argc; i++)
        {
          inotest_dir(&t, argv[i], false);
        }
    }
  else
    {
#ifdef CONFIG_FS_TMPFS
      if (wdir == NULL)
        {
          if (mount(NULL, INOTEST_TMPFS, "tmpfs", 0, NULL) < 0)
            {
              inotest_fail(&t, INOTEST_TMPFS, "mount: errno %d", errno);
            }
          else
            {
              mounted = true;
              wdir = INOTEST_TMPFS;
            }
        }
#endif

      inotest_dir(&t, "/", false);
      inotest_dir(&t, "/dev", true);
      inotest_dir(&t, "/proc", true);
    }

  if (wdir != NULL)
    {
      inotest_dir(&t, wdir, false);
      inotest_files(&t, wdir);
    }

#ifdef CONFIG_FS_TMPFS
  if (mounted)
    {
      umount(INOTEST_TMPFS);
    }
#endif

  printf("inotest: DONE pass %d fail %d skip %d\n", t.pass, t.fail, t.skip);
  return t.fail == 0 && t.pass > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
