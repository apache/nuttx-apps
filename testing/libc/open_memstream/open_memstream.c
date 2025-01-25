/****************************************************************************
 * apps/testing/libc/open_memstream/open_memstream.c
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

/****************************************************************************
 * Private data
 ****************************************************************************/

static char correctbuf[] = "abcdefghijklmnopqrstuvwxyz";
static char correctbuf_seek[] = "123defghijklmnopqrstuvw456";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: open_memstream_write_test
 ****************************************************************************/

static int open_memstream_write_test(void)
{
  FILE *stream;
  char *buf;
  size_t len;
  ssize_t size;
  int i;

  size = strlen(correctbuf);
  stream = open_memstream(&buf, &len);
  if (stream == NULL)
    {
      printf("open_memstream_write_test: open_memstream failed %d\n",
             -errno);
      return -1;
    }

  fprintf(stream, "%s", correctbuf);
  fflush(stream);

  /* Check whether values are updated after flush. */

  if (len != size)
    {
      printf("open_memstream_write_test: len (%zu) != size (%zu)\n", len,
              size);
      fclose(stream);
      free(buf);
      return -1;
    }

  for (i = 0; i < len; i++)
    {
      if (buf[i] != correctbuf[i])
        {
          printf("open_memstream_write_test: error %d got %c, expected %c\n",
                  i, buf[i], correctbuf[i]);
          fclose(stream);
          free(buf);
          return -1;
        }
    }

  fclose (stream);
  if (len != size)
    {
      printf("open_memstream_write_test: len (%zu) != size (%zu)\n", len,
              size);
      free(buf);
      return -1;
    }

  for (i = 0; i < len; i++)
    {
      if (buf[i] != correctbuf[i])
        {
          printf("open_memstream_write_test: error %d got %c, expected %c\n",
                  i, buf[i], correctbuf[i]);
          free(buf);
          return -1;
        }
    }

  free(buf);
  printf("open_memstream_write_test: successful\n");
  return 0;
}

/****************************************************************************
 * Name: open_memstream_seek_test
 ****************************************************************************/

static int open_memstream_seek_test(void)
{
  FILE *stream;
  char *buf;
  size_t len;
  off_t end_pos;
  ssize_t size;
  int i;

  size = strlen(correctbuf);

  stream = open_memstream(&buf, &len);
  if (stream == NULL)
    {
      printf("open_memstream_seek_test: open_memstream failed %d\n", -errno);
      return -1;
    }

  fprintf(stream, "%s", correctbuf);
  fflush(stream);
  end_pos = ftello(stream);
  fseeko(stream, 0, SEEK_SET);
  fprintf(stream, "123");
  fseeko(stream, end_pos, SEEK_SET);

  fseeko(stream, -3, SEEK_END);
  fprintf(stream, "456");
  fseeko(stream, end_pos, SEEK_SET);

  fclose(stream);

  if (len != size)
    {
      printf("open_memstream_seek_test: len (%zu) != size (%zu)\n", len,
              size);
      free(buf);
      return -1;
    }

  for (i = 0; i < len; i++)
    {
      if (buf[i] != correctbuf_seek[i])
        {
          printf("open_memstream_seek_test: error %d got %c, expected %c\n",
                  i, buf[i], correctbuf_seek[i]);
          free(buf);
          return -1;
        }
    }

  free(buf);
  printf("open_memstream_seek_test: successful\n");
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  int tests_ok;
  int tests_err;

  tests_ok = tests_err = 0;

  /* Run tests. They should return 0 on success, -1 otherwise. */

  ret = open_memstream_write_test();
  if (ret < 0)
    {
      tests_err += 1;
    }
  else
    {
      tests_ok += 1;
    }

  ret = open_memstream_seek_test();
  if (ret < 0)
    {
      tests_err += 1;
    }
  else
    {
      tests_ok += 1;
    }

  printf("open_memstream tests: SUCCESSFUL: %d; FAILED: %d\n", tests_ok,
          tests_err);

  return 0;
}
