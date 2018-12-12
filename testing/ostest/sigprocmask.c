/****************************************************************************
 * apps/testing/ostest/sigprocmask.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
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
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NSIGNALS 5

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_some_signals[NSIGNALS] = {1, 3, 5, 7, 9};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void sigprocmask_test(void)
{
  sigset_t saved;
  sigset_t currmask;
  sigset_t newmask;
  int ret;
  int i;

  /* Save the current sigprocmask */

  ret = sigprocmask(SIG_SETMASK, NULL, &saved);
  if (ret != OK)
    {
      int errcode = errno;
      printf("sigprocmask_test: ERROR sigprocmask failed: %d\n", errcode);
      goto errout;
    }

  /* Set the mask to all zeroes */

  ret = sigemptyset(&newmask);
  if (ret != OK)
    {
      int errcode = errno;
      printf("sigprocmask_test: ERROR sigemptyset failed: %d\n", errcode);
      goto errout;
    }

  ret = sigprocmask(SIG_SETMASK, &newmask, NULL);
  if (ret != OK)
    {
      int errcode = errno;
      printf("sigprocmask_test: ERROR sigprocmask failed: %d\n", errcode);
      goto errout_with_mask;
    }

  /* Set a few signals */

  for (i = 0; i < NSIGNALS; i++)
    {
      int signo = g_some_signals[i];

      ret = sigaddset(&newmask, signo);
      if (ret != OK)
        {
          int errcode = errno;
          printf("sigprocmask_test: ERROR sigaddset failed: %d\n", errcode);
          goto errout_with_mask;
        }

      ret = sighold(signo);
      if (ret != OK)
        {
          int errcode = errno;
          printf("sigprocmask_test: ERROR sighold failed: %d\n", errcode);
          goto errout_with_mask;
        }
    }

  /* Now get the modified mask */

  ret = sigprocmask(SIG_SETMASK, NULL, &currmask);
  if (ret != OK)
    {
      int errcode = errno;
      printf("sigprocmask_test: ERROR sigprocmask failed: %d\n", errcode);
      goto errout_with_mask;
    }

  /* It should be the same as newmask */

  if (memcmp(&currmask, &newmask, sizeof(sigset_t)) != 0)
    {
      printf("sigprocmask_test: ERROR unexpected sigprocmask\n");
      goto errout_with_mask;
    }

  /* Set the mask to all ones */

  ret = sigfillset(&newmask);
  if (ret != OK)
    {
      int errcode = errno;
      printf("sigprocmask_test: ERROR sigfillset failed: %d\n", errcode);
      goto errout;
    }

  ret = sigprocmask(SIG_SETMASK, &newmask, NULL);
  if (ret != OK)
    {
      int errcode = errno;
      printf("sigprocmask_test: ERROR sigprocmask failed: %d\n", errcode);
      goto errout_with_mask;
    }

  /* Release a few signals */

  for (i = 0; i < NSIGNALS; i++)
    {
      int signo = g_some_signals[i];

      ret = sigdelset(&newmask, signo);
      if (ret != OK)
        {
          int errcode = errno;
          printf("sigprocmask_test: ERROR sigdelset failed: %d\n", errcode);
          goto errout_with_mask;
        }

      ret = sigrelse(signo);
      if (ret != OK)
        {
          int errcode = errno;
          printf("sigprocmask_test: ERROR sigrelse failed: %d\n", errcode);
          goto errout_with_mask;
        }
    }

  /* Now get the modified mask */

  ret = sigprocmask(SIG_SETMASK, NULL, &currmask);
  if (ret != OK)
    {
      int errcode = errno;
      printf("sigprocmask_test: ERROR sigprocmask failed: %d\n", errcode);
      goto errout_with_mask;
    }

  /* It should be the same as newmask */

  if (memcmp(&currmask, &newmask, sizeof(sigset_t)) != 0)
    {
      printf("sigprocmask_test: ERROR unexpected sigprocmask\n");
      goto errout_with_mask;
    }

  ret = sigprocmask(SIG_SETMASK, &saved, NULL);
  if (ret != OK)
    {
      int errcode = errno;
      printf("sigprocmask_test: ERROR sigprocmask failed: %d\n", errcode);
      goto errout;
    }

  printf("sigprocmask_test: SUCCESS\n" );
  FFLUSH();
  return;

errout_with_mask:
  ret = sigprocmask(SIG_SETMASK, &saved, NULL);
  if (ret != OK)
    {
      int errcode = errno;
      printf("sigprocmask_test: ERROR sigprocmask failed: %d\n", errcode);
      goto errout;
    }

errout:
  printf("sigprocmask_test: Aborting\n" );
  FFLUSH();
}
