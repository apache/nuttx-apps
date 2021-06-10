/****************************************************************************
 * apps/testing/ostest/sigprocmask.c
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

static int g_some_signals[NSIGNALS] =
{
  1,
  3,
  5,
  7,
  9
};

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

  printf("sigprocmask_test: SUCCESS\n");
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
  printf("sigprocmask_test: Aborting\n");
  FFLUSH();
}
