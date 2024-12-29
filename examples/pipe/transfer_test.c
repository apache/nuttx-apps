/****************************************************************************
 * apps/examples/pipe/transfer_test.c
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
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "pipe.h"

/****************************************************************************
 * Pre-proecessor Definitions
 ****************************************************************************/

#define MAX_BYTE      13

#define WRITE_SIZE    MAX_BYTE
#define NWRITES       1400
#define NWRITE_BYTES (NWRITES * WRITE_SIZE)

#define READ_SIZE    (2*MAX_BYTE)
#define NREADS       (NWRITES / 2)
#define NREAD_BYTES   NWRITE_BYTES

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
 * Name: transfer_reader
 ****************************************************************************/

static void *transfer_reader(pthread_addr_t pvarg)
{
  char buffer[READ_SIZE];
  int fd = (intptr_t)pvarg;
  int ret;
  int nbytes;
  int value;
  int ndx;

  printf("transfer_reader: started\n");
  for (nbytes = 0, value = 0; nbytes < NREAD_BYTES; )
    {
      ret = read(fd, buffer, READ_SIZE);
      if (ret < 0)
        {
          fprintf(stderr, \
                  "transfer_reader: read failed, errno=%d\n",
                  errno);

          return (void *)(uintptr_t)1;
        }
      else if (ret == 0)
        {
          if (nbytes < NREAD_BYTES)
            {
              fprintf(stderr, \
                      "transfer_reader: Too few bytes read. Aborting: %d\n",
                      nbytes);

              return (void *)(uintptr_t)2;
            }
            break;
        }

      for (ndx = 0; ndx < ret; ndx++)
        {
          if (value >= WRITE_SIZE)
            {
              value = 0;
            }

          if (buffer[ndx] != value)
            {
              fprintf(stderr, \
                      "transfer_reader: Byte %d, expected %d, found %d\n",
                      nbytes + ndx, value, buffer[ndx]);

              return (void *)(uintptr_t)3;
            }

          value++;
        }

      nbytes += ret;
      if (nbytes > NREAD_BYTES)
        {
          fprintf(stderr, \
                  "transfer_reader: Too many bytes read. Aborting: %d\n",
                  nbytes);

          return (void *)(uintptr_t)4;
        }
    }

  printf("transfer_reader: %d bytes read\n", nbytes);

  return NULL;
}

/****************************************************************************
 * Name: transfer_writer
 ****************************************************************************/

static void *transfer_writer(pthread_addr_t pvarg)
{
  char buffer[WRITE_SIZE];
  int fd = (intptr_t)pvarg;
  int ret;
  int nbytes;
  int i;

  printf("transfer_writer: started\n");
  for (i = 0; i < WRITE_SIZE; i++)
    {
      buffer[i] = i;
    }

  for (i = 0; i < NWRITES; i++)
    {
      for (nbytes = 0; nbytes < WRITE_SIZE; )
        {
          ret = write(fd, buffer + nbytes, WRITE_SIZE - nbytes);
          if (ret < 0)
            {
              fprintf(stderr, \
                      "transfer_writer: write failed, errno=%d\n", errno);
              return (void *)(uintptr_t)1;
            }
          else if (ret == 0)
            {
              fprintf(stderr, \
                      "transfer_writer: Unexpected zero write size\n");
              return (void *)(uintptr_t)2;
            }

          nbytes += ret;
        }
    }

  printf("transfer_writer: %d bytes written\n", NWRITE_BYTES);
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: transfer_test
 ****************************************************************************/

int transfer_test(int fdin, int fdout, int boost_reader, int boost_writer)
{
  pthread_t readerid;
  pthread_t writerid;
  pthread_attr_t rattr;
  pthread_attr_t wattr;
  void *value;
  int tmp;
  int ret;

  printf("transfer_test: fdin=%d fdout=%d\n", fdin, fdout);

  /* Boost the priority of reader or writer on need */

  pthread_attr_init(&rattr);
  if (boost_reader)
    {
      rattr.priority += 1;
      printf("transfer_test: Boost priority of transfer_reader"
             "thread to %d\n", rattr.priority);
    }

  /* Start transfer_reader thread */

  printf("transfer_test: Starting transfer_reader thread\n");
  ret = pthread_create(&readerid, &rattr,
        transfer_reader, (void *)(intptr_t)fdin);
  pthread_attr_destroy(&rattr);
  if (ret != 0)
    {
        fprintf(stderr, \
                "transfer_test: Failed to create transfer_reader thread,"
                "error=%d\n", ret);
      return 1;
    }

  /* Boost the priority of reader or writer on need */

  pthread_attr_init(&wattr);
  if (boost_writer)
    {
      wattr.priority += 1;
      printf("transfer_test: Boost priority of transfer_writer"
             "thread to %d\n", wattr.priority);
    }

  /* Start transfer_writer thread */

  printf("transfer_test: Starting transfer_writer thread\n");
  ret = pthread_create(&writerid, &wattr,
        transfer_writer, (void *)(intptr_t)fdout);
  pthread_attr_destroy(&wattr);
  if (ret != 0)
    {
      fprintf(stderr, \
              "transfer_test: Failed to create transfer_writer thread,"
              "error=%d\n", ret);
      pthread_detach(readerid);
      ret = pthread_cancel(readerid);
      if (ret != 0)
        {
          fprintf(stderr, \
                  "transfer_test: Failed to cancel transfer_reader thread,"
                  "error=%d\n", ret);
        }

      return 2;
    }

  /* Wait for transfer_writer thread to complete */

  printf("transfer_test: Waiting for transfer_writer thread\n");
  ret = pthread_join(writerid, &value);
  if (ret != 0)
    {
      fprintf(stderr, "transfer_test: pthread_join failed, error=%d\n", ret);
    }
  else
    {
      ret = (intptr_t)value;
      printf("transfer_test: transfer_writer returned %d\n", ret);
    }

  /* Wait for transfer_reader thread to complete */

  printf("transfer_test: Waiting for transfer_reader thread\n");
  tmp = pthread_join(readerid, &value);
  if (tmp != 0)
    {
      fprintf(stderr, "transfer_test: pthread_join failed, error=%d\n", ret);
    }
  else
    {
      tmp = (intptr_t)value;
      printf("transfer_test: transfer_reader returned %d\n", tmp);
    }

  if (ret == 0)
    {
      ret = tmp;
    }

  printf("transfer_test: returning %d\n", ret);
  return ret;
}
