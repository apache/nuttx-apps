/****************************************************************************
 * apps/examples/serialrx/serialrx_main.c
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
#if defined (CONFIG_EXAMPLES_SERIALRX_PRINTHEX) || \
    defined (CONFIG_EXAMPLES_SERIALRX_PRINTSTR)
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
#elif defined(CONFIG_EXAMPLES_SERIALRX_PRINTSTR)
          for (i = 0; i < (int)n; i++)
            {
              printf("%c", buf[i]);
            }

          fflush(stdout);
#endif
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
