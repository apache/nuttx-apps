/****************************************************************************
 * examples/nxffs/nxffs_main.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
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

#include <nuttx/config.h>

#include <sys/mount.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <crc32.h>

#include <nuttx/mtd.h>
#include <nuttx/nxffs.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/
/* Configuration ************************************************************/
/* This must exactly match the default configuration in drivers/mtd/rammtd.c */

#ifndef CONFIG_RAMMTD_BLOCKSIZE
#  define CONFIG_RAMMTD_BLOCKSIZE 512
#endif

#ifndef CONFIG_RAMMTD_ERASESIZE
#  define CONFIG_RAMMTD_ERASESIZE 4096
#endif

#ifndef CONFIG_EXAMPLES_NXFFS_NEBLOCKS
#  define CONFIG_EXAMPLES_NXFFS_NEBLOCKS (32)
#endif

#undef CONFIG_EXAMPLES_NXFFS_BUFSIZE
#define CONFIG_EXAMPLES_NXFFS_BUFSIZE \
  (CONFIG_RAMMTD_ERASESIZE * CONFIG_EXAMPLES_NXFFS_NEBLOCKS)

#ifndef CONFIG_EXAMPLES_NXFFS_MAXNAME
#  define CONFIG_EXAMPLES_NXFFS_MAXNAME 128
#endif

#if CONFIG_EXAMPLES_NXFFS_MAXNAME > 255
#  undef CONFIG_EXAMPLES_NXFFS_MAXNAME
#  define CONFIG_EXAMPLES_NXFFS_MAXNAME 255
#endif

#ifndef CONFIG_EXAMPLES_NXFFS_MAXFILE
#  define CONFIG_EXAMPLES_NXFFS_MAXFILE 8192
#endif

#ifndef CONFIG_EXAMPLES_NXFFS_MAXIO
#  define CONFIG_EXAMPLES_NXFFS_MAXIO 347
#endif

#ifndef CONFIG_EXAMPLES_NXFFS_MAXOPEN
#  define CONFIG_EXAMPLES_NXFFS_MAXOPEN 512
#endif

#ifndef CONFIG_EXAMPLES_NXFFS_MOUNTPT
#  define CONFIG_EXAMPLES_NXFFS_MOUNTPT "/mnt/nxffs"
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

static uint8_t g_simflash[CONFIG_EXAMPLES_NXFFS_BUFSIZE];
static uint8_t g_fileimage[CONFIG_EXAMPLES_NXFFS_MAXFILE];
static struct nxffs_filedesc_s g_files[CONFIG_EXAMPLES_NXFFS_MAXOPEN];
static const char g_mountdir[] = CONFIG_EXAMPLES_NXFFS_MOUNTPT "/";
static int g_nfiles;
static int g_ndeleted;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxffs_randchar
 ****************************************************************************/

static inline char nxffs_randchar(void)
{
  int value = rand() % 63;
  if (value == 0)
    {
      return '/';
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
  maxname  = CONFIG_EXAMPLES_NXFFS_MAXNAME - dirlen;
  namelen  = (rand() % maxname) + 1;
  alloclen = namelen + dirlen;

  file->name = (FAR char*)malloc(alloclen + 1);
  if (!file->name)
    {
      fprintf(stderr, "ERROR: Failed to allocate name, length=%d\n", namelen);
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

  file->len = (rand() % CONFIG_EXAMPLES_NXFFS_MAXFILE) + 1;
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

  /* Create a random file */

  nxffs_randname(file);
  nxffs_randfile(file);
  fd = open(file->name, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0)
    {
      fprintf(stderr, "Failed to open file for writing: %d\n", errno);
      fprintf(stderr, "  File name: %s\n", file->name);
      fprintf(stderr, "  File size: %d\n", file->len);
      nxffs_freefile(file);
      return ERROR;
    }

  /* Write a random amount of data dat the file */

  for (offset = 0; offset < file->len; )
    {
      size_t maxio = (rand() % CONFIG_EXAMPLES_NXFFS_MAXIO) + 1;
      size_t nbytestowrite = file->len - offset;
      ssize_t nbyteswritten;

      if (nbytestowrite > maxio)
        {
          nbytestowrite = maxio;
        }

      nbyteswritten = write(fd, &g_fileimage[offset], nbytestowrite);
      if (nbyteswritten < 0)
        {
          fprintf(stderr, "ERROR: Failed to write file: %d\n", errno);
          fprintf(stderr, "  File name:    %s\n", file->name);
          fprintf(stderr, "  File size:    %d\n", file->len);
          fprintf(stderr, "  Write offset: %d\n", offset);
          fprintf(stderr, "  Write size:   %d\n", nbytestowrite);
          nxffs_freefile(file);
          close(fd);
          return ERROR;
        }
      else if (nbyteswritten != nbytestowrite)
        {
          fprintf(stderr, "ERROR: Partial write:\n");
          fprintf(stderr, "  File name:    %s\n", file->name);
          fprintf(stderr, "  File size:    %d\n", file->len);
          fprintf(stderr, "  Write offset: %d\n", offset);
          fprintf(stderr, "  Write size:   %d\n", nbytestowrite);
          fprintf(stderr, "  Written:      %d\n", nbyteswritten);
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

  for (i = 0; i < CONFIG_EXAMPLES_NXFFS_MAXOPEN; i++)
    {
      file = &g_files[i];
      if (file->name == NULL)
        {
          ret = nxffs_wrfile(file);
          if (ret < 0)
            {
              fprintf(stderr, "ERROR: Failed to write a file. g_nfiles=%d\n", g_nfiles);
              return ERROR;
            }

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
  size_t maxio = (rand() % CONFIG_EXAMPLES_NXFFS_MAXIO) + 1;
  ssize_t nbytesread;

  if (len > maxio)
    {
      len = maxio;
    }

  nbytesread = read(fd, &g_fileimage[offset], len);
  if (nbytesread < 0)
    {
      fprintf(stderr, "ERROR: Failed to read file: %d\n", errno);
      fprintf(stderr, "  File name:    %s\n", file->name);
      fprintf(stderr, "  File size:    %d\n", file->len);
      fprintf(stderr, "  Read offset:  %d\n", offset);
      fprintf(stderr, "  Read size:    %d\n", len);
      return ERROR;
    }
  else if (nbytesread == 0)
    {
#if 0 /* No... we do this on purpose sometimes */
      fprintf(stderr, "ERROR: Unexpected end-of-file:\n");
      fprintf(stderr, "  File name:    %s\n", file->name);
      fprintf(stderr, "  File size:    %d\n", file->len);
      fprintf(stderr, "  Read offset:  %d\n", offset);
      fprintf(stderr, "  Read size:    %d\n", len);
#endif
      return ERROR;
    }
  else if (nbytesread != len)
    {
      fprintf(stderr, "ERROR: Partial read:\n");
      fprintf(stderr, "  File name:    %s\n", file->name);
      fprintf(stderr, "  File size:    %d\n", file->len);
      fprintf(stderr, "  Read offset:  %d\n", offset);
      fprintf(stderr, "  Read size:    %d\n", len);
      fprintf(stderr, "  Bytes read:   %d\n", nbytesread);
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
          fprintf(stderr, "ERROR: Failed to open file for reading: %d\n", errno);
          fprintf(stderr, "  File name: %s\n", file->name);
          fprintf(stderr, "  File size: %d\n", file->len);
        }
      return ERROR;
    }

  /* Read all of the data info the fileimage buffer using random read sizes */

  for (ntotalread = 0; ntotalread < file->len; )
    {
      nbytesread = nxffs_rdblock(fd, file, ntotalread, file->len - ntotalread);
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
      fprintf(stderr, "ERROR: Bad CRC: %d vs %d\n", crc, file->crc);
      fprintf(stderr, "  File name: %s\n", file->name);
      fprintf(stderr, "  File size: %d\n", file->len);
      close(fd);
      return ERROR;
    }

  /* Try reading past the end of the file */

  nbytesread = nxffs_rdblock(fd, file, ntotalread, 1024) ;
  if (nbytesread > 0)
    {
      fprintf(stderr, "ERROR: Read past the end of file\n");
      fprintf(stderr, "  File name:  %s\n", file->name);
      fprintf(stderr, "  File size:  %d\n", file->len);
      fprintf(stderr, "  Bytes read: %d\n", nbytesread);
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

  for (i = 0; i < CONFIG_EXAMPLES_NXFFS_MAXOPEN; i++)
    {
      file = &g_files[i];
      if (file->name != NULL)
        {
          ret = nxffs_rdfile(file);
          if (ret < 0)
            {
              if (file->deleted)
                {
                  fprintf(stderr, "Deleted file %d OK\n", i);
                  nxffs_freefile(file);
                  g_ndeleted--;
                  g_nfiles--;
                }
              else
                {
                  fprintf(stderr, "ERROR: Failed to read a file: %d\n", i);
                  fprintf(stderr, "  File name: %s\n", file->name);
                  fprintf(stderr, "  File size: %d\n", file->len);
                  return ERROR;
                }
            }
          else
            {
              if (file->deleted)
                {
                  fprintf(stderr, "Succesffully read a deleted file\n");
                  fprintf(stderr, "  File name: %s\n", file->name);
                  fprintf(stderr, "  File size: %d\n", file->len);
                  nxffs_freefile(file);
                  g_ndeleted--;
                  g_nfiles--;
                  return ERROR;
                }
              else
                {
                  fprintf(stderr, "File %d: OK\n", i);
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

  /* How many files should we delete? */

  ndel = (rand() % (g_nfiles - g_ndeleted)) + 1;

  /* Now pick which files to delete */

  for (i = 0; i < ndel; i++)
    {
      /* Guess a file index */

      int ndx = (rand() % (g_nfiles - g_ndeleted));

      /* And delete the next undeleted file after that random index */

      for (j = ndx + 1; j != ndx; j++)
        {
          if (j >= CONFIG_EXAMPLES_NXFFS_MAXOPEN)
            {
              j = 0;
            }

          file = &g_files[j];
          if (file->name && !file->deleted)
            {
              ret = unlink(file->name);
              if (ret < 0)
                {
                  fprintf(stderr, "ERROR: Unlink %d failed: %d\n", i+1, errno);
                  fprintf(stderr, "  File name:  %s\n", file->name);
                  fprintf(stderr, "  File size:  %d\n", file->len);
                  fprintf(stderr, "  File index: %d\n", j);
                }
              else
                {
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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: user_start
 ****************************************************************************/

int user_start(int argc, char *argv[])
{
  FAR struct mtd_dev_s *mtd;
  int ret;

  /* Seed the random number generated */

  srand(0x93846);

  /* Create and initialize a RAM MTD device instance */

  mtd = rammtd_initialize(g_simflash, CONFIG_EXAMPLES_NXFFS_BUFSIZE);
  if (!mtd)
    {
      fprintf(stderr, "ERROR: Failed to create RAM MTD instance\n");
      exit(1);
    }

  /* Initialize to provide NXFFS on an MTD interface */

  ret = nxffs_initialize(mtd);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: NXFFS initialization failed: %d\n", -ret);
      exit(2);
    }

  /* Mount the file system */

  ret = mount(NULL, CONFIG_EXAMPLES_NXFFS_MOUNTPT, "nxffs", 0, NULL);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to mount the NXFFS volume: %d\n", errno);
      exit(3);
    }

  /* Then write a files to the NXFFS file system until either (1) all of the
   * open file structures are utilized or until (2) NXFFS reports an error
   * (hopefully that the file system is full)
   */

  ret = nxffs_fillfs();
  fprintf(stderr, "Filled file system\n");
  fprintf(stderr, "  Number of files: %d\n", g_nfiles);
  fprintf(stderr, "  Number deleted:  %d\n", g_ndeleted);
  nxffs_dump(mtd);

  /* Verify all files written to FLASH */

  ret = nxffs_verifyfs();
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to verify files\n");
      fprintf(stderr, "  Number of files: %d\n", g_nfiles);
      fprintf(stderr, "  Number deleted:  %d\n", g_ndeleted);
    }

  /* Delete some files */

  ret = nxffs_delfiles();
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to delete files\n");
    }
  else
    {
      fprintf(stderr, "Deleted some files\n");
    }
  fprintf(stderr, "  Number of files: %d\n", g_nfiles);
  fprintf(stderr, "  Number deleted:  %d\n", g_ndeleted);
  nxffs_dump(mtd);

  /* Verify all files written to FLASH */

  ret = nxffs_verifyfs();
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to verify files\n");
      fprintf(stderr, "  Number of files: %d\n", g_nfiles);
      fprintf(stderr, "  Number deleted:  %d\n", g_ndeleted);
    }

  return 0;
}

