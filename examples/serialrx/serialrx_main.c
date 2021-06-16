/****************************************************************************
 * apps/examples/serialrx/serialrx_main.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Copyright (C) 2015 Omni Hoverboards Inc. All rights reserved.
 *   Authors: Gregory Nutt <gnutt@nuttx.org>
 *            Bob Doiron
 *            Paul Alexander Patience <paul-a.patience@polymtl.ca>
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

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <nuttx/arch.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * serialrx_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_EXAMPLES_SERIALRX_BUFFERED
  FAR FILE *f;
  int fd;
  int nbytes;
  int cnt;
#else
  int fd;
  int cnt;
  int bytecount = 0;
#endif
#ifdef CONFIG_EXAMPLES_SERIALRX_PRINTHYPHEN
  int count = 0;
#endif
#ifdef CONFIG_EXAMPLES_SERIALRX_PRINTHEX
  int i;
#endif
  bool eof = false;
  FAR char *buf;
  FAR char *devpath;

  if (argc == 1)
    {
      devpath = CONFIG_EXAMPLES_SERIALRX_DEVPATH;
    }
  else if (argc == 2)
    {
      devpath = argv[1];
    }
  else if (argc == 3)
    {
          devpath = argv[1];
          bytecount = strtol(argv[2], NULL, 10);
    }
  else
    {
      fprintf(stderr, "Usage: %s [devpath]\n", argv[0]);
      goto errout;
    }

#ifdef CONFIG_EXAMPLES_SERIALRX_PRINTSTR
  buf = (FAR char *)malloc(CONFIG_EXAMPLES_SERIALRX_BUFSIZE + 1);
#else
  buf = (FAR char *)malloc(CONFIG_EXAMPLES_SERIALRX_BUFSIZE);
#endif

  if (buf == NULL)
    {
      fprintf(stderr, "ERROR: malloc failed: %d\n", errno);
      goto errout;
    }

#ifdef CONFIG_EXAMPLES_SERIALRX_BUFFERED
  f = fopen(devpath, "r");
  if (f == NULL)
    {
      fprintf(stderr, "ERROR: fopen failed: %d\n", errno);
      goto errout_with_buf;
    }
#else
  fd = open(devpath, O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: open failed: %d\n", errno);
      goto errout_with_buf;
    }
#endif

  printf("Reading from %s\n", devpath);
  fflush(stdout);
  cnt = 0;
  while (cnt < bytecount)
    {
#ifdef CONFIG_EXAMPLES_SERIALRX_BUFFERED
      size_t n = fread(buf, 1, 26, f);
      cnt++;
      if (feof(f))
        {
          eof = true;
        }
      else if (ferror(f))
        {
          printf("fread failed: %d\n", errno);
          fflush(stdout);
          clearerr(f);
        }
#else
      ssize_t n = read(fd, buf, CONFIG_EXAMPLES_SERIALRX_BUFSIZE);
      up_udelay(1000);
      if (n == 0)
        {
          eof = true;
        }
      else if (n < 0)
        {
          printf("read failed: %d\n", errno);
          fflush(stdout);
        }
#endif
      else
        {
#if defined(CONFIG_EXAMPLES_SERIALRX_PRINTHYPHEN)
          count += (int)n;
          if (count >= CONFIG_EXAMPLES_SERIALRX_BUFSIZE)
            {
              printf("-");
              fflush(stdout);
              count -= CONFIG_EXAMPLES_SERIALRX_BUFSIZE;
            }
#elif defined(CONFIG_EXAMPLES_SERIALRX_PRINTHEX)
          for (i = 0; i < (int)n; i++)
            {
              printf("0x%02x ", buf[i]);
            }

          fflush(stdout);
#endif                                  /* Kamal */
          cnt += n;
        }

      UNUSED(eof);
    }

#ifdef CONFIG_EXAMPLES_SERIALRX_PRINTHYPHEN
  printf("\n");
#endif
  printf("EOF reached\n");
  printf("Total bytes received = %d\n", cnt); /* Kamal */
  fflush(stdout);

#ifdef CONFIG_EXAMPLES_SERIALRX_BUFFERED
  fclose(f);
#else
  up_udelay(1000000);
  close(fd);
#endif

  free(buf);
  return EXIT_SUCCESS;

errout_with_buf:
  free(buf);

errout:
  fflush(stderr);
  return EXIT_FAILURE;
}
