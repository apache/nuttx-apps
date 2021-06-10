/****************************************************************************
 * apps/system/zmodem/zm_watchdog.c
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
#include <signal.h>
#include <time.h>
#include <errno.h>

#include "zm.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: zm_expiry
 *
 * Description:
 *   SIGALRM signal handler.  Simply posts the timeout event.
 *
 ****************************************************************************/

static void zm_expiry(int signo, FAR siginfo_t *info, FAR void *context)
{
  FAR struct zm_state_s *pzm =
            (FAR struct zm_state_s *)info->si_value.sival_ptr;

  /* Just set the timeout flag.  If the Zmodem logic was truly waiting, then
   * the signal should wake it up and it should then process the timeout
   * condition.
   *
   * REVISIT:  This is a read-modify-write operation and has the potential
   * for atomicity issue.  We might need to use a dedicated boolean value
   * to indicate to timeout!
   */

  pzm->flags |= ZM_FLAG_TIMEOUT;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  zm_timerinit
 *
 * Description:
 *   Create the POSIX timer used to manage timeouts and attach the SIGALRM
 *   signal handler to catch the timeout events.
 *
 ****************************************************************************/

int zm_timerinit(FAR struct zm_state_s *pzm)
{
  struct sigevent toevent;
  struct sigaction act;
  int ret;

  /* Create a POSIX timer to handle timeouts */

  toevent.sigev_notify          = SIGEV_SIGNAL;
  toevent.sigev_signo           = SIGALRM;
  toevent.sigev_value.sival_ptr = pzm;

  ret = timer_create(CLOCK_REALTIME, &toevent, &pzm->timer);
  if (ret < 0)
    {
      int errorcode = errno;
      zmdbg("ERROR: Failed to create a timer: %d\n", errorcode);
      return -errorcode;
    }

  /* Attach a signal handler to catch the timeout */

  act.sa_sigaction = zm_expiry;
  act.sa_flags     = SA_SIGINFO;
  sigemptyset(&act.sa_mask);

  ret = sigaction(SIGALRM, &act, NULL);
  if (ret < 0)
    {
      int errorcode = errno;
      zmdbg("ERROR: Failed to attach a signal handler: %d\n", errorcode);
      return -errorcode;
    }

  return OK;
}

/****************************************************************************
 * Name:  zm_timerstart
 *
 * Description:
 *   Start, restart, or stop the timer.
 *
 ****************************************************************************/

int zm_timerstart(FAR struct zm_state_s *pzm, unsigned int sec)
{
  struct itimerspec todelay;
  int ret;

  /* Start, restart, or stop the timer */

  todelay.it_interval.tv_sec    = 0;   /* Nonrepeating */
  todelay.it_interval.tv_nsec   = 0;
  todelay.it_value.tv_sec       = sec;
  todelay.it_value.tv_nsec      = 0;

  ret = timer_settime(pzm->timer, 0, &todelay, NULL);
  if (ret < 0)
    {
      int errorcode = errno;
      zmdbg("ERROR: Failed to set the timer: %d\n", errorcode);
      return -errorcode;
    }

  return OK;
}

/****************************************************************************
 * Name:  zm_timerrelease
 *
 * Description:
 *   Destroy the timer and and detach the signal handler.
 *
 ****************************************************************************/

int zm_timerrelease(FAR struct zm_state_s *pzm)
{
  struct sigaction act;
  int result;
  int ret = OK;

  /* Delete the POSIX timer */

  result = timer_delete(pzm->timer);
  if (result < 0)
    {
      int errorcode = errno;
      zmdbg("ERROR: Failed to delete the timer: %d\n", errorcode);
      ret = -errorcode;
    }

  /* Revert to the default signal behavior */

  act.sa_handler = SIG_DFL;
  act.sa_flags   = 0;
  sigemptyset(&act.sa_mask);

  result = sigaction(SIGALRM, &act, NULL);
  if (result < 0)
    {
      int errorcode = errno;
      zmdbg("ERROR: Failed to detach the signal handler: %d\n", errorcode);
      ret = -errorcode;
    }

  return ret;
}
