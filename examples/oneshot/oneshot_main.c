/****************************************************************************
 * examples/oneshot/oneshot_main.c
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
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include <nuttx/timers/oneshot.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_ONESHOT_DEVNAME
#  define CONFIG_EXAMPLES_ONESHOT_DEVNAME "/dev/oneshot"
#endif

#ifndef CONFIG_EXAMPLES_ONESHOT_DELAY
#  define CONFIG_EXAMPLES_ONESHOT_DELAY 100000
#endif

#ifndef CONFIG_EXAMPLES_ONESHOT_SIGNO
#  define CONFIG_EXAMPLES_ONESHOT_SIGNO 13
#endif

/* For long delays that have to be broken into segments, some loss of
 * precision is expected due to interrupt and context switching overhead.
 * This inaccuracy will cause somewhat longer times than the time requested
 * due to this overhead.  It might be possible to reduce this inaccuracy
 * some by (1) making this task very high priority so that it runs in a more
 * deterministic way, and (2) by subtracting a "fudge factor" to account for
 * time lost due to the overhead.
 */

#define FUDGE_FACTOR 10

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: oneshot_main
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  fprintf(stderr, "USAGE: %s [-d <usecs>] [<devname>]\n", progname);
  fprintf(stderr, "Where:\n");
  fprintf(stderr, "\t-d <usecs>:\n");
  fprintf(stderr, "\tSpecifies the oneshot delay in microseconds.  Default %ld\n",
          (unsigned long)CONFIG_EXAMPLES_ONESHOT_DELAY);
  fprintf(stderr, "\t<devname>:\n");
  fprintf(stderr, "\tSpecifies the path to the oneshot driver.  Default %s\n",
          CONFIG_EXAMPLES_ONESHOT_DEVNAME);
  exit(EXIT_FAILURE);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: oneshot_main
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int oneshot_main(int argc, char *argv[])
#endif
{
  FAR const char *devname = CONFIG_EXAMPLES_ONESHOT_DEVNAME;
  unsigned long usecs = CONFIG_EXAMPLES_ONESHOT_DELAY;
  unsigned long secs;
  struct oneshot_start_s start;
  struct timespec ts;
  uint64_t maxus;
  sigset_t set;
  int ret;
  int fd;

  /* USAGE: nsh> oneshot [-d <usecs>] [<devname>] */

  if (argc > 1)
    {
      if (argc == 2)
        {
          /* FORM: nsh> oneshot [<devname>] */

          devname = argv[1];
        }
      else if (argc < 5)
        {
          /* FORM: nsh> oneshot [-d <usecs>] */

          if (strcmp(argv[1], "-d") != 0)
            {
              fprintf(stderr, "ERROR: Unrecognized option: %s\n", argv[1]);
              show_usage(argv[0]);
            }

          usecs = strtoul(argv[2], NULL, 10);

          if (argc == 4)
            {
              /* FORM: nsh> oneshot [-d <usecs>] [<devname>] */

              devname = argv[3];
            }
        }
      else
        {
          fprintf(stderr, "ERROR: Unsupported number of arguments: %d\n", argc);
          show_usage(argv[0]);
        }
    }

  /* Open the oneshot device */

  printf("Opening %s\n", devname);

  fd = open(devname, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              devname, errno);
      return EXIT_FAILURE;
    }

  /* Get the maximum delay */

  ret = ioctl(fd, OSIOC_MAXDELAY, (unsigned long)((uintptr_t)&ts));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to get the maximum delayl: %d\n",
             errno);
      close(fd);
      return EXIT_FAILURE;
    }

  maxus = (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000;

  printf("Maximum delay is %llu\n", maxus);

  /* Loop waiting until the full delay expires */

  while (usecs > 0)
    {
      /* Start the oneshot timer */

      sigemptyset(&set);
      sigaddset(&set, CONFIG_EXAMPLES_ONESHOT_SIGNO);

      if (usecs < maxus)
        {
          /* Wait for the remaining time */

          printf("Starting oneshot timer with delay %lu microseconds\n",
                 usecs);

          start.pid        = 0;
          start.signo      = CONFIG_EXAMPLES_ONESHOT_SIGNO;
          start.arg        = NULL;

          secs             = usecs / 1000000;
          usecs           -= 1000000 * secs;

          start.ts.tv_sec  = secs;
          start.ts.tv_nsec = usecs * 1000;

          /* Zero usecs to terminate the loop */

          usecs            = 0;
        }
      else
        {
          /* Wait for the maximum */

          printf("Starting oneshot timer with delay %llu microseconds\n",
                 maxus);

          start.ts.tv_sec  = ts.tv_sec;
          start.ts.tv_nsec = ts.tv_nsec;

          usecs           -= maxus;

#if FUDGE_FACTOR > 0
          if (usecs > FUDGE_FACTOR)
            {
              usecs      -= FUDGE_FACTOR;
            }
          else
            {
              usecs       = 0;
            }
#endif
        }

      ret = ioctl(fd, OSIOC_START, (unsigned long)((uintptr_t)&start));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: Failed to start the oneshot interval: %d\n",
                 errno);
          close(fd);
          return EXIT_FAILURE;
        }

      /* Wait for the oneshot to fire */

      printf("Waiting...\n");
      ret = sigwaitinfo(&set, NULL);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: sigwaitinfo failed: %d\n",
                  errno);
          close(fd);
          return EXIT_FAILURE;
        }
    }

  /* Close the oneshot driver */

  printf("Finished\n");
  close(fd);
  return EXIT_SUCCESS;
}
