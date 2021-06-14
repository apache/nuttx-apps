/****************************************************************************
 * apps/testing/nxffs/nxffs_main.c
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

#include <nuttx/mtd/mtd.h>
#include <nuttx/fs/nxffs.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* The default is to use the RAM MTD device at drivers/mtd/rammtd.c.  But
 * an architecture-specific MTD driver can be used instead by defining
 * CONFIG_TESTING_NXFFS_ARCHINIT.  In this case, the initialization logic
 * will call nxffs_archinitialize() to obtain the MTD driver instance.
 */

#ifndef CONFIG_TESTING_NXFFS_ARCHINIT

/* This must exactly match the default configuration in
 * drivers/mtd/rammtd.c
 */

#  ifndef CONFIG_RAMMTD_ERASESIZE
#    define CONFIG_RAMMTD_ERASESIZE 4096
#  endif

#  ifndef CONFIG_TESTING_NXFFS_NEBLOCKS
#    define CONFIG_TESTING_NXFFS_NEBLOCKS (32)
#  endif

#  define TESTING_NXFFS_BUFSIZE \
  (CONFIG_RAMMTD_ERASESIZE * CONFIG_TESTING_NXFFS_NEBLOCKS)
#endif

#ifndef CONFIG_TESTING_NXFFS_MAXNAME
#  define CONFIG_TESTING_NXFFS_MAXNAME 128
#endif

#if CONFIG_TESTING_NXFFS_MAXNAME > 255
#  undef CONFIG_TESTING_NXFFS_MAXNAME
#  define CONFIG_TESTING_NXFFS_MAXNAME 255
#endif

#ifndef CONFIG_TESTING_NXFFS_MAXFILE
#  define CONFIG_TESTING_NXFFS_MAXFILE 8192
#endif

#ifndef CONFIG_TESTING_NXFFS_MAXIO
#  define CONFIG_TESTING_NXFFS_MAXIO 347
#endif

#ifndef CONFIG_TESTING_NXFFS_MAXOPEN
#  define CONFIG_TESTING_NXFFS_MAXOPEN 512
#endif

#ifndef CONFIG_TESTING_NXFFS_MOUNTPT
#  define CONFIG_TESTING_NXFFS_MOUNTPT "/mnt/nxffs"
#endif

#ifndef CONFIG_TESTING_NXFFS_NLOOPS
#  define CONFIG_TESTING_NXFFS_NLOOPS 100
#endif

#ifndef CONFIG_TESTING_NXFFS_VERBOSE
#  define CONFIG_TESTING_NXFFS_VERBOSE 0
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct nxffs_filedesc_s
{
  FAR char *name;
  bool deleted;
  size_t len;
  uint32_t crc;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Pre-allocated simulated flash */

#ifndef CONFIG_TESTING_NXFFS_ARCHINIT
static uint8_t g_simflash[TESTING_NXFFS_BUFSIZE];
#endif

static uint8_t g_fileimage[CONFIG_TESTING_NXFFS_MAXFILE];
static struct nxffs_filedesc_s g_files[CONFIG_TESTING_NXFFS_MAXOPEN];
static const char g_mountdir[] = CONFIG_TESTING_NXFFS_MOUNTPT "/";
static int g_nfiles;
static int g_ndeleted;

static struct mallinfo g_mmbefore;
static struct mallinfo g_mmprevious;
static struct mallinfo g_mmafter;

/****************************************************************************
 * External Functions
 ****************************************************************************/

#ifdef CONFIG_TESTING_NXFFS_ARCHINIT
extern FAR struct mtd_dev_s *nxffs_archinitialize(void);
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxffs_memusage
 ****************************************************************************/

static void nxffs_showmemusage(struct mallinfo *mmbefore,
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
 * Name: nxffs_loopmemusage
 ****************************************************************************/

static void nxffs_loopmemusage(void)
{
  /* Get the current memory usage */

  g_mmafter = mallinfo();

  /* Show the change from the previous loop */

  printf("\nEnd of loop memory usage:\n");
  nxffs_showmemusage(&g_mmprevious, &g_mmafter);

  /* Set up for the next test */

  g_mmprevious = g_mmafter;
}

/****************************************************************************
 * Name: nxffs_endmemusage
 ****************************************************************************/

static void nxffs_endmemusage(void)
{
  g_mmafter = mallinfo();

  printf("\nFinal memory usage:\n");
  nxffs_showmemusage(&g_mmbefore, &g_mmafter);
}

/****************************************************************************
 * Name: nxffs_randchar
 ****************************************************************************/

static inline char nxffs_randchar(void)
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
 * Name: nxffs_randname
 ****************************************************************************/

static inline void nxffs_randname(FAR struct nxffs_filedesc_s *file)
{
  int dirlen;
  int maxname;
  int namelen;
  int alloclen;
  int i;

  dirlen   = strlen(g_mountdir);
  maxname  = CONFIG_TESTING_NXFFS_MAXNAME - dirlen;
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
      file->name[i] = nxffs_randchar();
    }

  file->name[alloclen] = '\0';
}

/****************************************************************************
 * Name: nxffs_randfile
 ****************************************************************************/

static inline void nxffs_randfile(FAR struct nxffs_filedesc_s *file)
{
  int i;

  file->len = (rand() % CONFIG_TESTING_NXFFS_MAXFILE) + 1;
  for (i = 0; i < file->len; i++)
    {
      g_fileimage[i] = nxffs_randchar();
    }

  file->crc = crc32(g_fileimage, file->len);
}

/****************************************************************************
 * Name: nxffs_freefile
 ****************************************************************************/

static void nxffs_freefile(FAR struct nxffs_filedesc_s *file)
{
  if (file->name)
    {
      free(file->name);
    }

  memset(file, 0, sizeof(struct nxffs_filedesc_s));
}

/****************************************************************************
 * Name: nxffs_wrfile
 ****************************************************************************/

static inline int nxffs_wrfile(FAR struct nxffs_filedesc_s *file)
{
  size_t offset;
  int fd;
  int ret;

  /* Create a random file */

  nxffs_randname(file);
  nxffs_randfile(file);
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
          printf("  File size: %lu\n", (unsigned long)file->len);
        }

      nxffs_freefile(file);
      return ERROR;
    }

  /* Write a random amount of data to the file */

  for (offset = 0; offset < file->len; )
    {
      size_t maxio = (rand() % CONFIG_TESTING_NXFFS_MAXIO) + 1;
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

          /* If the write failed because there is no space on the device,
           * then don't complain.
           */

          if (errcode != ENOSPC)
            {
              printf("ERROR: Failed to write file: %d\n", errcode);
              printf("  File name:    %s\n", file->name);
              printf("  File size:    %lu\n", (unsigned long)file->len);
              printf("  Write offset: %ld\n", (long)offset);
              printf("  Write size:   %ld\n", (long)nbytestowrite);
              ret = ERROR;
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
#if CONFIG_TESTING_NXFFS_VERBOSE != 0
              printf("  Successfully removed partial file\n");
#endif
            }

          nxffs_freefile(file);
          return ERROR;
        }
      else if (nbyteswritten != nbytestowrite)
        {
          printf("ERROR: Partial write:\n");
          printf("  File name:    %s\n", file->name);
          printf("  File size:    %lu\n", (unsigned long)file->len);
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
 * Name: nxffs_fillfs
 ****************************************************************************/

static int nxffs_fillfs(void)
{
  FAR struct nxffs_filedesc_s *file;
  int ret;
  int i;

  /* Create a file for each unused file structure */

  for (i = 0; i < CONFIG_TESTING_NXFFS_MAXOPEN; i++)
    {
      file = &g_files[i];
      if (file->name == NULL)
        {
          ret = nxffs_wrfile(file);
          if (ret < 0)
            {
#if CONFIG_TESTING_NXFFS_VERBOSE != 0
              printf("ERROR: Failed to write file %d\n", i);
#endif
              return ERROR;
            }

#if CONFIG_TESTING_NXFFS_VERBOSE != 0
         printf("  Created file %s\n", file->name);
#endif
         g_nfiles++;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: nxffs_rdblock
 ****************************************************************************/

static ssize_t nxffs_rdblock(int fd, FAR struct nxffs_filedesc_s *file,
                             size_t offset, size_t len)
{
  size_t maxio = (rand() % CONFIG_TESTING_NXFFS_MAXIO) + 1;
  ssize_t nbytesread;

  if (len > maxio)
    {
      len = maxio;
    }

  nbytesread = read(fd, &g_fileimage[offset], len);
  if (nbytesread < 0)
    {
      printf("ERROR: Failed to read file: %d\n", errno);
      printf("  File name:    %s\n", file->name);
      printf("  File size:    %lu\n", (unsigned long)file->len);
      printf("  Read offset:  %ld\n", (long)offset);
      printf("  Read size:    %ld\n", (long)len);
      return ERROR;
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
      printf("  File size:    %lu\n", (unsigned long)file->len);
      printf("  Read offset:  %ld\n", (long)offset);
      printf("  Read size:    %ld\n", (long)len);
      printf("  Bytes read:   %ld\n", (long)nbytesread);
    }

  return nbytesread;
}

/****************************************************************************
 * Name: nxffs_rdfile
 ****************************************************************************/

static inline int nxffs_rdfile(FAR struct nxffs_filedesc_s *file)
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
          printf("  File size: %lu\n", (unsigned long)file->len);
        }

      return ERROR;
    }

  /* Read all of the data info the fileimage buffer using random read sizes */

  for (ntotalread = 0; ntotalread < file->len; )
    {
      nbytesread = nxffs_rdblock(fd, file, ntotalread,
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
      printf("ERROR: Bad CRC: %u vs %u\n",
             (unsigned int)crc, (unsigned int)file->crc);
      printf("  File name: %s\n", file->name);
      printf("  File size: %lu\n", (unsigned long)file->len);
      close(fd);
      return ERROR;
    }

  /* Try reading past the end of the file */

  nbytesread = nxffs_rdblock(fd, file, ntotalread, 1024) ;
  if (nbytesread > 0)
    {
      printf("ERROR: Read past the end of file\n");
      printf("  File name:  %s\n", file->name);
      printf("  File size:  %lu\n", (unsigned long)file->len);
      printf("  Bytes read: %ld\n", (long)nbytesread);
      close(fd);
      return ERROR;
    }

  close(fd);
  return OK;
}

/****************************************************************************
 * Name: nxffs_verifyfs
 ****************************************************************************/

static int nxffs_verifyfs(void)
{
  FAR struct nxffs_filedesc_s *file;
  int ret;
  int i;

  /* Create a file for each unused file structure */

  for (i = 0; i < CONFIG_TESTING_NXFFS_MAXOPEN; i++)
    {
      file = &g_files[i];
      if (file->name != NULL)
        {
          ret = nxffs_rdfile(file);
          if (ret < 0)
            {
              if (file->deleted)
                {
#if CONFIG_TESTING_NXFFS_VERBOSE != 0
                  printf("Deleted file %d OK\n", i);
#endif
                  nxffs_freefile(file);
                  g_ndeleted--;
                  g_nfiles--;
                }
              else
                {
                  printf("ERROR: Failed to read a file: %d\n", i);
                  printf("  File name: %s\n", file->name);
                  printf("  File size: %lu\n", (unsigned long)file->len);
                  return ERROR;
                }
            }
          else
            {
              if (file->deleted)
                {
#if CONFIG_TESTING_NXFFS_VERBOSE != 0
                  printf("Succesffully read a deleted file\n");
                  printf("  File name: %s\n", file->name);
                  printf("  File size: %d\n", file->len);
#endif
                  nxffs_freefile(file);
                  g_ndeleted--;
                  g_nfiles--;
                  return ERROR;
                }
              else
                {
#if CONFIG_TESTING_NXFFS_VERBOSE != 0
                  printf("  Verifed file %s\n", file->name);
#endif
                }
            }
        }
    }

  return OK;
}

/****************************************************************************
 * Name: nxffs_delfiles
 ****************************************************************************/

static int nxffs_delfiles(void)
{
  FAR struct nxffs_filedesc_s *file;
  int ndel;
  int ret;
  int i;
  int j;

  /* Are there any files to be deleted? */

  int nfiles = g_nfiles - g_ndeleted;
  if (nfiles < 1)
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

          if (j >= CONFIG_TESTING_NXFFS_MAXOPEN)
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
                  printf("  File size:  %lu\n", (unsigned long)file->len);
                  printf("  File index: %d\n", j);
                }
              else
                {
#if CONFIG_TESTING_NXFFS_VERBOSE != 0
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
 * Name: nxffs_delallfiles
 ****************************************************************************/

static int nxffs_delallfiles(void)
{
  FAR struct nxffs_filedesc_s *file;
  int ret;
  int i;

  for (i = 0; i < CONFIG_TESTING_NXFFS_MAXOPEN; i++)
    {
      file = &g_files[i];
      if (file->name)
        {
          ret = unlink(file->name);
          if (ret < 0)
            {
               printf("ERROR: Unlink %d failed: %d\n", i + 1, errno);
               printf("  File name:  %s\n", file->name);
               printf("  File size:  %lu\n", (unsigned long)file->len);
               printf("  File index: %d\n", i);
            }
          else
            {
#if CONFIG_TESTING_NXFFS_VERBOSE != 0
              printf("  Deleted file %s\n", file->name);
#endif
              nxffs_freefile(file);
            }
        }
    }

  g_nfiles = 0;
  g_ndeleted = 0;
  return OK;
}

/****************************************************************************
 * Name: nxffs_directory
 ****************************************************************************/

static int nxffs_directory(void)
{
  DIR *dirp;
  FAR struct dirent *entryp;
  int number;

  /* Open the directory */

  dirp = opendir(CONFIG_TESTING_NXFFS_MOUNTPT);

  if (!dirp)
    {
      /* Failed to open the directory */

      printf("ERROR: Failed to open directory '%s': %d\n",
             CONFIG_TESTING_NXFFS_MOUNTPT, errno);
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
 * Name: nxffs_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct mtd_dev_s *mtd;
  unsigned int i;
  int ret;

  /* Seed the random number generated */

  srand(0x93846);

  /* Create and initialize a RAM MTD device instance */

#ifdef CONFIG_TESTING_NXFFS_ARCHINIT
  mtd = nxffs_archinitialize();
#else
  mtd = rammtd_initialize(g_simflash, TESTING_NXFFS_BUFSIZE);
#endif
  if (!mtd)
    {
      printf("ERROR: Failed to create RAM MTD instance\n");
      fflush(stdout);
      exit(1);
    }

  /* Initialize to provide NXFFS on an MTD interface */

  ret = nxffs_initialize(mtd);
  if (ret < 0)
    {
      printf("ERROR: NXFFS initialization failed: %d\n", -ret);
      fflush(stdout);
      exit(2);
    }

  /* Mount the file system */

  ret = mount(NULL, CONFIG_TESTING_NXFFS_MOUNTPT, "nxffs", 0, NULL);
  if (ret < 0)
    {
      printf("ERROR: Failed to mount the NXFFS volume: %d\n", errno);
      fflush(stdout);
      exit(3);
    }

  /* Set up memory monitoring */

  g_mmbefore   = mallinfo();
  g_mmprevious = g_mmbefore;

  /* Loop a few times ... file the file system with some random, files,
   * delete some files randomly, fill the file system with more random file,
   * delete, etc.  This beats the FLASH very hard!
   */

#if CONFIG_TESTING_NXFFS_NLOOPS == 0
  for (i = 0; ; i++)
#else
  for (i = 1; i <= CONFIG_TESTING_NXFFS_NLOOPS; i++)
#endif
    {
      /* Write a files to the NXFFS file system until either (1) all of the
       * open file structures are utilized or until (2) NXFFS reports an
       * error (hopefully that the file system is full)
       */

      printf("\n=== FILLING %u =============================\n", i);
      nxffs_fillfs();
      printf("Filled file system\n");
      printf("  Number of files: %d\n", g_nfiles);
      printf("  Number deleted:  %d\n", g_ndeleted);
      nxffs_dump(mtd, CONFIG_TESTING_NXFFS_VERBOSE);

      /* Directory listing */

      nxffs_directory();

      /* Verify all files written to FLASH */

      ret = nxffs_verifyfs();
      if (ret < 0)
        {
          printf("ERROR: Failed to verify files\n");
          printf("  Number of files: %d\n", g_nfiles);
          printf("  Number deleted:  %d\n", g_ndeleted);
          exit(ret);
        }
      else
        {
#if CONFIG_TESTING_NXFFS_VERBOSE != 0
          printf("Verified!\n");
          printf("  Number of files: %d\n", g_nfiles);
          printf("  Number deleted:  %d\n", g_ndeleted);
#endif
        }

      /* Delete some files */

      printf("\n=== DELETING %u ============================\n", i);
      ret = nxffs_delfiles();
      if (ret < 0)
        {
          printf("ERROR: Failed to delete files\n");
          printf("  Number of files: %d\n", g_nfiles);
          printf("  Number deleted:  %d\n", g_ndeleted);
          exit(ret);
        }
      else
        {
          printf("Deleted some files\n");
          printf("  Number of files: %d\n", g_nfiles);
          printf("  Number deleted:  %d\n", g_ndeleted);
        }

      nxffs_dump(mtd, CONFIG_TESTING_NXFFS_VERBOSE);

      /* Directory listing */

      nxffs_directory();

      /* Verify all files written to FLASH */

      ret = nxffs_verifyfs();
      if (ret < 0)
        {
          printf("ERROR: Failed to verify files\n");
          printf("  Number of files: %d\n", g_nfiles);
          printf("  Number deleted:  %d\n", g_ndeleted);
          exit(ret);
        }
      else
        {
#if CONFIG_TESTING_NXFFS_VERBOSE != 0
          printf("Verified!\n");
          printf("  Number of files: %d\n", g_nfiles);
          printf("  Number deleted:  %d\n", g_ndeleted);
#endif
        }

      /* Show memory usage */

      nxffs_loopmemusage();
      fflush(stdout);
    }

  /* Delete all files then show memory usage again */

  nxffs_delallfiles();
  nxffs_endmemusage();
  fflush(stdout);
  return 0;
}
