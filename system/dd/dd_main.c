/****************************************************************************
 * apps/system/dd/dd_main.c
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

#if defined(__NuttX__)
#include <nuttx/config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <debug.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ERROR (-1)
#define OK    (0)

/* If no sector size is specified with BS=, then the following default value
 * is used.
 */

#define DEFAULT_SECTSIZE 512

#if !defined(CONFIG_SYSTEM_DD_PROGNAME)
#define CONFIG_SYSTEM_DD_PROGNAME "dd"
#endif
#if !defined(__NuttX__)
#define FAR
#define NSEC_PER_USEC 1000
#define USEC_PER_SEC 1000000
#define NSEC_PER_SEC 1000000000
#endif

#define g_dd CONFIG_SYSTEM_DD_PROGNAME

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct dd_s
{
  int          infd;       /* File descriptor of the input device */
  int          outfd;      /* File descriptor of the output device */
  uint32_t     nsectors;   /* Number of sectors to transfer */
  uint32_t     skip;       /* The number of sectors skipped on input */
  uint32_t     seek;       /* The number of sectors seeked on output */
  int          oflags;     /* The open flags on output deivce */
  bool         eof;        /* true: The end of the input or output file has been hit */
  size_t       sectsize;   /* Size of one sector */
  size_t       nbytes;     /* Number of valid bytes in the buffer */
  FAR uint8_t *buffer;     /* Buffer of data to write to the output file */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dd_write
 ****************************************************************************/

static int dd_write(FAR struct dd_s *dd)
{
  FAR uint8_t *buffer = dd->buffer;
  size_t written;
  ssize_t nbytes;

  /* Is the out buffer full (or is this the last one)? */

  written = 0;
  do
    {
      nbytes = write(dd->outfd, buffer, dd->nbytes - written);
      if (nbytes < 0)
        {
          printf("%s: failed to write: %s\n", g_dd, strerror(errno));
          return ERROR;
        }

      written += nbytes;
      buffer  += nbytes;
    }
  while (written < dd->nbytes);

  return OK;
}

/****************************************************************************
 * Name: dd_read
 ****************************************************************************/

static int dd_read(FAR struct dd_s *dd)
{
  FAR uint8_t *buffer = dd->buffer;
  ssize_t nbytes;

  dd->nbytes = 0;
  do
    {
      nbytes = read(dd->infd, buffer, dd->sectsize - dd->nbytes);
      if (nbytes < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          printf("%s: failed to read: %s\n", g_dd, strerror(errno));
          return ERROR;
        }

      dd->nbytes += nbytes;
      buffer     += nbytes;
      if (nbytes == 0)
        {
          dd->eof = true;
          break;
        }
    }
  while (dd->nbytes < dd->sectsize && nbytes != 0);

  return OK;
}

/****************************************************************************
 * Name: dd_infopen
 ****************************************************************************/

static inline int dd_infopen(FAR const char *name, FAR struct dd_s *dd)
{
  if (name == NULL)
    {
      dd->infd = STDIN_FILENO;
      return OK;
    }

  dd->infd = open(name, O_RDONLY);
  if (dd->infd < 0)
    {
      printf("%s: failed to open '%s': %s\n", g_dd, name, strerror(errno));
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: dd_outfopen
 ****************************************************************************/

static inline int dd_outfopen(FAR const char *name, FAR struct dd_s *dd)
{
  if (name == NULL)
    {
      dd->outfd = STDOUT_FILENO;
      return OK;
    }

  dd->outfd = open(name, dd->oflags, 0644);
  if (dd->outfd < 0)
    {
      printf("%s: failed to open '%s': %s\n", g_dd, name, strerror(errno));
      return ERROR;
    }

  return OK;
}

static int dd_verify(FAR struct dd_s *dd)
{
  FAR uint8_t *buffer;
  unsigned sector = 0;
  int ret = OK;

  ret = lseek(dd->infd, dd->skip ? dd->skip * dd->sectsize : 0, SEEK_SET);
  if (ret < 0)
    {
      printf("%s: failed to infd lseek: %s\n", g_dd, strerror(errno));
      return ret;
    }

  dd->eof = 0;
  ret = lseek(dd->outfd, 0, SEEK_SET);
  if (ret < 0)
    {
      printf("%s: failed to outfd lseek: %s\n", g_dd, strerror(errno));
      return ret;
    }

  buffer = malloc(dd->sectsize);
  if (buffer == NULL)
    {
      return ERROR;
    }

  while (!dd->eof && sector < dd->nsectors)
    {
      ret = dd_read(dd);
      if (ret < 0)
        {
          break;
        }

      ret = read(dd->outfd, buffer, dd->nbytes);
      if (ret != dd->nbytes)
        {
          printf("%s: failed to outfd read: %d\n",
                 g_dd, ret < 0 ? errno : ret);
          break;
        }

      if (memcmp(dd->buffer, buffer, dd->nbytes) != 0)
        {
          char msg[32];
          snprintf(msg, sizeof(msg), "infile sector %d", sector);
          lib_dumpbuffer(msg, dd->buffer, dd->nbytes);
          snprintf(msg, sizeof(msg), "\noutfile sector %d", sector);
          lib_dumpbuffer(msg, buffer, dd->nbytes);
          ret = ERROR;
          break;
        }

      sector++;
    }

  if (ret < 0)
    {
      printf("%s: failed to dd verify: %d\n", g_dd, ret);
    }

  free(buffer);
  return ret;
}

/****************************************************************************
 * Name: print_usage
 ****************************************************************************/

static void print_usage(void)
{
  printf("usage:\n");
  printf("  %s [if=<infile>] [of=<outfile>] [bs=<sectsize>] "
         "[count=<sectors>] [skip=<sectors>] [seek=<sectors>] [verify] "
         "[conv=<nocreat,notrunc>]\n", g_dd);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  struct dd_s dd;
  FAR char *infile = NULL;
  FAR char *outfile = NULL;
#ifdef CONFIG_SYSTEM_DD_STATS
  struct timespec ts0;
  struct timespec ts1;
  uint64_t elapsed;
  uint64_t total = 0;
#endif
  uint32_t sector = 0;
  int ret = ERROR;
  int i;

  /* Initialize the dd structure */

  memset(&dd, 0, sizeof(struct dd_s));
  dd.sectsize  = DEFAULT_SECTSIZE;  /* Sector size if 'bs=' not provided */
  dd.nsectors  = 0xffffffff;        /* MAX_UINT32 */
  dd.oflags    = O_WRONLY | O_CREAT | O_TRUNC;

  /* Parse command line parameters */

  for (i = 1; i < argc; i++)
    {
      if (strncmp(argv[i], "if=", 3) == 0)
        {
          infile = &argv[i][3];
        }
      else if (strncmp(argv[i], "of=", 3) == 0)
        {
          outfile = &argv[i][3];
        }
      else if (strncmp(argv[i], "bs=", 3) == 0)
        {
          dd.sectsize = atoi(&argv[i][3]);
        }
      else if (strncmp(argv[i], "count=", 6) == 0)
        {
          dd.nsectors = atoi(&argv[i][6]);
        }
      else if (strncmp(argv[i], "skip=", 5) == 0)
        {
          dd.skip = atoi(&argv[i][5]);
        }
      else if (strncmp(argv[i], "seek=", 5) == 0)
        {
          dd.seek = atoi(&argv[i][5]);
        }
      else if (strncmp(argv[i], "verify", 6) == 0)
        {
          dd.oflags |= O_RDONLY;
        }
      else if (strncmp(argv[i], "conv=", 5) == 0)
        {
          const char *cur = &argv[i][5];
          while (true)
            {
              const char *next = strchr(cur, ',');
              size_t len = next != NULL ? next - cur : strlen(cur);
              if (len == 7 && !memcmp(cur, "notrunc", 7))
                {
                  dd.oflags &= ~O_TRUNC;
                }
              else if (len == 7 && !memcmp(cur, "nocreat", 7))
                {
                  dd.oflags &= ~(O_CREAT | O_TRUNC);
                }
              else
                {
                  printf("%s: unknown conversion '%.*s'\n", g_dd,
                         (int)len, cur);
                  goto errout_with_paths;
                }

              if (next == NULL)
                {
                  break;
                }

              cur = next + 1;
            }
        }
      else
        {
          print_usage();
          goto errout_with_paths;
        }
    }

  /* If verify enabled, infile and outfile are mandatory */

  if ((dd.oflags & O_RDONLY) && (infile == NULL || outfile == NULL))
    {
      printf("%s: invalid parameters: %s\n", g_dd, strerror(EINVAL));
      print_usage();
      goto errout_with_paths;
    }

  /* Allocate the I/O buffer */

  dd.buffer = malloc(dd.sectsize);
  if (!dd.buffer)
    {
      printf("%s: failed to malloc: %s\n", g_dd, strerror(errno));
      goto errout_with_paths;
    }

  /* Open the input file */

  ret = dd_infopen(infile, &dd);
  if (ret < 0)
    {
      goto errout_with_alloc;
    }

  /* Open the output file */

  ret = dd_outfopen(outfile, &dd);
  if (ret < 0)
    {
      goto errout_with_inf;
    }

  if (dd.skip)
    {
      ret = lseek(dd.infd, dd.skip * dd.sectsize, SEEK_SET);
      if (ret < 0)
        {
          printf("%s: failed to lseek: %s\n", g_dd, strerror(errno));
          ret = ERROR;
          goto errout_with_outf;
        }
    }

  if (dd.seek)
    {
      ret = lseek(dd.outfd, dd.seek * dd.sectsize, SEEK_SET);
      if (ret < 0)
        {
          printf("%s: failed to lseek on output: %s\n",
                 g_dd, strerror(errno));
          ret = ERROR;
          goto errout_with_outf;
        }
    }

  /* Then perform the data transfer */

#ifdef CONFIG_SYSTEM_DD_STATS
  clock_gettime(CLOCK_MONOTONIC, &ts0);
#endif

  while (!dd.eof && sector < dd.nsectors)
    {
      /* Read one sector from from the input */

      ret = dd_read(&dd);
      if (ret < 0)
        {
          goto errout_with_outf;
        }

      /* Has the incoming data stream ended? */

      if (dd.nbytes > 0)
        {
          /* Write one sector to the output file */

          ret = dd_write(&dd);
          if (ret < 0)
            {
              goto errout_with_outf;
            }

          /* Increment the sector number */

          sector++;
#ifdef CONFIG_SYSTEM_DD_STATS
          total += dd.nbytes;
#endif
        }
    }

  ret = OK;

#ifdef CONFIG_SYSTEM_DD_STATS
  clock_gettime(CLOCK_MONOTONIC, &ts1);

  elapsed  = (((uint64_t)ts1.tv_sec * NSEC_PER_SEC) + ts1.tv_nsec);
  elapsed -= (((uint64_t)ts0.tv_sec * NSEC_PER_SEC) + ts0.tv_nsec);
  elapsed /= NSEC_PER_USEC; /* usec */

  printf("%" PRIu64 " bytes (%" PRIu32 " blocks) copied, %u usec, ",
         total, sector, (unsigned int)elapsed);
  printf("%u KB/s\n" ,
         (unsigned int)(((double)total / 1024)
         / ((double)elapsed / USEC_PER_SEC)));
#endif

  if (ret == 0 && (dd.oflags & O_RDONLY) != 0)
    {
      ret = dd_verify(&dd);
    }

errout_with_outf:
  if (outfile)
    {
      dd.outfd = close(dd.outfd);
      if (dd.outfd < 0)
        {
          printf("%s failed to close outfd:%s\n", g_dd, strerror(errno));
        }
    }

errout_with_inf:
  if (infile)
    {
      dd.infd = close(dd.infd);
      if (dd.infd < 0)
        {
          printf("%s failed to close infd:%s\n", g_dd, strerror(errno));
        }
    }

errout_with_alloc:
  free(dd.buffer);

errout_with_paths:
  return ret < 0 ? ret : (dd.outfd < 0 ? dd.outfd : dd.infd);
}
