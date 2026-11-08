/****************************************************************************
 * apps/testing/libc/mktemp/mktemp_main.c
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

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The calls must return at once.  Wait this many seconds for them. */

#define MKTEMP_TEST_TIMEOUT 10

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Static: the thread can outlive main() when the calls hang */

static sem_t g_done;
static int g_nerrors;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Make a template with a name longer than NAME_MAX: stat() fails with
 * ENAMETOOLONG for each name that mktemp() makes from it.
 */

static void make_template(FAR char *buf)
{
  buf[0] = '/';
  memset(&buf[1], 'a', NAME_MAX + 1);
  strcpy(&buf[NAME_MAX + 2], "XXXXXX");
}

static void check(FAR const char *name, bool failed, int errcode)
{
  if (!failed || errcode != ENAMETOOLONG)
    {
      printf("ERROR: %s: failed %d errno %d (expected ENAMETOOLONG)\n",
             name, failed, errcode);
      g_nerrors++;
    }
  else
    {
      printf("%s: OK\n", name);
    }
}

static FAR void *mktemp_thread(FAR void *arg)
{
  char path[NAME_MAX + 16];
  FAR char *ptr;
  int fd;

  make_template(path);
  errno = 0;
  ptr = mktemp(path);
  check("mktemp", ptr == NULL, errno);

  make_template(path);
  errno = 0;
  fd = mkstemp(path);
  check("mkstemp", fd < 0, errno);
  if (fd >= 0)
    {
      close(fd);
    }

  make_template(path);
  errno = 0;
  ptr = mkdtemp(path);
  check("mkdtemp", ptr == NULL, errno);

  sem_post(&g_done);
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct timespec abstime;
  pthread_t thread;
  int ret;

  printf("mktemp test: a template that stat() refuses\n");

  sem_init(&g_done, 0, 0);
  ret = pthread_create(&thread, NULL, mktemp_thread, NULL);
  if (ret != 0)
    {
      printf("ERROR: pthread_create() failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  /* If the calls do not return, the exit of the task ends the thread */

  clock_gettime(CLOCK_REALTIME, &abstime);
  abstime.tv_sec += MKTEMP_TEST_TIMEOUT;
  while ((ret = sem_timedwait(&g_done, &abstime)) < 0 && errno == EINTR);

  if (ret < 0)
    {
      printf("ERROR: the calls did not return in %d seconds\n",
             MKTEMP_TEST_TIMEOUT);
      return EXIT_FAILURE;
    }

  pthread_join(thread, NULL);
  sem_destroy(&g_done);

  printf("mktemp test: %d errors\n", g_nerrors);
  return g_nerrors > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
