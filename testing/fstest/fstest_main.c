/****************************************************************************
 * testing/fstest/fstest_main.c
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

static uint8_t g_fileimage[CONFIG_TESTING_FSTEST_MAXFILE];
static struct fstest_filedesc_s g_files[CONFIG_TESTING_FSTEST_MAXOPEN];
static const char g_mountdir[] = CONFIG_TESTING_FSTEST_MOUNTPT "/";
static int g_nfiles;
static int g_ndeleted;
static int g_nfailed;
static bool g_media_full;

static struct mallinfo g_mmbefore;
static struct mallinfo g_mmprevious;
static struct mallinfo g_mmafter;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fstest_memusage
 ****************************************************************************/

static void fstest_showmemusage(struct mallinfo *mmbefore,
                                struct mallinfo *mmafter)
{
  printf("VARIABLE  BEFORE   AFTER\n");
  printf("======== ======== ========\n");
  printf("arena    %8x %8x\n", mmbefore->arena,    mmafter->arena);
  printf("ordblks  %8d %8d\n", mmbefore->ordblks,  mmafter->ordblks);
  printf("mxordblk %8x %8x\n", mmbefore->mxordblk, mmafter->mxordblk);
  printf("uordblks %8x %8x\n", mmbefore->uordblks, mmafter->uordblks);
  printf("fordblks %8x %8x\n", mmbefore->fordblks, mmafter->fordblks);
}

/****************************************************************************
 * Name: fstest_loopmemusage
 ****************************************************************************/

static void fstest_loopmemusage(void)
{
  /* Get the current memory usage */

  g_mmafter = mallinfo();

  /* Show the change from the previous loop */

  printf("\nEnd of loop memory usage:\n");
  fstest_showmemusage(&g_mmprevious, &g_mmafter);

  /* Set up for the next test */

  g_mmprevious = g_mmafter;
}

/****************************************************************************
 * Name: fstest_endmemusage
 ****************************************************************************/

static void fstest_endmemusage(void)
{
  g_mmafter = mallinfo();

  printf("\nFinal memory usage:\n");
  fstest_showmemusage(&g_mmbefore, &g_mmafter);
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
 * Name: fstest_randname
 ****************************************************************************/

static inline void fstest_randname(FAR struct fstest_filedesc_s *file)
{
  int dirlen;
  int maxname;
  int namelen;
  int alloclen;
  int i;

  dirlen   = strlen(g_mountdir);
  maxname  = CONFIG_TESTING_FSTEST_MAXNAME - dirlen;
  namelen  = (rand() % maxname) + 1;
  alloclen = namelen + dirlen;

  file->name = (FAR char *)malloc(alloclen + 1);
  if (!file->name)
    {
      printf("ERROR: Failed to allocate name, length=%d\n", namelen);
      fflush(stdout);
      exit(5);
    }

  memcpy(file->name, g_mountdir, dirlen);
  for (i = dirlen; i < alloclen; i++)
    {
      file->name[i] = fstest_randchar();
    }

  file->name[alloclen] = '\0';
}

/****************************************************************************
 * Name: fstest_randfile
 ****************************************************************************/

static inline void fstest_randfile(FAR struct fstest_filedesc_s *file)
{
  int i;

  file->len = (rand() % CONFIG_TESTING_FSTEST_MAXFILE) + 1;
  for (i = 0; i < file->len; i++)
    {
      g_fileimage[i] = fstest_randchar();
    }

  file->crc = crc32(g_fileimage, file->len);
}

/****************************************************************************
 * Name: fstest_freefile
 ****************************************************************************/

static void fstest_freefile(FAR struct fstest_filedesc_s *file)
{
  if (file->name)
    {
      free(file->name);
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

static int fstest_gc(size_t nbytes)
{
  FAR struct fstest_filedesc_s *file;
  int ret = OK;
  int fd;
  int i;

  /* Find the first valid file */

  for (i = 0; i < CONFIG_TESTING_FSTEST_MAXOPEN; i++)
    {
      file = &g_files[i];
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

static inline int fstest_wrfile(FAR struct fstest_filedesc_s *file)
{
  size_t offset;
  int fd;
  int ret;

  /* Create a random file */

  fstest_randname(file);
  fstest_randfile(file);

  fd = open(file->name, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0)
    {
      /* If it failed because there is no space on the device, then don't
       * complain.
       */

      if (errno != ENOSPC)
        {
          printf("ERROR: Failed to open file for writing: %d\n", errno);
          printf("  File name: %s\n", file->name);
          printf("  File size: %d\n", file->len);
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

      nbyteswritten = write(fd, &g_fileimage[offset], nbytestowrite);
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
          else if (errcode == ENOSPC)
            {
              g_media_full = true;
            }
          else
            {
              printf("ERROR: Failed to write file: %d\n", errcode);
              printf("  File name:    %s\n", file->name);
              printf("  File size:    %d\n", file->len);
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
          printf("  File size:    %d\n", file->len);
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

static int fstest_fillfs(void)
{
  FAR struct fstest_filedesc_s *file;
  int ret;
  int i;

  /* Create a file for each unused file structure */

  g_media_full = false;

  for (i = 0; i < CONFIG_TESTING_FSTEST_MAXOPEN; i++)
    {
      file = &g_files[i];
      if (file->name == NULL)
        {
          ret = fstest_wrfile(file);
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
          g_nfiles++;

          if (g_media_full)
            {
              break;
            }
        }
    }

  return OK;
}

/****************************************************************************
 * Name: fstest_rdblock
 ****************************************************************************/

static ssize_t fstest_rdblock(int fd, FAR struct fstest_filedesc_s *file,
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
      nbytesread = read(fd, &g_fileimage[offset], len);
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
              printf("  File size:    %d\n", file->len);
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
          printf("  File size:    %d\n", file->len);
          printf("  Read offset:  %ld\n", (long)offset);
          printf("  Read size:    %ld\n", (long)len);
#endif
          return ERROR;
        }
      else if (nbytesread != len)
        {
          printf("ERROR: Partial read:\n");
          printf("  File name:    %s\n", file->name);
          printf("  File size:    %d\n", file->len);
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

static inline int fstest_rdfile(FAR struct fstest_filedesc_s *file)
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
          printf("  File size: %d\n", file->len);
        }

      return ERROR;
    }

  /* Read all of the data info the file image buffer using random read
   * sizes.
   */

  for (ntotalread = 0; ntotalread < file->len; )
    {
      nbytesread = fstest_rdblock(fd, file, ntotalread,
                                  file->len - ntotalread);
      if (nbytesread < 0)
        {
          close(fd);
          return ERROR;
        }

      ntotalread += nbytesread;
    }

  /* Verify the file image CRC */

  crc = crc32(g_fileimage, file->len);
  if (crc != file->crc)
    {
      printf("ERROR: Bad CRC: %d vs %d\n", crc, file->crc);
      printf("  File name: %s\n", file->name);
      printf("  File size: %d\n", file->len);
      close(fd);
      return ERROR;
    }

  /* Try reading past the end of the file */

  nbytesread = fstest_rdblock(fd, file, ntotalread, 1024) ;
  if (nbytesread > 0)
    {
      printf("ERROR: Read past the end of file\n");
      printf("  File name:  %s\n", file->name);
      printf("  File size:  %d\n", file->len);
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
static unsigned long long fstest_filesize(void)
{
  unsigned long long bytes_used;
  FAR struct fstest_filedesc_s *file;
  int i;

  bytes_used = 0;

  for (i = 0; i < CONFIG_TESTING_FSTEST_MAXOPEN; i++)
    {
      file = &g_files[i];
      if (file->name != NULL && !file->deleted)
        {
          bytes_used += file->len;
        }
    }

  return bytes_used;
}
#else
static unsigned long fstest_filesize(void)
{
  unsigned long bytes_used;
  FAR struct fstest_filedesc_s *file;
  int i;

  bytes_used = 0;

  for (i = 0; i < CONFIG_TESTING_FSTEST_MAXOPEN; i++)
    {
      file = &g_files[i];
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

static int fstest_verifyfs(void)
{
  FAR struct fstest_filedesc_s *file;
  int ret;
  int i;

  /* Create a file for each unused file structure */

  for (i = 0; i < CONFIG_TESTING_FSTEST_MAXOPEN; i++)
    {
      file = &g_files[i];
      if (file->name != NULL)
        {
          ret = fstest_rdfile(file);
          if (ret < 0)
            {
              if (file->deleted)
                {
#if CONFIG_TESTING_FSTEST_VERBOSE != 0
                  printf("Deleted file %d OK\n", i);
#endif
                  fstest_freefile(file);
                  g_ndeleted--;
                  g_nfiles--;
                }
              else
                {
                  printf("ERROR: Failed to read a file: %d\n", i);
                  printf("  File name: %s\n", file->name);
                  printf("  File size: %d\n", file->len);
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
                  printf("  File size: %d\n", file->len);
#endif
                  fstest_freefile(file);
                  g_ndeleted--;
                  g_nfiles--;
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

static int fstest_delfiles(void)
{
  FAR struct fstest_filedesc_s *file;
  int ndel;
  int ret;
  int i;
  int j;

  /* Are there any files to be deleted? */

  int nfiles = g_nfiles - g_ndeleted - g_nfailed;
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

      int ndx = (rand() % (g_nfiles - g_ndeleted));

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

          file = &g_files[j];
          if (file->name && !file->deleted)
            {
              ret = unlink(file->name);
              if (ret < 0)
                {
                  printf("ERROR: Unlink %d failed: %d\n", i + 1, errno);
                  printf("  File name:  %s\n", file->name);
                  printf("  File size:  %d\n", file->len);
                  printf("  File index: %d\n", j);

                  /* If we don't do this we can get stuck in an infinite
                   * loop on certain failures to unlink a file.
                   */

                  file->failed = true;
                  g_nfailed++;
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
                  g_ndeleted++;
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

static int fstest_delallfiles(void)
{
  FAR struct fstest_filedesc_s *file;
  int ret;
  int i;

  for (i = 0; i < CONFIG_TESTING_FSTEST_MAXOPEN; i++)
    {
      file = &g_files[i];
      if (file->name)
        {
          ret = unlink(file->name);
          if (ret < 0)
            {
               printf("ERROR: Unlink %d failed: %d\n", i + 1, errno);
               printf("  File name:  %s\n", file->name);
               printf("  File size:  %d\n", file->len);
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

  g_nfiles = 0;
  g_ndeleted = 0;
  return OK;
}

/****************************************************************************
 * Name: fstest_directory
 ****************************************************************************/

static int fstest_directory(void)
{
  DIR *dirp;
  FAR struct dirent *entryp;
  int number;

  /* Open the directory */

  dirp = opendir(CONFIG_TESTING_FSTEST_MOUNTPT);

  if (!dirp)
    {
      /* Failed to open the directory */

      printf("ERROR: Failed to open directory '%s': %d\n",
             CONFIG_TESTING_FSTEST_MOUNTPT, errno);
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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fstest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct statfs buf;
  unsigned int i;
  int ret;

  /* Seed the random number generated */

  srand(0x93846);

  /* Set up memory monitoring */

  g_mmbefore = mallinfo();
  g_mmprevious = g_mmbefore;

  /* Loop a few times ... file the file system with some random, files,
   * delete some files randomly, fill the file system with more random file,
   * delete, etc.  This beats the FLASH very hard!
   */

#if CONFIG_TESTING_FSTEST_NLOOPS == 0
  for (i = 0; ; i++)
#else
  for (i = 1; i <= CONFIG_TESTING_FSTEST_NLOOPS; i++)
#endif
    {
      /* Write a files to the file system until either (1) all of the open
       * file structures are utilized or until (2) the file system reports an
       * error (hopefully meaning that the file system is full)
       */

      printf("\n=== FILLING %u =============================\n", i);
      fstest_fillfs();
      printf("Filled file system\n");
      printf("  Number of files: %d\n", g_nfiles);
      printf("  Number deleted:  %d\n", g_ndeleted);

      /* Directory listing */

      fstest_directory();
#ifdef CONFIG_HAVE_LONG_LONG
      printf("Total file size: %llu\n", fstest_filesize());
#else
      printf("Total file size: %lu\n", fstest_filesize());
#endif

      /* Verify all files written to FLASH */

      ret = fstest_verifyfs();
      if (ret < 0)
        {
          printf("ERROR: Failed to verify files\n");
          printf("  Number of files: %d\n", g_nfiles);
          printf("  Number deleted:  %d\n", g_ndeleted);
        }
      else
        {
#if CONFIG_TESTING_FSTEST_VERBOSE != 0
          printf("Verified!\n");
          printf("  Number of files: %d\n", g_nfiles);
          printf("  Number deleted:  %d\n", g_ndeleted);
#endif
        }

      /* Delete some files */

      printf("\n=== DELETING %u ============================\n", i);
      ret = fstest_delfiles();
      if (ret < 0)
        {
          printf("ERROR: Failed to delete files\n");
          printf("  Number of files: %d\n", g_nfiles);
          printf("  Number deleted:  %d\n", g_ndeleted);
        }
      else
        {
          printf("Deleted some files\n");
          printf("  Number of files: %d\n", g_nfiles);
          printf("  Number deleted:  %d\n", g_ndeleted);
        }

      /* Directory listing */

      fstest_directory();
#ifdef CONFIG_HAVE_LONG_LONG
      printf("Total file size: %llu\n", fstest_filesize());
#else
      printf("Total file size: %lu\n", fstest_filesize());
#endif

      /* Verify all files written to FLASH */

      ret = fstest_verifyfs();
      if (ret < 0)
        {
          printf("ERROR: Failed to verify files\n");
          printf("  Number of files: %d\n", g_nfiles);
          printf("  Number deleted:  %d\n", g_ndeleted);
        }
      else
        {
#if CONFIG_TESTING_FSTEST_VERBOSE != 0
          printf("Verified!\n");
          printf("  Number of files: %d\n", g_nfiles);
          printf("  Number deleted:  %d\n", g_ndeleted);
#endif
        }

      /* Show file system usage */

      ret = statfs(g_mountdir, &buf);
      if (ret < 0)
        {
           printf("ERROR: statfs failed: %d\n", errno);
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

      fstest_loopmemusage();
      fflush(stdout);
    }

  /* Delete all files then show memory usage again */

  fstest_delallfiles();
  fstest_endmemusage();
  fflush(stdout);
  return 0;
}
