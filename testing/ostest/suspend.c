/****************************************************************************
 * apps/testing/ostest/suspend.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
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
 * Private Functions
 ****************************************************************************/

static int victim_main(int argc, char *argv[])
{
  printf("victim_main: Victim started\n" );

  for (; ; )
    {
      sleep(3);
      printf("victim_main: Wasting time\n" );
      FFLUSH();
    }

  return 0; /* Won't get here */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void suspend_test(void)
{
  struct sched_param param;
  pid_t victim;
  int ret;

  /* Start victim thread  */

  printf("suspend_test: Starting victim task\n" );
  ret = sched_getparam (0, &param);
  if (ret < 0)
    {
      printf("suspend_test: ERROR sched_getparam() failed\n" );
      param.sched_priority = PTHREAD_DEFAULT_PRIORITY;
    }

  victim = task_create("victim", param.sched_priority,
                           STACKSIZE, victim_main, NULL);
  if (victim == ERROR)
    {
      printf("suspend_test: ERROR failed to start victim_main\n" );
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
      printf("suspend_test: ERROR kill() failed\n" );
    }

  printf("suspend_test:  Is the victim still jabbering?\n");
  FFLUSH();
  sleep(10);

  printf("suspend_test: Signaling pid=%d with SIGCONT\n", victim);
  ret = kill(victim, SIGCONT);
  if (ret < 0)
    {
      printf("suspend_test: ERROR kill() failed\n" );
    }

  printf("suspend_test:  The victim should continue the rant.\n");
  FFLUSH();
  sleep(10);

  printf("suspend_test: Signaling pid=%d with SIGKILL\n", victim);
  ret = kill(victim, SIGKILL);
  if (ret < 0)
    {
      printf("suspend_test: ERROR kill() failed\n" );
    }

  FFLUSH();
  sleep(1);
  ret = kill(victim, 0);
  if (ret >= 0)
    {
      printf("suspend_test: ERROR kill() on the dead victim succeeded!\n" );
    }

  printf("suspend_test: done\n" );
  FFLUSH();
}
