/****************************************************************************
 * apps/system/tee/tee.c
 * $NetBSD: tee.c,v 1.6 1997/10/20 00:37:11 lukem Exp $
 *
 *   Copyright (c) 2016.  Gregory Nutt.  All rights reserved.
 *   Author:  Gregory Nutt <gnutt@nuttx.org>
 *
 * Leveraged from NetBSD:
 *
 *   Copyright (c) 1988, 1993
 *     The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BSIZE         1024
#define STDIN_FILENO  0
#define STDOUT_FILENO 1

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tee_list_s
{
  FAR struct tee_list_s *next;
  FAR char *name;
  int fd;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int tee_add(int fd, FAR char *name,
                   FAR struct tee_list_s **head)
{
  FAR struct tee_list_s *curr;

  curr = (FAR struct tee_list_s *)malloc((sizeof(struct tee_list_s)));
  if (curr == NULL)
    {
      _err("ERROR: alloc failed\n");
      return -ENOMEM;
    }

  curr->fd   = fd;
  curr->name = name;
  curr->next = *head;
  *head      = curr;

  return OK;
}

static void show_usage(FAR const char *progrname, int exitcode)
{
  fprintf(stderr, "USAGE: tee [-a] [file ...]\n");
  fprintf(stderr, "       tee -h\n");
  fprintf(stderr, "Where:\n");
  fprintf(stderr, "-a:\n");
  fprintf(stderr, "\tAppend to the file (vs. truncating)\n");
  fprintf(stderr, "file:\n");
  fprintf(stderr, "\tArbitrary number of options output files.  Output\n");
  fprintf(stderr, "\twill go to stdout in addition to each of these files.\n");
  fprintf(stderr, "-h:\n");
  fprintf(stderr, "\tShows this message and exits\n");
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int tee_main(int argc, char *argv[])
#endif
{
  FAR struct tee_list_s *head = NULL;
  FAR struct tee_list_s *curr;
  FAR struct tee_list_s *next;
  FAR char *buf;
  FAR char *bp;
  bool append = false;
  int exitcode = EXIT_FAILURE;
  int rval;
  int wval;
  int ret;
  int fd;
  int ch;
  int n;

  while ((ch = getopt(argc, argv, "ah")) != -1)
    {
      switch (ch)
        {
        case 'a':
          append = true;
          break;

        case 'h':
          show_usage(argv[0], EXIT_SUCCESS);

        case '?':
        default:
          show_usage(argv[0], EXIT_FAILURE);
        }
    }

  argv += optind;
  argc -= optind;

  buf = (FAR char *)malloc((size_t)BSIZE);
  if (buf == NULL)
    {
      _err("ERROR: malloc failed\n");
      exit(EXIT_FAILURE);
    }

  ret = tee_add(STDOUT_FILENO, "stdout", &head);
  if (ret < 0)
    {
      _err("ERROR: tee_add failed: %d\n", ret);
      goto errout_with_buf;
    }

  for (; *argv; ++argv)
    {
      fd = open(*argv, append ? O_WRONLY | O_CREAT | O_APPEND :
                                O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd < 0)
        {
          _err("ERROR: tee_add failed: %d\n", errno);
          goto errout_with_list;
        }
      else
        {
          ret = tee_add(fd, *argv, &head);
          if (ret < 0)
            {
              _err("ERROR: tee_add failed: %d\n", ret);
              goto errout_with_list;
            }
        }
    }

  while ((rval = read(STDIN_FILENO, buf, BSIZE)) > 0)
    {
      for (curr = head; curr; curr = curr->next)
        {
          n  = rval;
          bp = buf;

          do
            {
               wval = write(curr->fd, bp, n);
               if (wval < 0)
                {
                  _err("ERROR: write to %s failed: %d\n",
                       curr->name, errno);
                  break;
                }

              bp += wval;
              n  -= wval;
            }
          while (n > 0);
        }
    }

  if (rval >= 0)
    {
      exitcode = EXIT_SUCCESS;
    }
  else
    {
      _err("ERROR: read failed: %d\n", errno);
    }

errout_with_list:
  for (curr = head; curr; curr = next)
    {
      next = curr->next;
      (void)close(curr->fd);
      free(curr);
    }

errout_with_buf:
  free(buf);
  exit(exitcode);
}
