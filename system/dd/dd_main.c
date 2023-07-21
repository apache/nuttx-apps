/****************************************************************************
 * apps/system/dd/dd_main.c
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

#include <nuttx/clock.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <debug.h>
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
  bool         eof;        /* true: The end of the input or output file has been hit */
  uint16_t     sectsize;   /* Size of one sector */
  uint16_t     nbytes;     /* Number of valid bytes in the buffer */
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
  uint16_t written;
  ssize_t nbytes;

  /* Is the out buffer full (or is this the last one)? */

  written = 0;
  do
    {
      nbytes = write(dd->outfd, buffer, dd->nbytes - written);
      if (nbytes < 0)
        {
          fprintf(stderr, "%s: failed to write: %s\n",
            g_dd, strerror(errno));
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
          fprintf(stderr, "%s: failed to read: %s\n", g_dd, strerror(errno));
          return ERROR;
        }

      dd->nbytes += nbytes;
      buffer     += nbytes;
    }
  while (dd->nbytes < dd->sectsize && nbytes > 0);

  dd->eof |= (dd->nbytes == 0);
  return OK;
}

/****************************************************************************
 * Name: dd_infopen
 ****************************************************************************/

static inline int dd_infopen(FAR const char *name, FAR struct dd_s *dd)
{
  dd->infd = open(name, O_RDONLY);
  if (dd->infd < 0)
    {
      fprintf(stderr, "%s: failed to open '%s': %s\n",
        g_dd, name, strerror(errno));
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: dd_outfopen
 ****************************************************************************/

static inline int dd_outfopen(FAR const char *name, FAR struct dd_s *dd)
{
  dd->outfd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (dd->outfd < 0)
    {
      fprintf(stderr, "%s: failed to open '%s': %s\n",
        g_dd, name, strerror(errno));
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: print_usage
 ****************************************************************************/

static void print_usage(void)
{
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "  %s if=<infile> of=<outfile> [bs=<sectsize>] "
    "[count=<sectors>] [skip=<sectors>]\n", g_dd);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  struct dd_s dd;
  FAR char *infile = NULL;
  FAR char *outfile = NULL;
  struct timespec ts0;
  struct timespec ts1;
  uint64_t elapsed;
  uint64_t total;
  uint32_t sector = 0;
  int ret = ERROR;
  int i;

  /* Initialize the dd structure */

  memset(&dd, 0, sizeof(struct dd_s));
  dd.sectsize  = DEFAULT_SECTSIZE;  /* Sector size if 'bs=' not provided */
  dd.nsectors  = 0xffffffff;        /* MAX_UINT32 */

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
    }

  if (infile == NULL || outfile == NULL)
    {
      print_usage();
      goto errout_with_paths;
    }

  /* Allocate the I/O buffer */

  dd.buffer = malloc(dd.sectsize);
  if (!dd.buffer)
    {
      fprintf(stderr, "%s: failed to malloc: %s\n", g_dd, strerror(errno));
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
      if (ret < -1)
        {
          fprintf(stderr, "%s: failed to lseek: %s\n",
            g_dd, strerror(errno));
          ret = ERROR;
          goto errout_with_outf;
        }
    }

  /* Then perform the data transfer */

  clock_gettime(CLOCK_MONOTONIC, &ts0);

  while (!dd.eof && sector < dd.nsectors)
    {
      /* Read one sector from from the input */

      ret = dd_read(&dd);
      if (ret < 0)
        {
          goto errout_with_outf;
        }

      /* Has the incoming data stream ended? */

      if (!dd.eof)
        {
          /* Write one sector to the output file */

          ret = dd_write(&dd);
          if (ret < 0)
            {
              goto errout_with_outf;
            }

          /* Increment the sector number */

          sector++;
        }
    }

  ret = OK;

  clock_gettime(CLOCK_MONOTONIC, &ts1);

  elapsed  = (((uint64_t)ts1.tv_sec * NSEC_PER_SEC) + ts1.tv_nsec);
  elapsed -= (((uint64_t)ts0.tv_sec * NSEC_PER_SEC) + ts0.tv_nsec);
  elapsed /= NSEC_PER_USEC; /* usec */

  total = ((uint64_t)sector * (uint64_t)dd.sectsize);

  fprintf(stderr, "%llu bytes copied, %u usec, ",
             total, (unsigned int)elapsed);
  fprintf(stderr, "%u KB/s\n" ,
             (unsigned int)(((double)total / 1024)
             / ((double)elapsed / USEC_PER_SEC)));

errout_with_outf:
  close(dd.outfd);

errout_with_inf:
  close(dd.infd);

errout_with_alloc:
  free(dd.buffer);

errout_with_paths:
  return ret;
}
