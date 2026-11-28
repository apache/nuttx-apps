/****************************************************************************
 * apps/testing/ostest/flockfile.c
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

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "ostest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR void *flockfile_holder(FAR void *parameter)
{
  /* Exit while holding the lock of the stream */

  flockfile((FAR FILE *)parameter);
  return NULL;
}

static FAR void *flockfile_other(FAR void *parameter)
{
  FAR FILE *stream = parameter;

  if (ftrylockfile(stream) != 0)
    {
      return (FAR void *)1;
    }

  funlockfile(stream);
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void flockfile_test(void)
{
  FAR FILE *stream;
  FAR void *result;
  pthread_t thread;
  int fd;

  fd = dup(STDOUT_FILENO);
  stream = fdopen(fd, "w");
  if (stream == NULL)
    {
      printf("flockfile_test: ERROR: fdopen failed\n");
      ASSERT(false);
      close(fd);
      return;
    }

  printf("flockfile_test: Exiting a thread that holds the stream lock\n");
  pthread_create(&thread, NULL, flockfile_holder, stream);
  pthread_join(thread, NULL);

  /* Take the lock over from the exited thread and release it again */

  fflush(stream);

  printf("flockfile_test: Locking the stream from another thread\n");
  pthread_create(&thread, NULL, flockfile_other, stream);
  pthread_join(thread, &result);
  if (result != NULL)
    {
      printf("flockfile_test: ERROR: the stream stayed locked\n");
      ASSERT(false);
    }

  fclose(stream);
}
