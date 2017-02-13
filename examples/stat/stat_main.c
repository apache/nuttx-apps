/****************************************************************************
 * examples/stat/stat_main.c
 *
 *   Copyright (C) 2008, 2017 Gregory Nutt. All rights reserved.
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

#include <sys/stat.h>
#include <sys/statfs.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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

static void showusage(struct mallinfo *mmbefore, struct mallinfo *mmafter)
{
  printf("VARIABLE  BEFORE   AFTER\n");
  printf("======== ======== ========\n");
  printf("arena    %8x %8x\n", mmbefore->arena,    mmafter->arena);
  printf("ordblks  %8d %8d\n", mmbefore->ordblks,  mmafter->ordblks);
  printf("mxordblk %8x %8x\n", mmbefore->mxordblk, mmafter->mxordblk);
  printf("uordblks %8x %8x\n", mmbefore->uordblks, mmafter->uordblks);
  printf("fordblks %8x %8x\n", mmbefore->fordblks, mmafter->fordblks);
}

static void stepusage(void)
{
  /* Get the current memory usage */

#ifdef CONFIG_CAN_PASS_STRUCTS
  g_mmafter = mallinfo();
#else
  (void)mallinfo(&g_mmafter);
#endif

  /* Show the change from the previous loop */

  printf("\nStep memory usage:\n");
  showusage(&g_mmprevious, &g_mmafter);

  /* Set up for the next test */

#ifdef CONFIG_CAN_PASS_STRUCTS
  g_mmprevious = g_mmafter;
#else
  memcpy(&g_mmprevious, &g_mmafter, sizeof(struct mallinfo));
#endif
}

static void endusage(void)
{
#ifdef CONFIG_CAN_PASS_STRUCTS
  g_mmafter = mallinfo();
#else
  (void)mallinfo(&g_mmafter);
#endif
  printf("\nFinal memory usage:\n");
  showusage(&g_mmbefore, &g_mmafter);
}

static void dump_stat(FAR struct stat *buf)
{
  FAR const char *typename;

  if (S_ISLNK(buf->st_mode))
    {
      typename = "Link";
    }
  else if (S_ISCHR(buf->st_mode))
    {
      typename = "Character driver";
    }
  else if (S_ISDIR(buf->st_mode))
    {
      typename = "Directory";
    }
  else if (S_ISBLK(buf->st_mode))
    {
      typename = "Block driver";
    }
  else if (S_ISREG(buf->st_mode))
    {
      typename = "Regular file";
    }
  else if (S_ISMQ(buf->st_mode))
    {
      typename = "Message queue";
    }
  else if (S_ISSEM(buf->st_mode))
    {
      typename = "Named semaphore";
    }
  else if (S_ISSHM(buf->st_mode))
    {
      typename = "Shared memory";
    }
  else
    {
      typename = "Unknown file type";
    }

  printf("\nstat:\n");
  printf("  st_mode:    %04x\n",   buf->st_mode);
  printf("              %s\n",     typename);
  printf("  st_size:    %llu\n",  (unsigned long long)buf->st_size);
  printf("  st_blksize: %lu\n",   (unsigned long)buf->st_blksize);
  printf("  st_blocks:  %lu\n",   (unsigned long)buf->st_blocks);
  printf("  st_atime:   %08lx\n", (unsigned long)buf->st_atime);
  printf("  st_mtime:   %08lx\n", (unsigned long)buf->st_mtime);
  printf("  st_ctime:   %08lx\n", (unsigned long)buf->st_ctime);
}

static void dump_statfs(FAR struct statfs *buf)
{
  printf("\nstatfs:\n");
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

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int stat_main(int argc, char *argv[])
#endif
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

#ifdef CONFIG_CAN_PASS_STRUCTS
  g_mmbefore = mallinfo();
  g_mmprevious = g_mmbefore;
#else
  (void)mallinfo(&g_mmbefore);
  memcpy(&g_mmprevious, &g_mmbefore, sizeof(struct mallinfo));
#endif

  /* Try stat first */

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
      int fd = open(path, O_RDONLY);
      if (fd < 0)
        {
          int errcode = errno;
          fprintf(stderr,
              "ERROR: open(%s) failed: %d\n",
              path, errcode);
          return EXIT_FAILURE;
        }

      stepusage();

      ret = fstat(fd, &statbuf);
      if (ret < 0)
        {
          int errcode = errno;
          fprintf(stderr,
                  "ERROR: statfs(%s) failed: %d\n",
                  path, errcode);
        }
      else
        {
          dump_stat(&statbuf);
        }
  
      stepusage();
      close(fd);
    }

  endusage();
  return 0;
}
