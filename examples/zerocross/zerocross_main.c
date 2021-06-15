/****************************************************************************
 * examplex/zerocross/zerocross_main.c
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
#include <errno.h>
#include <debug.h>

#include <nuttx/sensors/zerocross.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_SENSORS_ZEROCROSS
#  error "CONFIG_SENSORS_ZEROCROSS is not defined in the configuration"
#endif

#ifndef CONFIG_EXAMPLES_ZEROCROSS_DEVNAME
#  define CONFIG_EXAMPLES_ZEROCROSS_DEVNAME "/dev/zc0"
#endif

#ifndef CONFIG_EXAMPLES_ZEROCROSS_SIGNO
#  define CONFIG_EXAMPLES_ZEROCROSS_SIGNO 13
#endif

/* Helpers ******************************************************************/

#ifndef MIN
#  define MIN(a,b) (a < b ? a : b)
#endif
#ifndef MAX
#  define MAX(a,b) (a > b ? a : b)
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * zerocross_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct sigevent event;
  int fd;
  int tmp;
  int ret;
  int errcode = EXIT_FAILURE;

  /* Open the zerocross device */

  fd = open(CONFIG_EXAMPLES_ZEROCROSS_DEVNAME, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_EXAMPLES_ZEROCROSS_DEVNAME, errno);
      return EXIT_FAILURE;
    }

  /* Register to receive a signal on every zero cross event */

  event.sigev_notify = SIGEV_SIGNAL;
  event.sigev_signo  = CONFIG_EXAMPLES_ZEROCROSS_SIGNO;

  ret = ioctl(fd, ZCIOC_REGISTER, (unsigned long)((uintptr_t)&event));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(ZCIOC_REGISTER) failed: %d\n", errno);
      goto errout_with_fd;
    }

  /* Ignore the default signal action */

  signal(CONFIG_EXAMPLES_ZEROCROSS_SIGNO, SIG_IGN);

  /* Then loop, receiving signals indicating zero cross events. */

  for (; ; )
    {
      struct siginfo value;
      sigset_t set;
      ssize_t nread;

      /* Wait for a signal */

      sigemptyset(&set);
      sigaddset(&set, CONFIG_EXAMPLES_ZEROCROSS_SIGNO);
      ret = sigwaitinfo(&set, &value);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: sigwaitinfo() failed: %d\n", errno);
          goto errout_with_fd;
        }

      /* Show the value accompanying the signal */

      printf("Signal received!\n");
      printf("Sample = %d\n", value.si_value.sival_int);
    }

  errcode = EXIT_SUCCESS;

errout_with_fd:
  close(fd);
  return errcode;
}
