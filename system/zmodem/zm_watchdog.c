/****************************************************************************
 * system/zmodem/zm_watchdog.c
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
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
  FAR struct zm_state_s *pzm = (FAR struct zm_state_s *)info->si_value.sival_ptr;

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
