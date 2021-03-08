/****************************************************************************
 * apps/examples/watched/watched_main.c
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
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "watched.h"

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
 * Name: Task that feeds the dog
 ****************************************************************************/

static int task1(int argc, FAR char *argv[])
{
  struct watched_info_s watched_info;

  /* Get the necessary information from watcher */

  watched_read_watcher_info(&watched_info);

  watched_subscribe(&watched_info);

  for (; ; )
    {
      watched_feed_dog(&watched_info);
      sleep(3);
    }

  return OK;
}

/****************************************************************************
 * Name: Task that doesn't feed the dog
 ****************************************************************************/

static int task2(int argc, FAR char *argv[])
{
  int counter = 0;
  struct watched_info_s watched_info;

  /* Get the necessary information from watcher */

  watched_read_watcher_info(&watched_info);

  watched_subscribe(&watched_info);

  for (; ; )
    {
      sleep((CONFIG_EXAMPLES_WATCHER_TIMEOUT) / 1000);
      counter++;
      if (counter == 5)
        {
          watched_unsubscribe(&watched_info);
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: null_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int ret = OK;

  /* Check if the watcher has already been initialized */

  if (!watched_is_watcher_on())
    {
      printf("Please, enable the watcher service "
             "before subscribing tasks!\n");
      ret = ENOENT;
      goto errout;
    }

  printf("Starting watched tasks\n");

  /* Starting Tasks Tasks 1 and 4 will subscribe, but they will not feed the
   * dog. Tasks 2 and 3 will subscribe and will feed the dog each 3 secs.
   */

  printf("Creating Watched Task 1 - It will not feed the dog\n");

  ret = task_create("Watched Task 1", CONFIG_EXAMPLES_WATCHED_PRIORITY,
                    CONFIG_EXAMPLES_WATCHED_STACKSIZE, task2, NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("watched_main: ERROR: Failed to start Watched Task 1: %d\n",
             errcode);
      ret = errcode;
      goto errout;
    }

  printf("Creating Watched Task 2 - It will feed the dog\n");

  ret = task_create("Watched Task 2", CONFIG_EXAMPLES_WATCHED_PRIORITY,
                    CONFIG_EXAMPLES_WATCHED_STACKSIZE, task1, NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("watched_main: ERROR: Failed to start Watched Task 2: %d\n",
             errcode);
      ret = errcode;
      goto errout;
    }

  printf("Creating Watched Task 3 - It will feed the dog\n");

  ret = task_create("Watched Task 3", CONFIG_EXAMPLES_WATCHED_PRIORITY,
                    CONFIG_EXAMPLES_WATCHED_STACKSIZE, task1, NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("watched_main: ERROR: Failed to start Watched Task 3: %d\n",
             errcode);
      ret = errcode;
      goto errout;
    }

  printf("Creating Watched Task 4 - It will not feed the dog\n");

  ret = task_create("Watched Task 4", CONFIG_EXAMPLES_WATCHED_PRIORITY,
                    CONFIG_EXAMPLES_WATCHED_STACKSIZE, task2, NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("watched_main: ERROR: Failed to start Watched Task 4: %d\n",
             errcode);
      ret = errcode;
      goto errout;
    }

errout:
  return ret;
}
