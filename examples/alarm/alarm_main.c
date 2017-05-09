/****************************************************************************
 * examples/alarm/alarm_main.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
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
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>

#include <nuttx/timers/rtc.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_alarm_daemon_started;
static pid_t g_alarm_daeon_pid;
static bool g_alarm_received[CONFIG_RTC_NALARMS];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: alarm_handler
 ****************************************************************************/

static void alarm_handler(int signo, FAR siginfo_t *info, FAR void *ucontext)
{
  int almndx = info->si_value.sival_int;
  if (almndx >= 0 && almndx < CONFIG_RTC_NALARMS)
    {
      g_alarm_received[almndx] = true;
    }
}

/****************************************************************************
 * Name: alarm_daemon
 ****************************************************************************/

static int alarm_daemon(int argc, FAR char *argv[])
{
  struct sigaction act;
  sigset_t set;
  int ret;
  int i;

  /* Indicate that we are running */

  g_alarm_daemon_started = true;
  printf("alarm_daemon: Running\n");

  /* Make sure that the alarm signal is unmasked */

  (void)sigemptyset(&set);
  (void)sigaddset(&set, CONFIG_EXAMPLES_ALARM_SIGNO);
  ret = sigprocmask(SIG_UNBLOCK, &set, NULL);
  if (ret != OK)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: sigprocmask failed: %d\n",
              errcode);
      goto errout;
    }

  /* Register alarm signal handler */

  act.sa_sigaction = alarm_handler;
  act.sa_flags     = SA_SIGINFO;

  (void)sigfillset(&act.sa_mask);
  (void)sigdelset(&act.sa_mask, CONFIG_EXAMPLES_ALARM_SIGNO);

  ret = sigaction(CONFIG_EXAMPLES_ALARM_SIGNO, &act, NULL);
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: sigaction failed: %d\n",
              errcode);
      goto errout;
    }

  /* Now loop forever, waiting for alarm signals */

  for (; ; )
    {
      /* Check if any alarms fired.
       *
       * NOTE that there are race conditions here... if we missing an alarm,
       * we will just report it a half second late.
       */

      for (i = 0; i < CONFIG_RTC_NALARMS; i++)
        {
          if (g_alarm_received[i])
            {
              printf("alarm_demon: alarm %d received\n", i)  ;
              g_alarm_received[i] = false;
            }
        }

      /* Now wait a little while and poll again.  If a signal is received
       * this should cuase us to awken earlier.
       */

      usleep(500*1000L);
    }

errout:
  g_alarm_daemon_started = false;

  printf("alarm_daemon: Terminating\n");
  return EXIT_FAILURE;
}

/****************************************************************************
 * Name: start_daemon
 ****************************************************************************/

static int start_daemon(void)
{
  if (!g_alarm_daemon_started)
    {
      g_alarm_daeon_pid =
        task_create("alarm_daemon", CONFIG_EXAMPLES_ALARM_PRIORITY,
                    CONFIG_EXAMPLES_ALARM_STACKSIZE, alarm_daemon,
                    NULL);
      if (g_alarm_daeon_pid < 0)
        {
          int errcode = errno;
          fprintf(stderr, "ERROR: Failed to start alarm_daemon: %d\n",
                 errcode);
          return -errcode;
        }

      printf("alarm_daemon started\n");
      usleep(500*1000L);
    }

  return OK;
}

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  fprintf(stderr, "USAGE:\n");
#if CONFIG_RTC_NALARMS > 1
  fprintf(stderr, "\t%s [-a <alarmid>] <seconds>\n", progname);
  fprintf(stderr, "Where:\n");
  fprintf(stderr, "\t-a <alarmid>\n");
  fprintf(stderr, "\t\t<alarmid> selects the alarm: 0..%d\n",
          CONFIG_RTC_NALARMS - 1);
#else
  fprintf(stderr, "\t%s <seconds>\n", progname);
  fprintf(stderr, "Where:\n");
#endif
  fprintf(stderr, "\t<seconds>\n");
  fprintf(stderr, "\t\tThe number of seconds until the alarm expires.\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * alarm_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int alarm_main(int argc, FAR char *argv[])
#endif
{
  struct rtc_setrelative_s setrel;
  unsigned long seconds;
  int alarmid = 0;
  int secndx;
  int fd;
  int ret;

  /* Make sure that the alarm daemon is running */

  ret = start_daemon();
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Unrecognized option: %s\n", argv[1]);
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

#if CONFIG_RTC_NALARMS > 1
  /* Check for an optional alarm ID */

  if (argc == 4)
    {
      if (strcmp(argv[1], "-a") != 0)
        {
          fprintf(stderr, "ERROR: Unrecognized option: %s\n", argv[1]);
          show_usage(argv[0]);
          return EXIT_FAILURE;
        }

      alarmid = atoi(argv[2]);
      secndx = 3;
    }
  else
#endif
  if (argc == 2)
    {
       secndx = 1;
    }
  else
    {
      fprintf(stderr, "ERROR: Invalid number of arguments: %d\n", argc - 1);
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

  /* Get the number of seconds until the alarm expiration */

  seconds = strtoul(argv[secndx], NULL, 10);
  if (seconds < 1)
    {
      fprintf(stderr, "ERROR: Invalid number of seconds: %lu\n", seconds);
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

  /* Open the RTC driver */

  printf("Opening %s\n", CONFIG_EXAMPLES_ALARM_DEVPATH);

  fd = open(CONFIG_EXAMPLES_ALARM_DEVPATH, O_WRONLY);
  if (fd < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_EXAMPLES_ALARM_DEVPATH, errcode);
      return EXIT_FAILURE;
    }

  /* Set the alarm */

  setrel.id      = alarmid;
  setrel.signo   = CONFIG_EXAMPLES_ALARM_SIGNO;
  setrel.pid     = g_alarm_daeon_pid;
  setrel.reltime = (time_t)seconds;

  setrel.sigvalue.sival_int = alarmid;

  ret = ioctl(fd, RTC_SET_RELATIVE, (unsigned long)((uintptr_t)&setrel));
  if (ret < 0)
    {
      int errcode = errno;

      fprintf(stderr, "ERROR: RTC_SET_RELATIVE ioctl failed: %d\n",
              errcode);

      (void)close(fd);
      return EXIT_FAILURE;
    }

  printf("Alarm %d set in %lu seconds\n", alarmid, seconds);

  (void)close(fd);
  return EXIT_SUCCESS;
}
