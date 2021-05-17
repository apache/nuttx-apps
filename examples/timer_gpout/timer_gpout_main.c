/****************************************************************************
 * apps/examples/timer_gpout/timer_gpout_main.c
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

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <nuttx/timers/timer.h>
#include <nuttx/ioexpander/gpio.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEVNAME_SIZE 16

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_devtim[DEVNAME_SIZE];
static char g_devgpout[DEVNAME_SIZE];
static bool g_timer_gpout_daemon_started = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: timer_status
 *
 * Description:
 *   Auxiliar function to get the timer status and print it.
 *
 * Parameters:
 *   fd           - Timer file descriptor to access the timer.
 *
 ****************************************************************************/

static void timer_status(int fd)
{
  struct timer_status_s status;
  int ret;

  /* Get timer status */

  ret = ioctl(fd, TCIOC_GETSTATUS, (unsigned long)((uintptr_t)&status));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to get timer status: %d\n", errno);
      close(fd);
      exit(EXIT_FAILURE);
    }

  /* Print the timer status */

  printf("  flags: %08lx timeout: %lu timeleft: %lu\n",
         (unsigned long)status.flags, (unsigned long)status.timeout,
         (unsigned long)status.timeleft);
}

/****************************************************************************
 * Name: timer_gpout_daemon
 *
 * Description:
 *   Deamon that will be active waiting on a signal to change the digital
 *   output state.
 *
 ****************************************************************************/

static int timer_gpout_daemon(int argc, char *argv[])
{
  struct timer_notify_s notify;
  sigset_t set;
  struct siginfo value;
  int fd_timer;
  int fd_gpout;
  int ret;
  bool state = false;

  /* Indicate that the deamon is running */

  g_timer_gpout_daemon_started = true;
  printf("timer_gpout_daemon: timer_gpout_daemon started\n");

  /* Open the timer device */

  printf("Open %s\n", g_devtim);

  fd_timer = open(g_devtim, O_RDONLY);
  if (fd_timer < 0)
    {
      int errcode = errno;
      printf("timer_gpout_daemon: Failed to open %s: %d\n",
              g_devtim, errcode);
      return EXIT_FAILURE;
    }

  /* Open the GPIO driver */

  printf("Open %s\n", g_devgpout);

  fd_gpout = open(g_devgpout, O_RDWR);
  if (fd_gpout < 0)
    {
      int errcode = errno;
      printf("timer_gpout_daemon: Failed to open %s: %d\n",
              g_devgpout, errcode);
      close(fd_timer);
      return EXIT_FAILURE;
    }

  /* Show the timer status before setting the timer interval */

  timer_status(fd_timer);

  /* Set the timer interval */

  printf("Set timer interval to %lu\n",
         (unsigned long)CONFIG_EXAMPLES_TIMER_GPOUT_INTERVAL);

  ret = ioctl(fd_timer, TCIOC_SETTIMEOUT,
              CONFIG_EXAMPLES_TIMER_GPOUT_INTERVAL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("timer_gpout_daemon: Failed to set the timer interval: %d\n",
             errcode);
      goto errout;
    }

  /* Show the timer status before attaching the timer handler */

  timer_status(fd_timer);

  /* Configure the signal set for this task */

  sigemptyset(&set);
  sigaddset(&set, CONFIG_EXAMPLES_TIMER_GPOUT_SIGNO);

  /* Configure the timer notifier to receive a signal when timeout occurs.
   * Inform the PID of the process that will be notified by the internal
   * timer callback, and configure the event to describe the way this
   * task will be notified. In this case, through a signal.
   */

  printf("Configure the timer notification.\n");

  notify.pid   = getpid();
  notify.event.sigev_notify = SIGEV_SIGNAL;
  notify.event.sigev_signo  = CONFIG_EXAMPLES_TIMER_GPOUT_SIGNO;
  notify.event.sigev_value.sival_ptr = NULL;

  ret = ioctl(fd_timer, TCIOC_NOTIFICATION,
              (unsigned long)((uintptr_t)&notify));
  if (ret < 0)
    {
      int errcode = errno;
      printf("timer_gpout_daemon: Failed to set the timer handler: %d\n",
             errcode);
      goto errout;
    }

  /* Start the timer */

  printf("Start the timer\n");

  ret = ioctl(fd_timer, TCIOC_START, 0);
  if (ret < 0)
    {
      int errcode = errno;
      printf("timer_gpout_daemon: Failed to start the timer: %d\n", errcode);
      goto errout;
    }

  while (1)
    {
      ret = sigwaitinfo(&set, &value);
      if (ret < 0)
        {
          int errcode = errno;
          printf("timer_gpout_daemon: ERROR: sigwaitinfo() failed: %d\n",
                 errcode);
          goto errout;
        }

      /* Change the gpout state */

      state = !state;

      /* Write the pin value */

      ret = ioctl(fd_gpout, GPIOC_WRITE, (unsigned long)state);
      if (ret < 0)
        {
          int errcode = errno;
          printf("timer_gpout_daemon: Failed to write value"
                 " %u from %s: %d\n",
                (unsigned int)state, g_devgpout, errcode);
          goto errout;
        }
    }

errout:
  close(fd_timer);
  close(fd_gpout);
  g_timer_gpout_daemon_started = false;

  printf("timer_gpout_daemon: Terminating!\n");
  return EXIT_FAILURE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * timer_gpout main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  int opt;

  if (g_timer_gpout_daemon_started)
    {
      printf("timer_gpout_main: timer_gpout daemon already running\n");
      return EXIT_SUCCESS;
    }

  /* Retrieve the arguments */

  /* Use the ones configured on menuconfig */

  strcpy(g_devtim, CONFIG_EXAMPLES_TIMER_GPOUT_TIM_DEVNAME);
  strcpy(g_devgpout, CONFIG_EXAMPLES_TIMER_GPOUT_GPOUT_DEVNAME);

  /* Or the ones passed as arguments */

  while ((opt = getopt(argc, argv, ":t:g:")) != -1)
    {
      switch (opt)
      {
        case 't':
            strcpy(g_devtim, optarg);
            break;
        case 'g':
            strcpy(g_devgpout, optarg);
            break;
        case ':':
            fprintf(stderr, "ERROR: Option needs a value\n");
            exit(EXIT_FAILURE);
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-d /dev/timerx] [-d /dev/gpoutx]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
      }
    }

  printf("timer_gpout_main: Starting the timer_gpout daemon\n");
  ret = task_create("timer_gpout_daemon",
                    CONFIG_EXAMPLES_TIMER_GPOUT_PRIORITY,
                    CONFIG_EXAMPLES_TIMER_GPOUT_STACKSIZE,
                    timer_gpout_daemon,
                    NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("timer_gpout_main: Failed to start timer_gpout_daemon: %d\n",
             errcode);
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
