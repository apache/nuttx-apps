/****************************************************************************
 * apps/testing/libc/fmemopen/fmemopen.c
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

static char correctbuf[53] = "abcdefghijklmnopqrstuvwxyz";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fmemopen_read_test
 ****************************************************************************/

static int fmemopen_read_test(void)
{
  FILE *stream;
  ssize_t size;
  char buf[256];
  int n;

  size = strlen(correctbuf);

  /* Open for read only */

  stream = fmemopen(correctbuf, size, "r");
  if (stream == NULL)
    {
      return -1;
    }

  /* Read buffer and check whether characters match. */

  n = fread(buf, 1, size, stream);
  for (int i = 0; i < n; i++)
    {
      if (correctbuf[i] != buf[i])
        {
          printf("fmemopen_read_test: error %d got %c, expected %c\n", i,
                 buf[i], correctbuf[i]);
          fclose(stream);
          return -1;
        }
    }

  printf("fmemopen_read_test: read buffer: %.*s\n", n, buf);

  /* Try writing, this should fail */

  n = fputs(correctbuf, stream);
  if (n >= 0)
    {
      printf("fmemopen_read_test: error: fputs was successful but"
             "write not permited!.\n");
      fclose(stream);
      return -1;
    }

  printf("fmemopen_read_test: successful\n");
  fclose(stream);
  return 0;
}

/****************************************************************************
 * Name: fmemopen_write_test
 ****************************************************************************/

static int fmemopen_write_test(void)
{
  FILE *stream;
  char buf[256];
  int n;

  /* Open for writing and reading. */

  stream = fmemopen(NULL, 256, "w+");
  if (stream == NULL)
    {
      return -1;
    }

  n = fputs(correctbuf, stream);
  if (n < 0)
    {
      printf("fmemopen_write_test: fputs failed.\n");
      fclose(stream);
      return -1;
    }

  /* Seek to begining */

  fseek(stream, 0, SEEK_SET);

  /* And read written buffer. */

  n = fread(buf, 1, 256, stream);
  for (int i = 0; i < n; i++)
    {
      if (correctbuf[i] != buf[i])
        {
          printf("fmemopen_write_test: error %d got %c, expected %c\n", i,
                 buf[i], correctbuf[i]);
          fclose(stream);
          return -1;
        }
    }

  printf("fmemopen_write_test: read buffer: %.*s\n", n, buf);
  printf("fmemopen_write_test: successful\n");
  fclose(stream);
  return 0;
}

/****************************************************************************
 * Name: fmemopen_seek_test
 ****************************************************************************/

static int fmemopen_seek_test(void)
{
  FILE *stream;
  ssize_t size;
  char buf[1];
  int n;

  size = strlen(correctbuf);

  stream = fmemopen(correctbuf, size, "r");
  if (stream == NULL)
    {
      return -1;
    }

  /* SEEK_END test */

  n = fseek(stream, -1, SEEK_END);
  if (n < 0)
    {
      printf("fmemopen_seek_test: error SEEK_END: %d\n", -errno);
      fclose(stream);
      return -1;
    }

  n = fread(buf, 1, 1, stream);
  if ((n != 1) || (correctbuf[size - 1] != buf[0]))
    {
      printf("fmemopen_seek_test: read at SEEK_END failed:"
             "expected: %c, read %c\n", correctbuf[size - 1], buf[0]);
      fclose(stream);
      return -1;
    }

  /* SEEK_SET test */

  n = fseek(stream, 0, SEEK_SET);
  if (n < 0)
    {
      printf("fmemopen_seek_test: error SEEK_SET: %d\n", -errno);
      fclose(stream);
      return -1;
    }

  n = fread(buf, 1, 1, stream);
  if ((n != 1) || (correctbuf[0] != buf[0]))
    {
      printf("fmemopen_seek_test: read at SEEK_SET failed:"
             "expected: %c, read %c\n", correctbuf[0], buf[0]);
      fclose(stream);
      return -1;
    }

  /* SEEK_CUR test. Note that current position is 1 because of previous
   * read, therefore the check has to be correctbuf[10] != buf[0].
   */

  n = fseek(stream, 9, SEEK_CUR);
  if (n < 0)
    {
      printf("fmemopen_seek_test: error SEEK_CUR: %d\n", -errno);
      fclose(stream);
      return -1;
    }

  n = fread(buf, 1, 1, stream);
  if ((n != 1) || (correctbuf[10] != buf[0]))
    {
      printf("fmemopen_seek_test: read at SEEK_CUR failed:"
             "expected: %c, read %c\n", correctbuf[10], buf[0]);
      fclose(stream);
      return -1;
    }

  fclose(stream);
  printf("fmemopen_seek_test: successful\n");
  return 0;
}

/****************************************************************************
 * Name: fmemopen_seek_test
 ****************************************************************************/

static int fmemopen_append_test(void)
{
  FILE *stream;
  char buf[256];
  int n;

  /* Add termination to the half of the buffer. This is to ensure we
   * can perform the test multiple times if required.
   */

  correctbuf[26] = '\0';

  stream = fmemopen(correctbuf, 53, "a+");
  if (stream == NULL)
    {
      return -1;
    }

  n = fputs(correctbuf, stream);
  if (n < 0)
    {
      printf("fmemopen_append_test: fputs failed.\n");
      fclose(stream);
      return -1;
    }

  n = fseek(stream, 0, SEEK_SET);

  n = fread(buf, 1, 256, stream);
  for (int i = 0; i < n; i++)
    {
      if (correctbuf[i % 26] != buf[i])
        {
          printf("fmemopen_append_test: error %d got %c, expected %c\n", i,
                 buf[i], correctbuf[i]);
          fclose(stream);
          return -1;
        }
    }

  printf("fmemopen_append_test: read buffer: %.*s\n", n, buf);
  printf("fmemopen_append_test: successful\n");
  fclose(stream);
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

  ret = fmemopen_read_test();
  if (ret < 0)
    {
      tests_err += 1;
    }
  else
    {
      tests_ok += 1;
    }

  ret = fmemopen_write_test();
  if (ret < 0)
    {
      tests_err += 1;
    }
  else
    {
      tests_ok += 1;
    }

  ret = fmemopen_seek_test();
  if (ret < 0)
    {
      tests_err += 1;
    }
  else
    {
      tests_ok += 1;
    }

  ret = fmemopen_append_test();
  if (ret < 0)
    {
      tests_err += 1;
    }
  else
    {
      tests_ok += 1;
    }

  printf("fmemopen tests: SUCCESSFUL: %d; FAILED: %d\n", tests_ok,
          tests_err);

  return 0;
}
