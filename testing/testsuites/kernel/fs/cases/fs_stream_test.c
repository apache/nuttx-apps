/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_stream_test.c
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <errno.h>
#include <cmocka.h>
#include "fstest.h"
#define TESTFILENAME "streamTestfile"

/****************************************************************************
 * Name: stream
 * Example description:
 *   1. open a file with "a+".
 *   2. Write some strings to the file.
 *   3. Open it again and Write some strings.
 *   4. Check the returned results.
 * Test item: fopen() fwrite() freopen()
 * Expect results: TEST PASSED
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void test_nuttx_fs_stream01(FAR void **state)
{
  FILE *stream;
  char buf[10];
  int i;
  size_t ret;
  if ((stream = fopen(TESTFILENAME, "a+")) == NULL)
    {
      syslog(LOG_ERR, "open file fail !, errno %d\n", errno);
      assert_true(1 == 0);
    }

  fwrite("a", 1, 1, stream);
  if ((stream = freopen(TESTFILENAME, "a+", stream)) == NULL)
    {
      syslog(LOG_ERR, "reopen file fail !, errno %d\n", errno);
      assert_true(1 == 0);
    }

  fclose(stream);
  if ((stream = fopen(TESTFILENAME, "r")) == NULL)
    {
      syslog(LOG_ERR, "open file fail !, errno %d\n", errno);
      assert_true(1 == 0);
    }

  else
    {
      for (i = 0; i < 10; i++)
        buf[i] = 0;
      ret = fread(buf, 1, 1, stream);
      if ((buf[0] != 'a') || (buf[1] != 0) || ret == 0)
        {
          fclose(stream);
          syslog(LOG_ERR, "bad contents in \n");
          assert_true(1 == 0);
        }

      fclose(stream);
    }

  unlink(TESTFILENAME);
}

/****************************************************************************
 * Name: stream
 * Example description:
 *   1. open a file with "a+".
 *    2. Write the file multiple times.
 *    3. Check that the file pointer is in the correct position after
 *each write.
 *   4. Reset the file pointer position. Repeat step 2-3
 *   4. Check the returned results.
 * Test item: fopen() fwrite() rewind() ftell() fseek() fgets()
 * Expect results: TEST PASSED
 ****************************************************************************/

void test_nuttx_fs_stream02(FAR void **state)
{
  FILE *stream;
  char buf[30];
  char *junk = "abcdefghijklmnopqrstuvwxyz";
  long pos;
  int lc;
  for (lc = 0; lc < 100; lc++)
    {
      if ((stream = fopen(TESTFILENAME, "a+")) == NULL)
        {
          syslog(LOG_ERR, "open file failed !, errno %d\n", errno);
          assert_true(1 == 0);
        }

      pos = ftell(stream);
      if (pos != 0)
        {
          syslog(LOG_ERR, "file pointer descrepancy 1, errno %d\n",
                 errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      if (fwrite(junk, sizeof(*junk), strlen(junk), stream) == 0)
        {
          syslog(LOG_ERR, "fwrite failed, errno %d\n", errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      pos = ftell(stream);
      if (pos != strlen(junk))
        {
          syslog(
              LOG_ERR,
              "strlen(junk)=%zi: file pointer descrepancy 2 (pos=%li)",
              strlen(junk), pos);
          fclose(stream);
          assert_true(1 == 0);
        }

      usleep(1000);
      rewind(stream);
      pos = ftell(stream);
      if (pos != 0)
        {
          fclose(stream);
          syslog(LOG_ERR,
                 "file pointer descrepancy 3 (pos=%li, wanted pos=0)",
                 pos);
          assert_true(1 == 0);
        }

      if (fseek(stream, strlen(junk), 1) != 0)
        {
          syslog(LOG_ERR, "fseek failed !, errno %d\n", errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      pos = ftell(stream);
      if (pos != strlen(junk))
        {
          fclose(stream);
          syslog(
              LOG_ERR,
              "strlen(junk)=%zi: file pointer descrepancy 4 (pos=%li)",
              strlen(junk), pos);
          assert_true(1 == 0);
        }

      if (fseek(stream, 0, 2) != 0)
        {
          syslog(LOG_ERR, "fseek failed !, errno %d\n", errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      pos = ftell(stream);
      if (pos != strlen(junk))
        {
          fclose(stream);
          syslog(
              LOG_ERR,
              "strlen(junk)=%zi: file pointer descrepancy 5 (pos=%li)",
              strlen(junk), pos);
          assert_true(1 == 0);
        }

      if (fseek(stream, 0, 0) != 0)
        {
          syslog(LOG_ERR, "fseek failed !, errno %d\n", errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      pos = ftell(stream);
      if (pos != 0)
        {
          fclose(stream);
          syslog(LOG_ERR,
                 "file pointer descrepancy 6 (pos=%li, wanted pos=0)",
                 pos);
          assert_true(1 == 0);
        }

      while (fgets(buf, sizeof(buf), stream))
        ;
      pos = ftell(stream);
      if (pos != strlen(junk))
        {
          syslog(
              LOG_ERR,
              "strlen(junk)=%zi: file pointer descrepancy 7 (pos=%li)",
              strlen(junk), pos);
          assert_true(1 == 0);
        }

      fclose(stream);
      unlink(TESTFILENAME);
      usleep(40000);
    }
}

/****************************************************************************
 * Name: stream
 * Example description:
 *   1. open a file with "a+".
 *   2. Write the file multiple times.
 *   3. close the file.
 *   4. open the file again with "r+"
 *   5. Request a piece of memory and read the contents of the file.
 *   6. Check that the read file class is correct.
 *   7. repeat step 2-3-4-5-6 for 10 times.
 *   8. Check that the test returns results.
 * Test item: fopen() fwrite() fread() malloc()
 * Expect results: TEST PASSED
 ****************************************************************************/

void test_nuttx_fs_stream03(FAR void **state)
{
  FILE *stream;
  char *junk = "abcdefghijklmnopqrstuvwxyz";
  size_t len = strlen(junk);
  char *inbuf = NULL;
  ssize_t ret;
  int lc;
  for (lc = 0; lc < 10; lc++)
    {
      if ((stream = fopen(TESTFILENAME, "a+")) == NULL)
        {
          syslog(LOG_ERR, "fopen a+ failed, errno %d\n", errno);
          assert_true(1 == 0);
        }

      if ((ret = fwrite(junk, sizeof(*junk), len, stream)) == 0)
        {
          syslog(LOG_ERR, "fwrite failed, errno %d\n", errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      if (ret != len)
        {
          syslog(LOG_ERR, "len = %zu != return value from fwrite = %zd",
                 len, ret);
          fclose(stream);
          assert_true(1 == 0);
        }

      fclose(stream);
      if ((stream = fopen(TESTFILENAME, "r+")) == NULL)
        {
          syslog(LOG_ERR, "fopen r+ failed, errno %d\n", errno);
          assert_true(1 == 0);
        }

      if ((inbuf = malloc(len)) == 0)
        {
          syslog(LOG_ERR,
                 "test failed , because of malloc fail, errno %d\n",
                 errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      if ((ret = fread(inbuf, sizeof(*junk), len, stream)) == 0)
        {
          syslog(LOG_ERR, "fread failed, errno %d\n", errno);
          free(inbuf);
          fclose(stream);
          assert_true(1 == 0);
        }

      if (ret != len)
        {
          syslog(LOG_ERR, "len = %zu != return value from fread = %zd",
                 len, ret);
          free(inbuf);
          fclose(stream);
          assert_true(1 == 0);
        }

      /* Free memory */

      free(inbuf);
      fclose(stream);
      unlink(TESTFILENAME);
    }
}

/****************************************************************************
 * Name: stream
 * Example description:
 *   1. Open the file in a different mode, such as "a+" "r+" "rb" etc.
 *   2. Check that the test returns results.
 * Test item: fopen() fprintf() read() fileno() fclose()
 * Expect results: TEST PASSED
 ****************************************************************************/

void test_nuttx_fs_stream04(FAR void **state)
{
  FILE *stream;
  char buf[10];
  int nr;
  int fd;
  size_t ret;
  int lc;
  for (lc = 0; lc < 10; lc++)
    {
      if ((stream = fopen(TESTFILENAME, "a+")) == NULL)
        {
          syslog(LOG_ERR, "fopen a+ failed, errno %d\n", errno);
          assert_true(1 == 0);
        }

      fprintf(stream, "a");
      fclose(stream);

      if ((stream = fopen(TESTFILENAME, "r+")) == NULL)
        {
          syslog(LOG_ERR, "fopen r+ failed, errno %d\n", errno);
          assert_true(1 == 0);
        }

      if (ferror(stream) != 0)
        {
          syslog(LOG_ERR, "ferror did not return zero, return %d\n",
                 ferror(stream));
          fclose(stream);
          assert_true(1 == 0);
        }

      fd = fileno(stream);
      if ((nr = read(fd, buf, 1)) < 0)
        {
          syslog(LOG_ERR, "read failed !, errno %d\n", errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      if (nr != 1)
        {
          syslog(LOG_ERR,
                 "read did not read right number, nr = %d, errno %d\n",
                 nr, errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      if (buf[0] != 'a')
        {
          syslog(LOG_ERR, "read returned bad values %c\n", buf[0]);
          fclose(stream);
          assert_true(1 == 0);
        }

      fclose(stream);

      if ((stream = fopen(TESTFILENAME, "r+")) == NULL)
        {
          syslog(LOG_ERR, "fopen(%s) r+ failed, errno %d\n",
                 TESTFILENAME, errno);
          assert_true(1 == 0);
        }

      if (feof(stream) != 0)
        {
          syslog(LOG_ERR,
                 "feof returned non-zero when it should not \n");
          fclose(stream);
          assert_true(1 == 0);
        }

      ret = fread(buf, 1, 2, stream);
      buf[ret] = 0;
      if (feof(stream) == 0)
        {
          syslog(LOG_ERR, "feof returned zero when it should not \n");
          fclose(stream);
          assert_true(1 == 0);
        }

      fclose(stream);
      if ((stream = fopen(TESTFILENAME, "rb")) == NULL)
        {
          syslog(LOG_ERR, "fopen rb failed, errno %d\n", errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      fclose(stream);
      if ((stream = fopen(TESTFILENAME, "wb")) == NULL)
        {
          syslog(LOG_ERR, "fopen wb failed, errno %d\n", errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      fclose(stream);
      if ((stream = fopen(TESTFILENAME, "ab")) == NULL)
        {
          syslog(LOG_ERR, "fopen ab failed, errno %d\n", errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      fclose(stream);
      if ((stream = fopen(TESTFILENAME, "rb+")) == NULL)
        {
          syslog(LOG_ERR, "fopen rb+ failed");
          fclose(stream);
          assert_true(1 == 0);
        }

      fclose(stream);
      if ((stream = fopen(TESTFILENAME, "wb+")) == NULL)
        {
          syslog(LOG_ERR, "fopen wb+ failed, errno %d\n", errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      fclose(stream);
      if ((stream = fopen(TESTFILENAME, "ab+")) == NULL)
        {
          syslog(LOG_ERR, "fopen ab+ failed, errno %d\n", errno);
          fclose(stream);
          assert_true(1 == 0);
        }

      fclose(stream);
      unlink(TESTFILENAME);
    }
}
