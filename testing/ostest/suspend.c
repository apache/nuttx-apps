/****************************************************************************
 * apps/testing/ostest/suspend.c
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

#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <sched.h>
#include <errno.h>
#include "ostest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static int victim_main(int argc, char *argv[])
{
  printf("victim_main: Victim started\n");

  for (; ; )
    {
      sleep(3);
      printf("victim_main: Wasting time\n");
      FFLUSH();
    }

  return 0; /* Won't get here */
}

void suspend_test(void)
{
  struct sched_param param;
  pid_t victim;
  int ret;

  /* Start victim thread  */

  printf("suspend_test: Starting victim task\n");
  ret = sched_getparam (0, &param);
  if (ret < 0)
    {
      printf("suspend_test: ERROR sched_getparam() failed\n");
      ASSERT(false);
      param.sched_priority = PTHREAD_DEFAULT_PRIORITY;
    }

  victim = task_create("victim", param.sched_priority,
                           STACKSIZE, victim_main, NULL);
  if (victim == ERROR)
    {
      printf("suspend_test: ERROR failed to start victim_main\n");
      ASSERT(false);
    }
  else
    {
      printf("suspend_test: Started victim_main pid=%d\n", victim);
    }

  /* Wait a bit */

  printf("suspend_test:  Is the victim saying anything?\n");
  FFLUSH();
  sleep(10);

  /* Then signal the victim thread. */

  printf("suspend_test: Signaling pid=%d with SIGSTOP\n", victim);
  ret = kill(victim, SIGSTOP);
  if (ret < 0)
    {
      printf("suspend_test: ERROR kill() failed\n");
      ASSERT(false);
    }

  printf("suspend_test:  Is the victim still jabbering?\n");
  FFLUSH();
  sleep(10);

  printf("suspend_test: Signaling pid=%d with SIGCONT\n", victim);
  ret = kill(victim, SIGCONT);
  if (ret < 0)
    {
      printf("suspend_test: ERROR kill() failed\n");
      ASSERT(false);
    }

  printf("suspend_test:  The victim should continue the rant.\n");
  FFLUSH();
  sleep(10);

  printf("suspend_test: Signaling pid=%d with SIGKILL\n", victim);
  ret = kill(victim, SIGKILL);
  if (ret < 0)
    {
      printf("suspend_test: ERROR kill() failed\n");
      ASSERT(false);
    }

  FFLUSH();
  sleep(1);
  ret = kill(victim, 0);
  if (ret >= 0)
    {
      printf("suspend_test: ERROR kill() on the dead victim succeeded!\n");
      ASSERT(false);
    }

  printf("suspend_test: done\n");
  FFLUSH();
}
