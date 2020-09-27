/****************************************************************************
 * examples/timer/timer_main.c
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

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <nuttx/timers/timer.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEVNAME_SIZE 16

#ifndef CONFIG_EXAMPLES_TIMER_DEVNAME
#  define CONFIG_EXAMPLES_TIMER_DEVNAME "/dev/timer0"
#endif

#ifndef CONFIG_EXAMPLES_TIMER_INTERVAL
#  define CONFIG_EXAMPLES_TIMER_INTERVAL 1000000
#endif

#ifndef CONFIG_EXAMPLES_TIMER_DELAY
#  define CONFIG_EXAMPLES_TIMER_DELAY 100000
#endif

#ifndef CONFIG_EXAMPLES_TIMER_NSAMPLES
#  define CONFIG_EXAMPLES_TIMER_NSAMPLES 20
#endif

#ifndef CONFIG_EXAMPLES_TIMER_SIGNO
#  define CONFIG_EXAMPLES_TIMER_SIGNO 17
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile unsigned long g_nsignals;

/****************************************************************************
 * timer_sighandler
 ****************************************************************************/

static void timer_sighandler(int signo, FAR siginfo_t *siginfo,
                             FAR void *context)
{
  /* Does nothing in this example except for increment a count of signals
   * received.
   *
   * NOTE: The use of signal handler is not recommended if you are concerned
   * about the signal latency.  Instead, a dedicated, high-priority thread
   * that waits on sigwaitinfo() is recommended.  High priority is required
   * if you want a deterministic wake-up time when the signal occurs.
   */

  g_nsignals++;
}

/****************************************************************************
 * timer_status
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

  printf("  flags: %08lx timeout: %lu timeleft: %lu nsignals: %lu\n",
         (unsigned long)status.flags, (unsigned long)status.timeout,
         (unsigned long)status.timeleft, g_nsignals);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * timer_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct timer_notify_s notify;
  struct sigaction act;
  int ret;
  int fd;
  int i;
  int opt;
  char devname[DEVNAME_SIZE];
  strcpy(devname, CONFIG_EXAMPLES_TIMER_DEVNAME);

  while ((opt = getopt(argc, argv, ":d:")) != -1)
    {
      switch (opt)
      {
        case 'd':
            strcpy(devname, optarg);
            break;
        case ':':
            fprintf(stderr, "ERROR: Option needs a value\n");
            exit(EXIT_FAILURE);
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-d /dev/timerx]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
      }
    }

  /* Open the timer device */

  printf("Open %s\n", devname);

  fd = open(devname, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              devname, errno);
      return EXIT_FAILURE;
    }

  /* Show the timer status before setting the timer interval */

  timer_status(fd);

  /* Set the timer interval */

  printf("Set timer interval to %lu\n",
         (unsigned long)CONFIG_EXAMPLES_TIMER_INTERVAL);

  ret = ioctl(fd, TCIOC_SETTIMEOUT, CONFIG_EXAMPLES_TIMER_INTERVAL);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to set the timer interval: %d\n",
              errno);
      close(fd);
      return EXIT_FAILURE;
    }

  /* Show the timer status before attaching the timer handler */

  timer_status(fd);

  /* Attach a signal handler to catch the notifications.  NOTE that using
   * signal handler is very slow.  A much more efficient thing to do is to
   * create a separate pthread that waits on sigwaitinfo() for timer events.
   * Much less overhead in that case.
   */

  g_nsignals       = 0;

  act.sa_sigaction = timer_sighandler;
  act.sa_flags     = SA_SIGINFO;

  sigfillset(&act.sa_mask);
  sigdelset(&act.sa_mask, CONFIG_EXAMPLES_TIMER_SIGNO);

  ret = sigaction(CONFIG_EXAMPLES_TIMER_SIGNO, &act, NULL);
  if (ret != OK)
    {
      fprintf(stderr, "ERROR: Fsigaction failed: %d\n", errno);
      close(fd);
      return EXIT_FAILURE;
    }

  /* Register a callback for notifications using the configured signal.
   *
   * NOTE: If no callback is attached, the timer stop at the first interrupt.
   */

  printf("Attach timer handler\n");

  notify.pid   = getpid();

  notify.event.sigev_notify = SIGEV_SIGNAL;
  notify.event.sigev_signo  = CONFIG_EXAMPLES_TIMER_SIGNO;
  notify.event.sigev_value.sival_ptr = NULL;

  ret = ioctl(fd, TCIOC_NOTIFICATION, (unsigned long)((uintptr_t)&notify));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to set the timer handler: %d\n", errno);
      close(fd);
      return EXIT_FAILURE;
    }

  /* Show the timer status before starting */

  timer_status(fd);

  /* Start the timer */

  printf("Start the timer\n");

  ret = ioctl(fd, TCIOC_START, 0);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to start the timer: %d\n", errno);
      close(fd);
      return EXIT_FAILURE;
    }

  /* Wait a bit showing timer status */

  for (i = 0; i < CONFIG_EXAMPLES_TIMER_NSAMPLES; i++)
    {
      usleep(CONFIG_EXAMPLES_TIMER_DELAY);
      timer_status(fd);
    }

  /* Stop the timer */

  printf("Stop the timer\n");

  ret = ioctl(fd, TCIOC_STOP, 0);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to stop the timer: %d\n", errno);
      close(fd);
      return EXIT_FAILURE;
    }

  /* Detach the signal handler */

  act.sa_handler = SIG_DFL;
  sigaction(CONFIG_EXAMPLES_TIMER_SIGNO, &act, NULL);

  /* Show the timer status before starting */

  timer_status(fd);

  /* Close the timer driver */

  printf("Finished\n");
  close(fd);
  return EXIT_SUCCESS;
}
