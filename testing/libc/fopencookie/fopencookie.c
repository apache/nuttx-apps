/****************************************************************************
 * apps/testing/libc/fopencookie/fopencookie.c
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

#include <nuttx/config.h>

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BUF_SIZE 32

/****************************************************************************
 * Private data
 ****************************************************************************/

struct my_cookie
{
  char   *buf;
  size_t  endpos;
  size_t  bufsize;
  off_t   offset;
};

static const char correctbuf[] = "abcdefghijklmnopqrstuvwxyz";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

ssize_t fopencookie_write_hook(void *c, const char *buf, size_t size)
{
  char *new_buff;
  off_t realloc_size;
  struct my_cookie *cookie = (struct my_cookie *)c;

  DEBUGASSERT(c != NULL);

  if (size + cookie->offset > cookie->bufsize)
    {
      realloc_size = size + cookie->offset - cookie->bufsize + 1;
      new_buff = realloc(cookie->buf, realloc_size);
      if (new_buff == NULL)
        {
          fprintf(stderr, "ERROR: Cannot reallocate memory!\n");
          return -1;
        }

      cookie->buf = new_buff;
      cookie->bufsize = realloc_size;
    }

  memcpy(cookie->buf + cookie->offset, buf, size);

  cookie->offset += size;
  if (cookie->offset > cookie->endpos)
    {
      cookie->endpos = cookie->offset;
    }

  return size;
}

ssize_t fopencookie_read_hook(void *c, char *buf, size_t size)
{
  struct my_cookie *cookie = (struct my_cookie *)c;

  if (cookie->offset + size > cookie->endpos)
    {
      size = cookie->endpos - cookie->offset;
    }

  if (size < 0)
    {
      size = 0;
    }

  memcpy(buf, cookie->buf + cookie->offset, size);

  cookie->offset += size;
  return size;
}

off_t fopencookie_seek_hook(void *c, off_t *offset, int whence)
{
  off_t new_offset;
  struct my_cookie *cookie = (struct my_cookie *)c;

  if (whence == SEEK_SET)
    {
      new_offset = *offset;
    }

  else if (whence == SEEK_END)
    {
      new_offset = cookie->endpos + *offset;
    }

  else if (whence == SEEK_CUR)
    {
      new_offset = cookie->offset + *offset;
    }

  else
    {
      return -1;
    }

  if (new_offset < 0)
    {
      return -1;
    }

  cookie->offset = new_offset;
  *offset = new_offset;
  return new_offset;
}

int fopencookie_close_hook(void *c)
{
  struct my_cookie *cookie = (struct my_cookie *)c;

  cookie->bufsize = 0;

  return OK;
}

/****************************************************************************
 * Name: fopencookie_write_test
 ****************************************************************************/

static int fopencookie_write_test(FILE *stream)
{
  int n;

  n = fputs(correctbuf, stream);
  if (n < 0)
    {
      printf("fopencookie_write_test: fputs failed: %d\n", -errno);
      return -1;
    }

  printf("fopencookie_write_test: written buffer: %.*s\n", n, correctbuf);
  printf("fopencookie_write_test: succesfull\n");
  return 0;
}

/****************************************************************************
 * Name: fopencookie_write_test
 ****************************************************************************/

static int fopencookie_read_test(FILE *stream)
{
  int n;
  int endpos;
  char buf[256];

  fseek(stream, 0, SEEK_END);
  endpos = ftell(stream);
  fseek(stream, 0, SEEK_SET);

  n = fread(buf, 1, endpos, stream);

  for (int i = 0; i < n; i++)
    {
      if (correctbuf[i] != buf[i])
        {
          printf("fopencookie_write_test: error %d got %c, expected %c\n", i,
                 buf[i], correctbuf[i]);
          return -1;
        }
    }

  printf("fopencookie_read_test: read buffer: %.*s\n", n, correctbuf);
  printf("fopencookie_read_test: succesfull\n");
  return 0;
}

/****************************************************************************
 * Name: fopencookie_seek_test
 ****************************************************************************/

static int fopencookie_seek_test(FILE *stream)
{
  int endpos;
  int i;
  int n;
  char buf[1];

  fseek(stream, 0, SEEK_END);
  endpos = ftell(stream);
  fseek(stream, 0, SEEK_SET);

  for (i = 0; i < endpos; i += 2)
    {
      fseek(stream, i, SEEK_SET);
      n = fread(buf, 1, 1, stream);
      if (n != 1)
        {
          printf("fopencookie_seek_test: read at %i failed\n", i);
        }

      if (correctbuf[i] != buf[0])
        {
          printf("fopencookie_seek_test: error %d got %c, expected %c\n", i,
                 buf[0], correctbuf[i]);
          return -1;
        }

      printf("fopencookie_seek_test: pos %d, char %c\n", i, buf[i]);
    }

  printf("fopencookie_seek_test: succesfull\n");

  fseek(stream, 0, SEEK_SET);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  FILE *stream;
  struct my_cookie mycookie;
  cookie_io_functions_t cookie_funcs =
    {
      .read  = fopencookie_read_hook,
      .write = fopencookie_write_hook,
      .seek  = fopencookie_seek_hook,
      .close = fopencookie_close_hook
    };

  /* Set up the cookie before calling fopencookie(). */

  mycookie.buf = malloc(BUF_SIZE);
  if (mycookie.buf == NULL)
    {
      printf("Could not allocate memory for cookie.\n");
      return -1;
    }

  mycookie.bufsize = BUF_SIZE;
  mycookie.offset = 0;
  mycookie.endpos = 0;

  stream = fopencookie(&mycookie, "w+", cookie_funcs);
  if (stream == NULL)
    {
      free(mycookie.buf);
      return -1;
    }

  ret = fopencookie_write_test(stream);
  if (ret < 0)
    {
      goto fopencookie_ret;
    }

  ret = fopencookie_read_test(stream);
  if (ret < 0)
    {
      goto fopencookie_ret;
    }

  ret = fopencookie_seek_test(stream);
  if (ret < 0)
    {
      goto fopencookie_ret;
    }

  printf("fopencokie tests were succesfull.\n");

fopencookie_ret:
  fclose(stream);
  free(mycookie.buf);

  return ret;
}
