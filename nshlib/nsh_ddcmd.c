/****************************************************************************
 * apps/nshlib/nsh_ddcmd.c
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

#include <sys/types.h>
#include <sys/stat.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <debug.h>
#include <errno.h>
#include <time.h>

#include "nsh.h"
#include "nsh_console.h"

#ifndef CONFIG_NSH_DISABLE_DD

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* If no sector size is specified with BS=, then the following default value
 * is used.
 */

#define DEFAULT_SECTSIZE 512

/* At present, piping of input and output are not support, i.e., both of=
 * and if= arguments are required.
 */

#undef CAN_PIPE_FROM_STD

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct dd_s
{
  FAR struct nsh_vtbl_s *vtbl;

  int      infd;       /* File descriptor of the input device */
  int      outfd;      /* File descriptor of the output device */
  uint32_t nsectors;   /* Number of sectors to transfer */
  uint32_t sector;     /* The current sector number */
  uint32_t skip;       /* The number of sectors skipped on input */
  bool     eof;        /* true: The end of the input or output file has been hit */
  uint16_t sectsize;   /* Size of one sector */
  uint16_t nbytes;     /* Number of valid bytes in the buffer */
  uint8_t *buffer;     /* Buffer of data to write to the output file */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_dd[] = "dd";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dd_write
 ****************************************************************************/

static int dd_write(FAR struct dd_s *dd)
{
  FAR uint8_t *buffer = dd->buffer;
  uint16_t written ;
  ssize_t nbytes;

  /* Is the out buffer full (or is this the last one)? */

  written = 0;
  do
    {
      nbytes = write(dd->outfd, buffer, dd->sectsize - written);
      if (nbytes < 0)
        {
          FAR struct nsh_vtbl_s *vtbl = dd->vtbl;
          nsh_error(vtbl, g_fmtcmdfailed, g_dd, "write",
                    NSH_ERRNO_OF(-nbytes));
          return ERROR;
        }

      written += nbytes;
      buffer  += nbytes;
    }
  while (written < dd->sectsize);

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
          FAR struct nsh_vtbl_s *vtbl = dd->vtbl;
          nsh_error(vtbl, g_fmtcmdfailed, g_dd, "read",
                    NSH_ERRNO_OF(-nbytes));
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
      FAR struct nsh_vtbl_s *vtbl = dd->vtbl;
      nsh_error(vtbl, g_fmtcmdfailed, g_dd, "open", NSH_ERRNO);
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
      FAR struct nsh_vtbl_s *vtbl = dd->vtbl;
      nsh_error(vtbl, g_fmtcmdfailed, g_dd, "open", NSH_ERRNO);
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_dd
 ****************************************************************************/

int cmd_dd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  struct dd_s dd;
  FAR char *infile = NULL;
  FAR char *outfile = NULL;
#ifdef CONFIG_NSH_CMDOPT_DD_STATS
  struct timespec ts0;
  struct timespec ts1;
  uint64_t elapsed;
  uint64_t total;
#endif
  int ret = ERROR;
  int i;

  /* Initialize the dd structure */

  memset(&dd, 0, sizeof(struct dd_s));
  dd.vtbl      = vtbl;              /* For nsh_output */
  dd.sectsize  = DEFAULT_SECTSIZE;  /* Sector size if 'bs=' not provided */
  dd.nsectors  = 0xffffffff;        /* MAX_UINT32 */

  /* If no IF= option is provided on the command line, then read
   * from stdin.
   */

#ifdef CAN_PIPE_FROM_STD
  dd->infd     = 0;       /* stdin */
#endif

  /* If no OF= option is provided on the command line, then write
   * to stdout.
   */

#ifdef CAN_PIPE_FROM_STD
  dd->outfd    = 1;       /* stdout */
#endif

  /* Parse command line parameters */

  for (i = 1; i < argc; i++)
    {
      if (strncmp(argv[i], "if=", 3) == 0)
        {
          if (infile != NULL)
            {
              free(infile);
            }

          infile = nsh_getfullpath(vtbl, &argv[i][3]);
        }
      else if (strncmp(argv[i], "of=", 3) == 0)
        {
          if (outfile != NULL)
            {
              free(outfile);
            }

          outfile = nsh_getfullpath(vtbl, &argv[i][3]);
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

#ifndef CAN_PIPE_FROM_STD
  if (infile == NULL || outfile == NULL)
    {
      nsh_error(vtbl, g_fmtargrequired, g_dd);
      goto errout_with_paths;
    }
#endif

  /* Allocate the I/O buffer */

  dd.buffer = malloc(dd.sectsize);
  if (!dd.buffer)
    {
      nsh_error(vtbl, g_fmtcmdoutofmemory, g_dd);
      goto errout_with_paths;
    }

  /* Open the input file */

  ret = dd_infopen(infile, &dd);
  if (ret < 0)
    {
      goto errout_with_paths;
    }

  /* Open the output file */

  ret = dd_outfopen(outfile, &dd);
  if (ret < 0)
    {
      goto errout_with_inf;
    }

  /* Then perform the data transfer */

#ifdef CONFIG_NSH_CMDOPT_DD_STATS
#ifdef CONFIG_CLOCK_MONOTONIC
  clock_gettime(CLOCK_MONOTONIC, &ts0);
#else
  clock_gettime(CLOCK_REALTIME, &ts0);
#endif
#endif

  dd.sector = 0;
  while (!dd.eof && dd.nsectors > 0)
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
          /* Pad with zero if necessary (at the end of file only) */

          for (i = dd.nbytes; i < dd.sectsize; i++)
            {
              dd.buffer[i] = 0;
            }

          /* Write one sector to the output file */

          if (dd.sector >= dd.skip)
            {
              ret = dd_write(&dd);
              if (ret < 0)
                {
                  goto errout_with_outf;
                }

              /* Decrement to show that a sector was written */

              dd.nsectors--;
            }

          /* Increment the sector number */

          dd.sector++;
        }
    }

  ret = OK;

#ifdef CONFIG_NSH_CMDOPT_DD_STATS
#ifdef CONFIG_CLOCK_MONOTONIC
  clock_gettime(CLOCK_MONOTONIC, &ts1);
#else
  clock_gettime(CLOCK_REALTIME, &ts1);
#endif

  elapsed  = (((uint64_t)ts1.tv_sec * NSEC_PER_SEC) + ts1.tv_nsec);
  elapsed -= (((uint64_t)ts0.tv_sec * NSEC_PER_SEC) + ts0.tv_nsec);
  elapsed /= NSEC_PER_MSEC; /* msec */

  total = ((uint64_t)dd.sector * (uint64_t)dd.sectsize);

  nsh_output(vtbl, "%llu bytes copied, %u msec, ",
             total, (unsigned int)elapsed);
  nsh_output(vtbl, "%u KB/s\n" ,
             (unsigned int)((double)total / (double)elapsed));
#endif

errout_with_outf:
  close(dd.outfd);

errout_with_inf:
  close(dd.infd);
  free(dd.buffer);

errout_with_paths:
  if (infile)
    {
      nsh_freefullpath(infile);
    }

  if (outfile)
    {
      nsh_freefullpath(outfile);
    }

  return ret;
}

#endif /* !CONFIG_NSH_DISABLE_DD */
