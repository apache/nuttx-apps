/****************************************************************************
 * apps/testing/fstest/fstest_main.c
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

#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/statfs.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <crc32.h>
#include <debug.h>
#include <assert.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_TESTING_FSTEST_MAXNAME
#  define CONFIG_TESTING_FSTEST_MAXNAME 128
#endif

#if CONFIG_TESTING_FSTEST_MAXNAME > 255
#  undef CONFIG_TESTING_FSTEST_MAXNAME
#  define CONFIG_TESTING_FSTEST_MAXNAME 255
#endif

#ifndef CONFIG_TESTING_FSTEST_MAXFILE
#  define CONFIG_TESTING_FSTEST_MAXFILE 8192
#endif

#ifndef CONFIG_TESTING_FSTEST_MAXIO
#  define CONFIG_TESTING_FSTEST_MAXIO 347
#endif

#ifndef CONFIG_TESTING_FSTEST_MAXOPEN
#  define CONFIG_TESTING_FSTEST_MAXOPEN 512
#endif

#ifndef CONFIG_TESTING_FSTEST_MOUNTPT
#  error CONFIG_TESTING_FSTEST_MOUNTPT must be provided
#endif

#ifndef CONFIG_TESTING_FSTEST_NLOOPS
#  define CONFIG_TESTING_FSTEST_NLOOPS 100
#endif

#ifndef CONFIG_TESTING_FSTEST_VERBOSE
#  define CONFIG_TESTING_FSTEST_VERBOSE 0
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fstest_filedesc_s
{
  FAR char *name;
  bool deleted;
  bool failed;
  size_t len;
  uint32_t crc;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Pre-allocated simulated flash */

struct fstest_ctx_s
{
  uint8_t fileimage[CONFIG_TESTING_FSTEST_MAXFILE];
  struct fstest_filedesc_s files[CONFIG_TESTING_FSTEST_MAXOPEN];
  char mountdir[CONFIG_TESTING_FSTEST_MAXNAME];
  int nfiles;
  int ndeleted;
  int nfailed;
  struct mallinfo mmbefore;
  struct mallinfo mmprevious;
  struct mallinfo mmafter;
};
/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fstest_memusage
 ****************************************************************************/

static void fstest_showmemusage(struct mallinfo *mmbefore,
                                struct mallinfo *mmafter)
{
  printf("VARIABLE  BEFORE   AFTER    DELTA\n");
  printf("======== ======== ======== ========\n");
  printf("arena    %8x %8x %8x\n", mmbefore->arena   , mmafter->arena,
                                   mmafter->arena    - mmbefore->arena);
  printf("ordblks  %8d %8d %8d\n", mmbefore->ordblks , mmafter->ordblks,
                                   mmafter->ordblks  - mmbefore->ordblks);
  printf("mxordblk %8x %8x %8x\n", mmbefore->mxordblk, mmafter->mxordblk,
                                   mmafter->mxordblk - mmbefore->mxordblk);
  printf("uordblks %8x %8x %8x\n", mmbefore->uordblks, mmafter->uordblks,
                                   mmafter->uordblks - mmbefore->uordblks);
  printf("fordblks %8x %8x %8x\n", mmbefore->fordblks, mmafter->fordblks,
                                   mmafter->fordblks - mmbefore->fordblks);
}

/****************************************************************************
 * Name: fstest_loopmemusage
 ****************************************************************************/

static void fstest_loopmemusage(FAR struct fstest_ctx_s *ctx)
{
  /* Get the current memory usage */

  ctx->mmafter = mallinfo();

  /* Show the change from the previous loop */

  printf("\nEnd of loop memory usage:\n");
  fstest_showmemusage(&ctx->mmprevious, &ctx->mmafter);

  /* Set up for the next test */

  ctx->mmprevious = ctx->mmafter;
}

/****************************************************************************
 * Name: fstest_endmemusage
 ****************************************************************************/

static void fstest_endmemusage(FAR struct fstest_ctx_s *ctx)
{
  ctx->mmafter = mallinfo();

  printf("\nFinal memory usage:\n");
  fstest_showmemusage(&ctx->mmbefore, &ctx->mmafter);
}

/****************************************************************************
 * Name: fstest_randchar
 ****************************************************************************/

static inline char fstest_randchar(void)
{
  int value = rand() % 63;
  if (value == 0)
    {
      return '0';
    }
  else if (value <= 10)
    {
      return value + '0' - 1;
    }
  else if (value <= 36)
    {
      return value + 'a' - 11;
    }
  else /* if (value <= 62) */
    {
      return value + 'A' - 37;
    }
}

/****************************************************************************
 * Name: fstest_checkexist
 ****************************************************************************/

static bool fstest_checkexist(FAR struct fstest_ctx_s *ctx,
                              FAR struct fstest_filedesc_s *file)
{
  int i;
  bool ret = false;

  for (i = 0; i < CONFIG_TESTING_FSTEST_MAXOPEN; i++)
    {
      if (&ctx->files[i] != file && ctx->files[i].name &&
          strcmp(ctx->files[i].name, file->name) == 0)
        {
          ret = true;
          break;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: fstest_randname
 ****************************************************************************/

static inline void fstest_randname(FAR struct fstest_ctx_s *ctx,
                                   FAR struct fstest_filedesc_s *file)
{
  int dirlen;
  int maxname;
  int namelen;
  int alloclen;
  int i;

  dirlen   = strlen(ctx->mountdir);

  /* Force the max filename lengh and also the min name len = 4 */

  maxname  = CONFIG_TESTING_FSTEST_MAXNAME - dirlen - 3;
  namelen  = (rand() % maxname) + 4;
  alloclen = namelen + dirlen;

  file->name = (FAR char *)malloc(alloclen + 1);
  if (!file->name)
    {
      printf("ERROR: Failed to allocate name, length=%d\n", namelen);
      fflush(stdout);
      exit(5);
    }

  memcpy(file->name, ctx->mountdir, dirlen);

  do
    {
      for (i = dirlen; i < alloclen; i++)
        {
          file->name[i] = fstest_randchar();
        }

      file->name[alloclen] = '\0';
    }
  while (fstest_checkexist(ctx, file));
}

/****************************************************************************
 * Name: fstest_randfile
 ****************************************************************************/

static inline void fstest_randfile(FAR struct fstest_ctx_s *ctx,
                                   FAR struct fstest_filedesc_s *file)
{
  int i;

  file->len = (rand() % CONFIG_TESTING_FSTEST_MAXFILE) + 1;
  for (i = 0; i < file->len; i++)
    {
      ctx->fileimage[i] = fstest_randchar();
    }

  file->crc = crc32(ctx->fileimage, file->len);
}

/****************************************************************************
 * Name: fstest_freefile
 ****************************************************************************/

static void fstest_freefile(FAR struct fstest_filedesc_s *file)
{
  if (file->name)
    {
      free(file->name);
      file->name = NULL;
    }

  memset(file, 0, sizeof(struct fstest_filedesc_s));
}

/****************************************************************************
 * Name: fstest_gc and fstest_gc_withfd
 ****************************************************************************/

#ifdef CONFIG_TESTING_FSTEST_SPIFFS
static int fstest_gc_withfd(int fd, size_t nbytes)
{
  int ret;

#ifdef CONFIG_SPIFFS_DUMP
  /* Dump the logic content of FLASH before garbage collection */

  printf("SPIFFS Content (before GC):\n");

  ret = ioctl(fd, FIOC_DUMP, (unsigned long)nbytes);
  if (ret < 0)
    {
      printf("ERROR: ioctl(FIOC_DUMP) failed: %d\n", errno);
    }
#endif

  /* Perform SPIFFS garbage collection */

  printf("SPIFFS Garbage Collection:  %lu bytes\n", (unsigned long)nbytes);

  ret = ioctl(fd, FIOC_OPTIMIZE, (unsigned long)nbytes);
  if (ret < 0)
    {
      int ret2;

      printf("ERROR: ioctl(FIOC_OPTIMIZE) failed: %d\n", errno);
      printf("SPIFFS Integrity Test\n");

      ret2 = ioctl(fd, FIOC_INTEGRITY, 0);
      if (ret2 < 0)
        {
          printf("ERROR: ioctl(FIOC_INTEGRITY) failed: %d\n", errno);
        }
    }
  else
    {
      /* Check the integrity of the SPIFFS file system */

      printf("SPIFFS Integrity Test\n");

      ret = ioctl(fd, FIOC_INTEGRITY, 0);
      if (ret < 0)
        {
          printf("ERROR: ioctl(FIOC_INTEGRITY) failed: %d\n", errno);
        }
    }

#ifdef CONFIG_SPIFFS_DUMP
  /* Dump the logic content of FLASH after garbage collection */

  printf("SPIFFS Content (After GC):\n");

  ret = ioctl(fd, FIOC_DUMP, (unsigned long)nbytes);
  if (ret < 0)
    {
      printf("ERROR: ioctl(FIOC_DUMP) failed: %d\n", errno);
    }
#endif

  return ret;
}

static int fstest_gc(FAR struct fstest_ctx_s *ctx, size_t nbytes)
{
  FAR struct fstest_filedesc_s *file;
  int ret = OK;
  int fd;
  int i;

  /* Find the first valid file */

  for (i = 0; i < CONFIG_TESTING_FSTEST_MAXOPEN; i++)
    {
      file = &ctx->files[i];
      if (file->name != NULL && !file->deleted)
        {
          /* Open the file for reading */

          fd = open(file->name, O_RDONLY);
          if (fd < 0)
            {
              printf("ERROR: Failed to open file for reading: %d\n", errno);
              ret = ERROR;
            }
          else
            {
              /* Use this file descriptor to support the garbage collection */

              ret = fstest_gc_withfd(fd, nbytes);
              close(fd);
            }

          break;
        }
    }

  return ret;
}
#else
#  define fstest_gc_withfd(f,n) (-ENOSYS)
#  define fstest_gc(n)          (-ENOSYS)
#endif

/****************************************************************************
 * Name: fstest_wrfile
 ****************************************************************************/

static inline int fstest_wrfile(FAR struct fstest_ctx_s *ctx,
                                FAR struct fstest_filedesc_s *file)
{
  size_t offset;
  int fd;
  int ret;

  /* Create a random file */

  fstest_randname(ctx, file);
  fstest_randfile(ctx, file);

  fd = open(file->name, O_WRONLY | O_CREAT | O_EXCL | O_TRUNC, 0666);
  if (fd < 0)
    {
      /* If it failed because there is no space on the device, then don't
       * complain.
       */

      if (errno != ENOSPC)
        {
          printf("ERROR: Failed to open file for writing: %d\n", errno);
          printf("  File name: %s\n", file->name);
          printf("  File size: %zd\n", file->len);
        }

      fstest_freefile(file);
      return ERROR;
    }

  /* Write a random amount of data to the file */

  for (offset = 0; offset < file->len; )
    {
      size_t maxio = (rand() % CONFIG_TESTING_FSTEST_MAXIO) + 1;
      size_t nbytestowrite = file->len - offset;
      ssize_t nbyteswritten;

      if (nbytestowrite > maxio)
        {
          nbytestowrite = maxio;
        }

      nbyteswritten = write(fd, &ctx->fileimage[offset], nbytestowrite);
      if (nbyteswritten < 0)
        {
          int errcode = errno;

          /* If the write failed because an interrupt occurred or because
           * there is no space on the device, then don't complain.
           */

          if (errcode == EINTR)
            {
              continue;
            }
          else if (errcode != ENOSPC)
            {
              printf("ERROR: Failed to write file: %d\n", errcode);
              printf("  File name:    %s\n", file->name);
              printf("  File size:    %zd\n", file->len);
              printf("  Write offset: %ld\n", (long)offset);
              printf("  Write size:   %ld\n", (long)nbytestowrite);
            }

          close(fd);

          /* Remove any garbage file that might have been left behind */

          ret = unlink(file->name);
          if (ret < 0)
            {
              printf("  Failed to remove partial file\n");
            }
          else
            {
#if CONFIG_TESTING_FSTEST_VERBOSE != 0
              printf("  Successfully removed partial file\n");
#endif
            }

          fstest_freefile(file);
          return ERROR;
        }
      else if (nbyteswritten != nbytestowrite)
        {
          printf("ERROR: Partial write:\n");
          printf("  File name:    %s\n", file->name);
          printf("  File size:    %zd\n", file->len);
          printf("  Write offset: %ld\n", (long)offset);
          printf("  Write size:   %ld\n", (long)nbytestowrite);
          printf("  Written:      %ld\n", (long)nbyteswritten);
        }

      offset += nbyteswritten;
    }

  close(fd);
  return OK;
}

/****************************************************************************
 * Name: fstest_fillfs
 ****************************************************************************/

static int fstest_fillfs(FAR struct fstest_ctx_s *ctx)
{
  FAR struct fstest_filedesc_s *file;
  int ret;
  int i;

  /* Create a file for each unused file structure */

  for (i = 0; i < CONFIG_TESTING_FSTEST_MAXOPEN; i++)
    {
      file = &ctx->files[i];
      if (file->name == NULL)
        {
          ret = fstest_wrfile(ctx, file);
          if (ret < 0)
            {
#if CONFIG_TESTING_FSTEST_VERBOSE != 0
              printf("ERROR: Failed to write file %d\n", i);
#endif
              return ERROR;
            }

#if CONFIG_TESTING_FSTEST_VERBOSE != 0
          printf("  Created file %s\n", file->name);
#endif
          ctx->nfiles++;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: fstest_rdblock
 ****************************************************************************/

static ssize_t fstest_rdblock(FAR struct fstest_ctx_s *ctx, int fd,
                              FAR struct fstest_filedesc_s *file,
                              size_t offset, size_t len)
{
  size_t maxio = (rand() % CONFIG_TESTING_FSTEST_MAXIO) + 1;
  ssize_t nbytesread;

  if (len > maxio)
    {
      len = maxio;
    }

  for (; ; )
    {
      nbytesread = read(fd, &ctx->fileimage[offset], len);
      if (nbytesread < 0)
        {
          int errcode = errno;

          if (errcode == EINTR)
            {
              continue;
            }
          else
            {
              printf("ERROR: Failed to read file: %d\n", errno);
              printf("  File name:    %s\n", file->name);
              printf("  File size:    %zd\n", file->len);
              printf("  Read offset:  %ld\n", (long)offset);
              printf("  Read size:    %ld\n", (long)len);
              return ERROR;
            }
        }
      else if (nbytesread == 0)
        {
#if 0 /* No... we do this on purpose sometimes */
          printf("ERROR: Unexpected end-of-file:\n");
          printf("  File name:    %s\n", file->name);
          printf("  File size:    %zd\n", file->len);
          printf("  Read offset:  %ld\n", (long)offset);
          printf("  Read size:    %ld\n", (long)len);
#endif
          return ERROR;
        }
      else if (nbytesread != len)
        {
          printf("ERROR: Partial read:\n");
          printf("  File name:    %s\n", file->name);
          printf("  File size:    %zd\n", file->len);
          printf("  Read offset:  %ld\n", (long)offset);
          printf("  Read size:    %ld\n", (long)len);
          printf("  Bytes read:   %ld\n", (long)nbytesread);
        }

      return nbytesread;
    }
}

/****************************************************************************
 * Name: fstest_rdfile
 ****************************************************************************/

static inline int fstest_rdfile(FAR struct fstest_ctx_s *ctx,
                                FAR struct fstest_filedesc_s *file)
{
  size_t ntotalread;
  ssize_t nbytesread;
  uint32_t crc;
  int fd;

  /* Open the file for reading */

  fd = open(file->name, O_RDONLY);
  if (fd < 0)
    {
      if (!file->deleted)
        {
          printf("ERROR: Failed to open file for reading: %d\n", errno);
          printf("  File name: %s\n", file->name);
          printf("  File size: %zd\n", file->len);
        }

      return ERROR;
    }

  /* Read all of the data info the file image buffer using random read
   * sizes.
   */

  for (ntotalread = 0; ntotalread < file->len; )
    {
      nbytesread = fstest_rdblock(ctx, fd, file, ntotalread,
                                  file->len - ntotalread);
      if (nbytesread < 0)
        {
          close(fd);
          return ERROR;
        }

      ntotalread += nbytesread;
    }

  /* Verify the file image CRC */

  crc = crc32(ctx->fileimage, file->len);
  if (crc != file->crc)
    {
      printf("ERROR: Bad CRC: %" PRId32 " vs %" PRId32 "\n", crc, file->crc);
      printf("  File name: %s\n", file->name);
      printf("  File size: %zd\n", file->len);
      close(fd);
      return ERROR;
    }

  /* Try reading past the end of the file */

  nbytesread = fstest_rdblock(ctx, fd, file, ntotalread, 1024);
  if (nbytesread > 0)
    {
      printf("ERROR: Read past the end of file\n");
      printf("  File name:  %s\n", file->name);
      printf("  File size:  %zd\n", file->len);
      printf("  Bytes read: %ld\n", (long)nbytesread);
      close(fd);
      return ERROR;
    }

  close(fd);
  return OK;
}

/****************************************************************************
 * Name: fstest_filesize
 ****************************************************************************/

#ifdef CONFIG_HAVE_LONG_LONG
static unsigned long long fstest_filesize(FAR struct fstest_ctx_s *ctx)
{
  unsigned long long bytes_used;
  FAR struct fstest_filedesc_s *file;
  int i;

  bytes_used = 0;

  for (i = 0; i < CONFIG_TESTING_FSTEST_MAXOPEN; i++)
    {
      file = &ctx->files[i];
      if (file->name != NULL && !file->deleted)
        {
          bytes_used += file->len;
        }
    }

  return bytes_used;
}
#else
static unsigned long fstest_filesize(FAR struct fstest_ctx_s *ctx)
{
  unsigned long bytes_used;
  FAR struct fstest_filedesc_s *file;
  int i;

  bytes_used = 0;

  for (i = 0; i < CONFIG_TESTING_FSTEST_MAXOPEN; i++)
    {
      file = &ctx->files[i];
      if (file->name != NULL && !file->deleted)
        {
          bytes_used += file->len;
        }
    }

  return bytes_used;
}
#endif

/****************************************************************************
 * Name: fstest_verifyfs
 ****************************************************************************/

static int fstest_verifyfs(FAR struct fstest_ctx_s *ctx)
{
  FAR struct fstest_filedesc_s *file;
  int ret;
  int i;

  /* Create a file for each unused file structure */

  for (i = 0; i < CONFIG_TESTING_FSTEST_MAXOPEN; i++)
    {
      file = &ctx->files[i];
      if (file->name != NULL)
        {
          ret = fstest_rdfile(ctx, file);
          if (ret < 0)
            {
              if (file->deleted)
                {
#if CONFIG_TESTING_FSTEST_VERBOSE != 0
                  printf("Deleted file %d OK\n", i);
#endif
                  fstest_freefile(file);
                  ctx->ndeleted--;
                  ctx->nfiles--;
                }
              else
                {
                  printf("ERROR: Failed to read a file: %d\n", i);
                  printf("  File name: %s\n", file->name);
                  printf("  File size: %zd\n", file->len);
                  return ERROR;
                }
            }
          else
            {
              if (file->deleted)
                {
#if CONFIG_TESTING_FSTEST_VERBOSE != 0
                  printf("ERROR: Successfully read a deleted file\n");
                  printf("  File name: %s\n", file->name);
                  printf("  File size: %zd\n", file->len);
#endif
                  fstest_freefile(file);
                  ctx->ndeleted--;
                  ctx->nfiles--;
                  return ERROR;
                }
              else
                {
#if CONFIG_TESTING_FSTEST_VERBOSE != 0
                  printf("  Verified file %s\n", file->name);
#endif
                }
            }
        }
    }

  return OK;
}

/****************************************************************************
 * Name: fstest_delfiles
 ****************************************************************************/

static int fstest_delfiles(FAR struct fstest_ctx_s *ctx)
{
  FAR struct fstest_filedesc_s *file;
  int ndel;
  int ret;
  int i;
  int j;

  /* Are there any files to be deleted? */

  int nfiles = ctx->nfiles - ctx->ndeleted - ctx->nfailed;
  if (nfiles <= 1)
    {
      return 0;
    }

  /* Yes... How many files should we delete? */

  ndel = (rand() % nfiles) + 1;

  /* Now pick which files to delete */

  for (i = 0; i < ndel; i++)
    {
      /* Guess a file index */

      int ndx = (rand() % (ctx->nfiles - ctx->ndeleted));

      /* And delete the next undeleted file after that random index.  NOTE
       * that the entry at ndx is not checked.
       */

      for (j = ndx + 1; j != ndx; j++)
        {
          /* Test for wrap-around */

          if (j >= CONFIG_TESTING_FSTEST_MAXOPEN)
            {
              j = 0;
            }

          file = &ctx->files[j];
          if (file->name && !file->deleted)
            {
              ret = unlink(file->name);
              if (ret < 0)
                {
                  printf("ERROR: Unlink %d failed: %d\n", i + 1, errno);
                  printf("  File name:  %s\n", file->name);
                  printf("  File size:  %zd\n", file->len);
                  printf("  File index: %d\n", j);

                  /* If we don't do this we can get stuck in an infinite
                   * loop on certain failures to unlink a file.
                   */

                  file->failed = true;
                  ctx->nfailed++;
                  nfiles--;

                  if (nfiles < 1)
                    {
                      return ret;
                    }
                }
              else
                {
#if CONFIG_TESTING_FSTEST_VERBOSE != 0
                  printf("  Deleted file %s\n", file->name);
#endif
                  file->deleted = true;
                  ctx->ndeleted++;
                  break;
                }
            }
        }
    }

  return OK;
}

/****************************************************************************
 * Name: fstest_delallfiles
 ****************************************************************************/

static int fstest_delallfiles(FAR struct fstest_ctx_s *ctx)
{
  FAR struct fstest_filedesc_s *file;
  int ret;
  int i;

  for (i = 0; i < CONFIG_TESTING_FSTEST_MAXOPEN; i++)
    {
      file = &ctx->files[i];
      if (file->name)
        {
          ret = unlink(file->name);
          if (ret < 0)
            {
               printf("ERROR: Unlink %d failed: %d\n", i + 1, errno);
               printf("  File name:  %s\n", file->name);
               printf("  File size:  %zd\n", file->len);
               printf("  File index: %d\n", i);
            }
          else
            {
#if CONFIG_TESTING_FSTEST_VERBOSE != 0
              printf("  Deleted file %s\n", file->name);
#endif
              fstest_freefile(file);
            }
        }
    }

  ctx->nfiles = 0;
  ctx->nfailed = 0;
  ctx->ndeleted = 0;
  return OK;
}

/****************************************************************************
 * Name: fstest_directory
 ****************************************************************************/

static int fstest_directory(FAR struct fstest_ctx_s *ctx)
{
  DIR *dirp;
  FAR struct dirent *entryp;
  int number;

  /* Open the directory */

  dirp = opendir(ctx->mountdir);

  if (!dirp)
    {
      /* Failed to open the directory */

      printf("ERROR: Failed to open directory '%s': %d\n",
             ctx->mountdir, errno);
      return ERROR;
    }

  /* Read each directory entry */

  printf("Directory:\n");
  number = 1;
  do
    {
      entryp = readdir(dirp);
      if (entryp)
        {
          printf("%2d. Type[%d]: %s Name: %s\n",
                 number, entryp->d_type,
                 entryp->d_type == DTYPE_FILE ? "File " : "Error",
                 entryp->d_name);
        }

      number++;
    }
  while (entryp != NULL);

  closedir(dirp);
  return OK;
}

/****************************************************************************
 * Show help Message
 ****************************************************************************/

static void show_useage(void)
{
  printf("Usage : fstest [OPTION [ARG]] ...\n");
  printf("-h    show this help statement\n");
  printf("-n    num of test loop e.g. [%d]\n", CONFIG_TESTING_FSTEST_NLOOPS);
  printf("-m    mount point to be tested e.g. [%s]\n",
          CONFIG_TESTING_FSTEST_MOUNTPT);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fstest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct fstest_ctx_s *ctx;
  struct statfs buf;
  unsigned int i;
  int ret;
  int loop_num;
  int option;

  ctx = malloc(sizeof(struct fstest_ctx_s));
  if (ctx == NULL)
    {
      printf("malloc ctx feild,exit!\n");
      exit(1);
    }

  memset(ctx, 0, sizeof(struct fstest_ctx_s));

  /* Seed the random number generated */

  srand(0x93846);
  loop_num = CONFIG_TESTING_FSTEST_NLOOPS;
  strcpy(ctx->mountdir, CONFIG_TESTING_FSTEST_MOUNTPT);

  /* Opt Parse */

  while ((option = getopt(argc, argv, ":m:hn:")) != -1)
    {
      switch (option)
        {
          case 'm':
            strcpy(ctx->mountdir, optarg);
            break;
          case 'h':
            show_useage();
            free(ctx);
            exit(0);
          case 'n':
            loop_num = atoi(optarg);
            break;
          case ':':
            printf("Error: Missing required argument\n");
            free(ctx);
            exit(1);
          case '?':
            printf("Error: Unrecognized option\n");
            free(ctx);
            exit(1);
        }
    }

  if (ctx->mountdir[strlen(ctx->mountdir)-1] != '/')
    {
      strcat(ctx->mountdir, "/");
    }

  /* Set up memory monitoring */

  ctx->mmbefore = mallinfo();
  ctx->mmprevious = ctx->mmbefore;

  /* Loop a few times ... file the file system with some random, files,
   * delete some files randomly, fill the file system with more random file,
   * delete, etc.  This beats the FLASH very hard!
   */

#if CONFIG_TESTING_FSTEST_NLOOPS == 0
  for (i = 0; ; i++)
#else
  for (i = 1; i <= loop_num; i++)
#endif
    {
      /* Write a files to the file system until either (1) all of the open
       * file structures are utilized or until (2) the file system reports an
       * error (hopefully meaning that the file system is full)
       */

      printf("\n=== FILLING %u =============================\n", i);
      fstest_fillfs(ctx);
      printf("Filled file system\n");
      printf("  Number of files: %d\n", ctx->nfiles);
      printf("  Number deleted:  %d\n", ctx->ndeleted);

      /* Directory listing */

      fstest_directory(ctx);
#ifdef CONFIG_HAVE_LONG_LONG
      printf("Total file size: %llu\n", fstest_filesize(ctx));
#else
      printf("Total file size: %lu\n", fstest_filesize(ctx));
#endif

      /* Verify all files written to FLASH */

      ret = fstest_verifyfs(ctx);
      if (ret < 0)
        {
          printf("ERROR: Failed to verify files\n");
          printf("  Number of files: %d\n", ctx->nfiles);
          printf("  Number deleted:  %d\n", ctx->ndeleted);
          free(ctx);
          exit(ret);
        }
      else
        {
#if CONFIG_TESTING_FSTEST_VERBOSE != 0
          printf("Verified!\n");
          printf("  Number of files: %d\n", ctx->nfiles);
          printf("  Number deleted:  %d\n", ctx->ndeleted);
#endif
        }

      /* Delete some files */

      printf("\n=== DELETING %u ============================\n", i);
      ret = fstest_delfiles(ctx);
      if (ret < 0)
        {
          printf("ERROR: Failed to delete files\n");
          printf("  Number of files: %d\n", ctx->nfiles);
          printf("  Number deleted:  %d\n", ctx->ndeleted);
          free(ctx);
          exit(ret);
        }
      else
        {
          printf("Deleted some files\n");
          printf("  Number of files: %d\n", ctx->nfiles);
          printf("  Number deleted:  %d\n", ctx->ndeleted);
        }

      /* Directory listing */

      fstest_directory(ctx);
#ifdef CONFIG_HAVE_LONG_LONG
      printf("Total file size: %llu\n", fstest_filesize(ctx));
#else
      printf("Total file size: %lu\n", fstest_filesize(ctx));
#endif

      /* Verify all files written to FLASH */

      ret = fstest_verifyfs(ctx);
      if (ret < 0)
        {
          printf("ERROR: Failed to verify files\n");
          printf("  Number of files: %d\n", ctx->nfiles);
          printf("  Number deleted:  %d\n", ctx->ndeleted);
          free(ctx);
          exit(ret);
        }
      else
        {
#if CONFIG_TESTING_FSTEST_VERBOSE != 0
          printf("Verified!\n");
          printf("  Number of files: %d\n", ctx->nfiles);
          printf("  Number deleted:  %d\n", ctx->ndeleted);
#endif
        }

      /* Show file system usage */

      ret = statfs(ctx->mountdir, &buf);
      if (ret < 0)
        {
           printf("ERROR: statfs failed: %d\n", errno);
           free(ctx);
           exit(ret);
        }
      else
        {
           printf("File System:\n");
           printf("  Block Size:      %lu\n", (unsigned long)buf.f_bsize);
           printf("  No. Blocks:      %lu\n", (unsigned long)buf.f_blocks);
           printf("  Free Blocks:     %ld\n", (long)buf.f_bfree);
           printf("  Avail. Blocks:   %ld\n", (long)buf.f_bavail);
           printf("  No. File Nodes:  %ld\n", (long)buf.f_files);
           printf("  Free File Nodes: %ld\n", (long)buf.f_ffree);
        }

      /* Perform garbage collection, integrity checks */

      ret = fstest_gc(buf.f_bfree);
      UNUSED(ret);

      /* Show memory usage */

      fstest_loopmemusage(ctx);
      fflush(stdout);
    }

  /* Delete all files then show memory usage again */

  fstest_delallfiles(ctx);
  fstest_endmemusage(ctx);
  fflush(stdout);
  free(ctx);
  return 0;
}
