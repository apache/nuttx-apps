/****************************************************************************
 * apps/examples/watched/watched.c
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "watched.h"
#include <signal.h>

/****************************************************************************
 * Private Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct watcher_info_s g_watcher_info =
{
  .watcher_pid = -1,
  .info_file_name = "/mnt/watcher/info.txt"
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool watched_is_watcher_on(void)
{
  FILE *fp;

  /* Check if the watcher has already been initialized */

  fp = fopen(g_watcher_info.info_file_name, "r");
  if (fp)
    {
      fclose(fp);
      return true;
    }
  else
    {
      return false;
    }
}

int watched_read_watcher_info(struct watched_info_s *info)
{
  int ret = OK;
  int watched_pid = getpid();
  FILE *fp;

  /* Load Watcher Info in case it was not loaded yet */

  if (g_watcher_info.watcher_pid < 0)
    {
      /* Reading watcher value from file */

      fp = fopen(g_watcher_info.info_file_name, "r");
      if (fp == NULL)
        {
          int errcode = errno;
          fprintf(stderr, "ERROR: Failed to open %s: %d\n",
                  g_watcher_info.info_file_name, errcode);
          ret = errcode;
          goto errout;
        }

      fscanf(fp, "%d %d %d %d %d", (int *)&(g_watcher_info.watcher_pid),
             &(g_watcher_info.signal), &(g_watcher_info.sub_cmd),
             &(g_watcher_info.feed_cmd), &(g_watcher_info.unsub_cmd));

      fclose(fp);
    }

  /* Initialize Wacthed Info */

  info->sub_request.task_id = watched_pid;
  info->sub_request.code = g_watcher_info.sub_cmd;
  info->sub_value.sival_ptr = &(info->sub_request);

  info->unsub_request.task_id = watched_pid;
  info->unsub_request.code = g_watcher_info.unsub_cmd;
  info->unsub_value.sival_ptr = &(info->unsub_request);

  info->feed_request.task_id = watched_pid;
  info->feed_request.code = g_watcher_info.feed_cmd;
  info->feed_value.sival_ptr = &(info->feed_request);

errout:
  return ret;
}

int watched_subscribe(struct watched_info_s *info)
{
  int ret;

  /* Signal watcher = Request to subscribe */

  ret =
    sigqueue(g_watcher_info.watcher_pid, g_watcher_info.signal,
             info->sub_value);
  if (ret == ERROR)
    {
      int errcode = errno;
      fprintf(stderr, "subscribe: error %d\n", errcode);
      ret = errcode;
    }

  return ret;
}

int watched_unsubscribe(struct watched_info_s *info)
{
  int ret;

  /* Signal watcher = Request to unsubscribe */

  ret =
    sigqueue(g_watcher_info.watcher_pid, g_watcher_info.signal,
             info->unsub_value);
  if (ret == ERROR)
    {
      int errcode = errno;
      fprintf(stderr, "unsubscribe: error %d\n", errcode);
      ret = errcode;
    }

  return ret;
}

int watched_feed_dog(struct watched_info_s *info)
{
  int ret;

  /* Signal watcher = Request to feed the dog */

  ret =
    sigqueue(g_watcher_info.watcher_pid, g_watcher_info.signal,
             info->feed_value);
  if (ret == ERROR)
    {
      int errcode = errno;
      fprintf(stderr, "watched_feed_dog: error %d\n", errcode);
      ret = errcode;
    }

  return ret;
}
