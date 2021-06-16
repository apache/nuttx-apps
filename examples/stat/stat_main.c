/****************************************************************************
 * apps/examples/stat/stat_main.c
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

#include <sys/stat.h>
#include <sys/statfs.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct mallinfo g_mmbefore;
static struct mallinfo g_mmprevious;
static struct mallinfo g_mmafter;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void showusage(FAR struct mallinfo *mmbefore,
                      FAR struct mallinfo *mmafter, FAR const char *msg)
{
  if (mmbefore->uordblks != mmafter->uordblks)
    {
      printf("\n%s:\n", msg);
      printf("VARIABLE  BEFORE   AFTER\n");
      printf("======== ======== ========\n");
      printf("arena    %8x %8x\n", mmbefore->arena,    mmafter->arena);
      printf("ordblks  %8d %8d\n", mmbefore->ordblks,  mmafter->ordblks);
      printf("mxordblk %8x %8x\n", mmbefore->mxordblk, mmafter->mxordblk);
      printf("uordblks %8x %8x\n", mmbefore->uordblks, mmafter->uordblks);
      printf("fordblks %8x %8x\n", mmbefore->fordblks, mmafter->fordblks);
    }
}

static void stepusage(void)
{
  /* Get the current memory usage */

  g_mmafter = mallinfo();

  /* Show the change from the previous loop */

  showusage(&g_mmprevious, &g_mmafter, "Step memory leak");

  /* Set up for the next test */

  g_mmprevious = g_mmafter;
}

static void endusage(void)
{
  g_mmafter = mallinfo();
  showusage(&g_mmbefore, &g_mmafter, "End-of-test memory leak");
}

static void dump_stat(FAR struct stat *buf)
{
  char details[] = "----------";

  if (S_ISLNK(buf->st_mode))
    {
      details[0] = 'l';  /* Takes precedence over type of the target */
    }
  else if (S_ISBLK(buf->st_mode))
    {
      details[0] = 'b';
    }
  else if (S_ISCHR(buf->st_mode))
    {
      details[0] = 'c';
    }
  else if (S_ISDIR(buf->st_mode))
    {
      details[0] = 'd';
    }
  else if (S_ISMTD(buf->st_mode))
    {
      details[0] = 'f';
    }
  else if (S_ISSHM(buf->st_mode))
    {
      details[0] = 'h';
    }
  else if (S_ISMQ(buf->st_mode))
    {
      details[0] = 'm';
    }
  else if (S_ISSOCK(buf->st_mode))
    {
      details[0] = 'n';
    }
  else if (S_ISSEM(buf->st_mode))
    {
      details[0] = 's';
    }
  else if (!S_ISREG(buf->st_mode))
    {
      details[0] = '?';
    }

  if ((buf->st_mode & S_IRUSR) != 0)
    {
      details[1] = 'r';
    }

  if ((buf->st_mode & S_IWUSR) != 0)
    {
      details[2] = 'w';
    }

  if ((buf->st_mode & S_IXUSR) != 0)
    {
      details[3] = 'x';
    }

  if ((buf->st_mode & S_IRGRP) != 0)
    {
      details[4] = 'r';
    }

  if ((buf->st_mode & S_IWGRP) != 0)
    {
      details[5] = 'w';
    }

  if ((buf->st_mode & S_IXGRP) != 0)
    {
      details[6] = 'x';
    }

  if ((buf->st_mode & S_IROTH) != 0)
    {
      details[7] = 'r';
    }

  if ((buf->st_mode & S_IWOTH) != 0)
    {
      details[8] = 'w';
    }

  if ((buf->st_mode & S_IXOTH) != 0)
    {
      details[9] = 'x';
    }

  printf("stat buffer:\n");
  printf("  st_mode:    %04x      %s\n",   buf->st_mode, details);
  printf("  st_size:    %llu\n",  (unsigned long long)buf->st_size);
  printf("  st_blksize: %lu\n",   (unsigned long)buf->st_blksize);
  printf("  st_blocks:  %lu\n",   (unsigned long)buf->st_blocks);
  printf("  st_atime:   %08lx\n", (unsigned long)buf->st_atime);
  printf("  st_mtime:   %08lx\n", (unsigned long)buf->st_mtime);
  printf("  st_ctime:   %08lx\n", (unsigned long)buf->st_ctime);
}

static void dump_statfs(FAR struct statfs *buf)
{
  printf("statfs buffer:\n");
  printf("  f_type:     %lu\n",   (unsigned long)buf->f_type);
  printf("  f_namelen:  %lu\n",   (unsigned long)buf->f_namelen);
  printf("  f_bsize:    %lu\n",   (unsigned long)buf->f_bsize);
  printf("  f_blocks:   %llu\n",  (unsigned long long)buf->f_blocks);
  printf("  f_bfree:    %llu\n",  (unsigned long long)buf->f_bfree);
  printf("  f_bavail:   %llu\n",  (unsigned long long)buf->f_bavail);
  printf("  f_files:    %llu\n",  (unsigned long long)buf->f_files);
  printf("  f_ffree:    %llu\n",  (unsigned long long)buf->f_ffree);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * stat_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *path;
  struct stat statbuf;
  struct statfs statfsbuf;
  bool isreg;
  int ret;

  /* Argument is expected... the path to the file to test */

  if (argc != 2)
    {
      fprintf(stderr,
              "ERROR: Invalid number of arguments: %d (expected 2)\n",
              argc);
      return EXIT_FAILURE;
    }

  path = argv[1];
  printf("Testing path: \"%s\"\n", path);

  /* Set up memory monitoring */

  g_mmbefore   = mallinfo();
  g_mmprevious = g_mmbefore;

  /* Try stat first */

  printf("\nTest stat(%s)\n", path);
  ret = stat(path, &statbuf);
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr,
              "ERROR: stat(%s) failed: %d\n",
              path, errcode);
      return EXIT_FAILURE;
    }

  dump_stat(&statbuf);
  stepusage();
  isreg = S_ISREG(statbuf.st_mode);

  /* Try statfs */

  printf("\nTest statfs(%s)\n", path);
  ret = statfs(path, &statfsbuf);
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr,
              "ERROR: statfs(%s) failed: %d\n",
              path, errcode);
    }

  dump_statfs(&statfsbuf);
  stepusage();

  /* Try fstat (only if it is a regular file) */

  if (isreg)
    {
      int fd;

      printf("\nOpen(%s)\n", path);
      fd = open(path, O_RDONLY);
      if (fd < 0)
        {
          int errcode = errno;
          fprintf(stderr,
              "ERROR: open(%s) failed: %d\n",
              path, errcode);
          return EXIT_FAILURE;
        }

      /* Try fstat */

      printf("\nTest fstat(%s)\n", path);
      ret = fstat(fd, &statbuf);
      if (ret < 0)
        {
          int errcode = errno;
          fprintf(stderr,
                  "ERROR: fstat(%s) failed: %d\n",
                  path, errcode);
        }
      else
        {
          dump_stat(&statbuf);
        }

      /* Try fstatfs */

      printf("\nTest fstatfs(%s)\n", path);
      ret = fstatfs(fd, &statfsbuf);
      if (ret < 0)
        {
          int errcode = errno;
          fprintf(stderr,
                  "ERROR: fstatfs(%s) failed: %d\n",
                  path, errcode);
        }
      else
        {
          dump_statfs(&statfsbuf);
        }

      close(fd);
      stepusage();
    }

  endusage();
  return 0;
}
