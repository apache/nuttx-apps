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

#ifndef CONFIG_EXAMPLES_NXFFS_MAXFILE
#  define CONFIG_EXAMPLES_NXFFS_MAXFILE 8192
#endif

#ifndef CONFIG_EXAMPLES_NXFFS_GULP
#  define CONFIG_EXAMPLES_NXFFS_GULP 347
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
      exit(3);
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

static inline int nxffs_wrfile(void)
{
  struct nxffs_filedesc_s *file = NULL;
  size_t offset;
  int fd;
  int i;

  for (i = 0; i < CONFIG_EXAMPLES_NXFFS_MAXOPEN; i++)
    {
      if (g_files[i].name == NULL)
        {
          file = &g_files[i];
          break;
        }
    }

  if (!file)
    {
      fprintf(stderr, "No available files\n");
      return ERROR;
    }

  nxffs_randname(file);
  nxffs_randfile(file);
  fd = open(file->name, O_WRONLY, 0666);
  if (fd < 0)
    {
      fprintf(stderr, "Failed to open file: %d\n", errno);
      fprintf(stderr, "  File name: %s\n", file->name);
      fprintf(stderr, "  File size: %d\n", file->len);
      nxffs_freefile(file);
      return ERROR;
    }

  for (offset = 0; offset < file->len; )
    {
      size_t nbytestowrite = file->len - offset;
      ssize_t nbyteswritten;

      if (nbytestowrite > CONFIG_EXAMPLES_NXFFS_GULP)
        {
          nbytestowrite = CONFIG_EXAMPLES_NXFFS_GULP;
        }

      nbyteswritten = write(fd, &g_fileimage[offset], nbytestowrite);
      if (nbyteswritten < 0)
        {
          fprintf(stderr, "Failed to write file: %d\n", errno);
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
          fprintf(stderr, "Partial write: %d\n");
          fprintf(stderr, "  File name:    %s\n", file->name);
          fprintf(stderr, "  File size:    %d\n", file->len);
          fprintf(stderr, "  Write offset: %d\n", offset);
          fprintf(stderr, "  Write size:   %d\n", nbytestowrite);
          fprintf(stderr, "  Written:      %d\n", nbyteswritten);
        }
      offset += nbyteswritten;
    }

  close(fd);
  g_nfiles++;
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

  /* Then write a file to the NXFFS file system */

  ret = nxffs_wrfile();
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to write a file\n");
      exit(3);
    }

  return 0;
}

