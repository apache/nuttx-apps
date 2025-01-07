/****************************************************************************
 * apps/examples/pipe/interlock_test.c
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

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "pipe.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: null_reader
 ****************************************************************************/

static void *null_reader(pthread_addr_t pvarg)
{
  int fd;

  /* Wait a bit */

  printf("null_reader: started -- sleeping\n");
  sleep(5);

  /* Then open the FIFO for read access */

  printf("null_reader: Opening FIFO for read access\n");
  fd = open(FIFO_PATH2, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, \
              "null_reader: Failed to open FIFO %s for reading, errno=%d\n",
              FIFO_PATH2, errno);
      return (void *)(uintptr_t)1;
    }

  /* Wait a bit more */

  printf("null_reader: Opened %s for reading -- sleeping\n", FIFO_PATH2);
  sleep(5);

  /* Then close the FIFO */

  printf("null_reader: Closing %s\n", FIFO_PATH2);
  if (close(fd) != 0)
    {
      fprintf(stderr, "null_reader: close failed: %d\n", errno);
    }

  sleep(5);

  printf("null_reader: Returning success\n");
  return NULL;
}

/****************************************************************************
 * Name: null_writer
 ****************************************************************************/

static void *null_writer(pthread_addr_t pvarg)
{
  int fd;

  /* Wait a bit */

  printf("null_writer: started -- sleeping\n");
  sleep(5);

  /* Then open the FIFO for write access */

  printf("null_writer: Opening FIFO for write access\n");
  fd = open(FIFO_PATH2, O_WRONLY);
  if (fd < 0)
    {
      fprintf(stderr, \
              "null_writer: Failed to open FIFO %s for writing, errno=%d\n",
              FIFO_PATH2, errno);
      return (void *)(uintptr_t)1;
    }

  /* Wait a bit more */

  printf("null_writer: Opened %s for writing -- sleeping\n", FIFO_PATH2);
  sleep(5);

  /* Then close the FIFO */

  printf("null_writer: Closing %s\n", FIFO_PATH2);
  if (close(fd) != 0)
    {
      fprintf(stderr, "null_writer: close failed: %d\n", errno);
    }

  sleep(5);

  printf("null_writer: Returning success\n");
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: interlock_test
 ****************************************************************************/

int interlock_test(void)
{
  pthread_t readerid;
  pthread_t writerid;
  void *value;
  char data[16];
  ssize_t nbytes;
  int fd = -1;
  int ret;

  /* Create a FIFO */

  ret = mkfifo(FIFO_PATH2, 0666);
  if (ret < 0)
    {
      fprintf(stderr, \
              "interlock_test: mkfifo failed with errno=%d\n", errno);
      return 1;
    }

  /* Start the null_writer thread */

  printf("interlock_test: Starting null_writer thread\n");
  ret = pthread_create(&writerid, NULL, null_writer, NULL);
  if (ret != 0)
    {
      fprintf(stderr, \
              "interlock_test: Failed to create null_writer thread,"
              "error=%d\n", ret);
      ret = 2;
      goto errout_with_fifo;
    }

  /* Open one end of the FIFO for reading. This open call should block until
   * the null_writer thread opens the other end of the FIFO for writing.
   */

  printf("interlock_test: Opening FIFO for read access\n");
  fd = open(FIFO_PATH2, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, \
              "interlock_test: Failed to open FIFO %s for reading"
              "errno=%d\n",
              FIFO_PATH2, errno);
      ret = 3;
      goto errout_with_null_writer_thread;
    }

  /* Attempt to read one byte from the FIFO. This should return end-of-file
   * because the null_writer closes the FIFO without writing anything.
   */

  printf("interlock_test: Reading from %s\n", FIFO_PATH2);
  nbytes = read(fd, data, 16);
  if (nbytes < 0)
    {
      fprintf(stderr, \
              "interlock_test: read failed, errno=%d\n", errno);
      ret = 4;
      goto errout_with_null_writer_thread;
    }
  else if (ret != 0)
    {
      fprintf(stderr, \
              "interlock_test: Read %ld bytes of data -- aborting: %d\n",
              (long)nbytes, errno);
      ret = 5;
      goto errout_with_null_writer_thread;
    }

  printf("interlock_test: read returned\n");

  /* Close the file */

  printf("interlock_test: Closing %s\n", FIFO_PATH2);
  if (close(fd) != 0)
    {
      fprintf(stderr, "interlock_test: close failed: %d\n", errno);
    }

  fd = -1;

  /* Wait for null_writer thread to complete */

  printf("interlock_test: Waiting for null_writer thread\n");
  ret = pthread_join(writerid, &value);
  if (ret != 0)
    {
      fprintf(stderr, \
              "interlock_test: pthread_join failed, error=%d\n", ret);
      ret = 6;
      goto errout_with_fifo;
    }
  else
    {
      printf("interlock_test: writer returned %p\n", value);
      if (value != NULL)
        {
          ret = 7;
          goto errout_with_fifo;
        }
    }

  /* Start the null_reader thread */

  printf("interlock_test: Starting null_reader thread\n");
  ret = pthread_create(&readerid, NULL, null_reader, NULL);
  if (ret != 0)
    {
      fprintf(stderr, \
              "interlock_test: Failed to create null_reader thread,"
              "error=%d\n", ret);
      ret = 8;
      goto errout_with_fifo;
    }

  /* Open one end of the FIFO for writing. This open call should block until
   * the null_reader thread opens the other end of the FIFO for reading.
   */

  printf("interlock_test: Opening FIFO for write access\n");
  fd = open(FIFO_PATH2, O_WRONLY);
  if (fd < 0)
    {
      fprintf(stderr, \
              "interlock_test: Failed to open FIFO %s for writing,"
              "errno=%d\n",
              FIFO_PATH2, errno);
      ret = 9;
      goto errout_with_null_reader_thread;
    }

  /* Attempt to write one byte from the FIFO. This should return 0 bytes
   * written because the null_reader closes the FIFO.
   */

  printf("interlock_test: Writing to %s\n", FIFO_PATH2);
  nbytes = write(fd, data, 16);
  if (nbytes < 0)
    {
      fprintf(stderr, \
              "interlock_test: write failed, errno=%d\n", errno);
      ret = 10;
      goto errout_with_null_reader_thread;
    }
  else if (ret != 0)
    {
      fprintf(stderr, \
              "interlock_test: Wrote %ld bytes of data -- aborting: %d\n",
              (long)nbytes, errno);
      ret = 11;
      goto errout_with_null_reader_thread;
    }

  printf("interlock_test: write returned\n");

  /* Close the file */

  printf("interlock_test: Closing %s\n", FIFO_PATH2);
  if (close(fd) != 0)
    {
      fprintf(stderr, "interlock_test: close failed: %d\n", errno);
    }

  fd = -1;

  /* Wait for null_reader thread to complete */

  printf("interlock_test: Waiting for null_reader thread\n");
  ret = pthread_join(readerid, &value);
  if (ret != 0)
    {
      fprintf(stderr, \
              "interlock_test: pthread_join failed, error=%d\n", ret);
      ret = 12;
      goto errout_with_fifo;
    }
  else
    {
      printf("interlock_test: reader returned %p\n", value);
      if (value != NULL)
        {
          ret = 13;
          goto errout_with_fifo;
        }
    }

  ret = 0;
  goto errout_with_fifo;

errout_with_null_reader_thread:
  pthread_detach(readerid);
  pthread_cancel(readerid);

errout_with_null_writer_thread:
  pthread_detach(writerid);
  pthread_cancel(writerid);

errout_with_fifo:

  if (fd != -1 && close(fd) != 0)
    {
      fprintf(stderr, "interlock_test: close failed: %d\n", errno);
    }

  ret = remove(FIFO_PATH2);
  if (ret != 0)
    {
      fprintf(stderr, \
              "interlock_test: remove failed with errno=%d\n", errno);
    }

  printf("interlock_test: Returning %d\n", ret);
  return ret;
}
