/****************************************************************************
 * apps/examples/watcher/watcher_main.c
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
#include <sched.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "ramdisk.h"
#include "wdt.h"
#include "task_mn.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SUBSCRIBE_CMD      1
#define UNSUSBCRIBE_CMD   -1
#define FEED_CMD           2

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_info_file_name[] = "/mnt/watcher/info.txt";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void feed_sighandler(int signo, FAR siginfo_t * siginfo,
                            FAR void *context)
{
  struct task_node_s *node;
  request = *(struct request_s *)siginfo->si_value.sival_ptr;

  switch (request.code)
    {
    case FEED_CMD:

      /* Update the current requester task status */

      node = task_mn_is_task_subscribed(request.task_id);
      if (node != NULL)
        {
          node->reset = true;
        }
      else
        {
          fprintf(stderr,
                  "watcher daemon: task is not subscribed to feed dog.\n");
        }

      /* Verify if all tasks requested to feed the dog */

      if (task_mn_all_tasks_fed())
        {
          /* If all tasks required, reset it and reset all tasks' status" */

          wdt_feed_the_dog();
          task_mn_reset_all();
        }

      break;
    case SUBSCRIBE_CMD:

      /* Include the current requester task to the watched tasks list */

      task_mn_subscribe(request.task_id);
      break;
    case UNSUSBCRIBE_CMD:

      /* Excludes the current requester task from the watched tasks list */

      task_mn_unsubscribe(request.task_id);

      /* Verify if all tasks has already requested to feed the dog in this
       * round. Because maybe the watcher was only expecting the task that
       * was unsubscribed to finally reset the dog.
       * If this task is no longer being watched and the others have already
       * sent a feed request, so it's time to feed the dog.
       */

      if (task_mn_all_tasks_fed())
        {
          /* If all tasks required, reset it and reset all tasks' status" */

          wdt_feed_the_dog();
          task_mn_reset_all();
        }
      break;
    default:
      fprintf(stderr, "watcher daemon: Invalid command\n");
    }
}

static void wdt_log(int signo, FAR siginfo_t * siginfo,
                            FAR void *context)
{
  if (watched_tasks.head != NULL)
    {
      printf("*** Printing Tasks Status ***\n");
      task_mn_print_tasks_status();
      task_mn_reset_all();
    }
}

/****************************************************************************
 * Name: watcher_daemon
 ****************************************************************************/

static int watcher_daemon(int argc, FAR char *argv[])
{
  int ret;
  struct sigaction act;
  struct sigaction act_log;
  pid_t watcher_pid;
  FILE *fp;

  printf("Watcher Daemon has started!\n");

  /* Initialize and configure the wdt */

  wdt_init();

  /* Configure signals action */

  /* Configure signal to receive the subscribe/unsubscribe
   *  and feed requests
   */

  act.sa_sigaction = feed_sighandler;   /* The handler to be triggered when
                                         * receiving a signal */
  act.sa_flags = SA_SIGINFO;            /* Invoke the signal-catching function */
  sigfillset(&act.sa_mask);

  /* Block all other signals less this one */

  sigdelset(&act.sa_mask, CONFIG_EXAMPLES_WATCHER_SIGNAL);

  ret = sigaction(CONFIG_EXAMPLES_WATCHER_SIGNAL, &act, NULL);
  if (ret != OK)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: sigaction failed: %d\n", errcode);
      ret = errcode;
      goto errout;
    }

  /* Configure signal to log the taks at wdt timeout */

  act_log.sa_sigaction = wdt_log;       /* The handler to be triggered when
                                         * receiving a signal */
  act_log.sa_flags = SA_SIGINFO;        /* Invoke the signal-catching function */
  sigfillset(&act_log.sa_mask);

  /* Block all other signals less this one */

  sigdelset(&act_log.sa_mask, CONFIG_EXAMPLES_WATCHER_SIGNAL_LOG);

  ret = sigaction(CONFIG_EXAMPLES_WATCHER_SIGNAL_LOG, &act_log, NULL);
  if (ret != OK)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: sigaction failed: %d\n", errcode);
      ret = errcode;
      goto errout;
    }

  /* Collecting the necessary information of the current task */

  watcher_pid = getpid();

  /* Writing PID, SIGNAL NUMBER, and COMMANDS' value to a file */

  fp = fopen(g_info_file_name, "w+");
  if (fp == NULL)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              g_info_file_name, errcode);
      ret = errcode;
      goto errout;
    }

  fprintf(fp, "%d %d %d %d %d\n", (int)watcher_pid,
          (int)CONFIG_EXAMPLES_WATCHER_SIGNAL, (int)SUBSCRIBE_CMD,
          (int)FEED_CMD, (int)UNSUSBCRIBE_CMD);
  fclose(fp);

  /* Suspends the calling thread until delivery of a non-blocked signal. */

  while (1)
    {
      pause();
    }

errout:
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * watcher_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  FILE *fp;

  /* Check if the watcher has already been initialized */

  fp = fopen(g_info_file_name, "r");
  if (fp)
    {
      fclose(fp);
      printf("Watcher is already running.\n");
      ret = EBUSY;
      goto errout;
    }

  /* Create a RAMDISK device, format it and mount it */

  prepare_fs();

  /* Start Daemon */

  ret = task_create("watcher_daemon", CONFIG_EXAMPLES_WATCHER_PRIORITY,
                    CONFIG_EXAMPLES_WATCHER_STACKSIZE, watcher_daemon, NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("watcher_main: ERROR: Failed to start watcher_daemon: %d\n",
             errcode);
      ret = errcode;
      goto errout;
    }

errout:
  return ret;

  return EXIT_SUCCESS;
}
