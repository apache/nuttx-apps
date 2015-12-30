/****************************************************************************
 * examples/ostest/sigev_thread.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <sched.h>
#include <errno.h>

#include "ostest.h"

/****************************************************************************
 * Private Definitions
 ****************************************************************************/

#ifndef NULL
# define NULL (void*)0
#endif

#define MY_TIMER_SIGNAL 17
#define SIGVALUE_INT  42

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t g_sigev_thread_sem;
static int g_value_received;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_CAN_PASS_STRUCTS
static void sigev_thread_callback(union sigval value)
#else
static void sigev_thread_callback(FAR void *sival_ptr)
#endif
{
#ifdef CONFIG_CAN_PASS_STRUCTS
  int sival_int = value.sival_int;
#else
  int sival_int = (int)((intptr_t)sival_ptr);
#endif

  printf("sigev_thread_callback: Received value %d\n" , sival_int);

  g_value_received = sival_int;
  sem_post(&g_sigev_thread_sem);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void sigev_thread_test(void)
{
  struct sigevent notify;
  struct itimerspec timer;
  timer_t timerid;
  int status;

  printf("sigev_thread_test: Initializing semaphore to 0\n" );
  sem_init(&g_sigev_thread_sem, 0, 0);
  g_value_received = -1;

  /* Create the POSIX timer */

  printf("sigev_thread_test: Creating timer\n" );

  notify.sigev_notify            = SIGEV_THREAD;
  notify.sigev_signo             = MY_TIMER_SIGNAL;
  notify.sigev_value.sival_int   = SIGVALUE_INT;
  notify.sigev_notify_function   = sigev_thread_callback;
  notify.sigev_notify_attributes = NULL;

  status = timer_create(CLOCK_REALTIME, &notify, &timerid);
  if (status != OK)
    {
      printf("sigev_thread_test: timer_create failed, errno=%d\n", errno);
      goto errorout;
    }

  /* Start the POSIX timer */

  printf("sigev_thread_test: Starting timer\n" );

  timer.it_value.tv_sec     = 2;
  timer.it_value.tv_nsec    = 0;
  timer.it_interval.tv_sec  = 2;
  timer.it_interval.tv_nsec = 0;

  status = timer_settime(timerid, 0, &timer, NULL);
  if (status != OK)
    {
      printf("sigev_thread_test: timer_settime failed, errno=%d\n", errno);
      goto errorout;
    }

  /* Take the semaphore */

  printf("sigev_thread_test: Waiting on semaphore\n" );

  do
    {
      status = sem_wait(&g_sigev_thread_sem);
      if (status < 0)
        {
          int error = errno;
          if (error == EINTR)
            {
              printf("sigev_thread_test: sem_wait() interrupted by signal\n" );
            }
          else
            {
              printf("sigev_thread_test: ERROR sem_wait failed, errno=%d\n",
                     error);
              goto errorout;
            }
        }
    }
  while (status < 0);

  printf("sigev_thread_test: Awakened with no error!\n" );

  /* Check sigval */

  printf("sigev_thread_test: g_value_received=%d\n", g_value_received);
  if (g_value_received != SIGVALUE_INT)
    {
      printf("sigev_thread_callback: ERROR sival_int=%d expected %d\n",
             g_value_received, SIGVALUE_INT);
    }

errorout:
  sem_destroy(&g_sigev_thread_sem);

  /* Then delete the timer */

  printf("sigev_thread_test: Deleting timer\n" );
  status = timer_delete(timerid);
  if (status != OK)
    {
      printf("sigev_thread_test: timer_create failed, errno=%d\n", errno);
    }

  printf("sigev_thread_test: Done\n" );
}
