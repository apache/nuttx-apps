/****************************************************************************
 * examplex/zerocross/zerocross_main.c
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
