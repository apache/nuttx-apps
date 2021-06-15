/****************************************************************************
 * apps/examples/oneshot/oneshot_main.c
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
  fprintf(stderr, "\tSpecifies the oneshot delay in microseconds."
          " Default %ld\n", (unsigned long)CONFIG_EXAMPLES_ONESHOT_DELAY);
  fprintf(stderr, "\t<devname>:\n");
  fprintf(stderr, "\tSpecifies the path to the oneshot driver. Default %s\n",
          CONFIG_EXAMPLES_ONESHOT_DEVNAME);
  exit(EXIT_FAILURE);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: oneshot_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
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
          fprintf(stderr, "ERROR: Unsupported number of arguments: %d\n",
                  argc);
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

  maxus = (uint64_t)ts.tv_sec * USEC_PER_SEC +
          (uint64_t)ts.tv_nsec / NSEC_PER_USEC;

  printf("Maximum delay is %" PRIu64 "\n", maxus);

  /* Ignore the default signal action */

  signal(CONFIG_EXAMPLES_ONESHOT_SIGNO, SIG_IGN);

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

          start.pid  = 0;
          secs       = usecs / USEC_PER_SEC;
          usecs     -= USEC_PER_SEC * secs;

          start.ts.tv_sec  = secs;
          start.ts.tv_nsec = usecs * NSEC_PER_USEC;

          start.event.sigev_notify = SIGEV_SIGNAL;
          start.event.sigev_signo  = CONFIG_EXAMPLES_ONESHOT_SIGNO;
          start.event.sigev_value.sival_ptr = NULL;

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

          usecs -= maxus;

#if FUDGE_FACTOR > 0
          if (usecs > FUDGE_FACTOR)
            {
              usecs -= FUDGE_FACTOR;
            }
          else
            {
              usecs = 0;
            }
#endif
        }

      ret = ioctl(fd, OSIOC_START, (unsigned long)((uintptr_t)&start));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: Failed to start the oneshot: %d\n",
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
