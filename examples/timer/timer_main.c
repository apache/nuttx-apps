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
#include <errno.h>

#include <nuttx/timers/timer.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLE_TIMER_DEVNAME
#  define CONFIG_EXAMPLE_TIMER_DEVNAME "/dev/timer0"
#endif

#ifndef CONFIG_EXAMPLE_TIMER_INTERVAL
#  define CONFIG_EXAMPLE_TIMER_INTERVAL 1000000
#endif

#ifndef CONFIG_EXAMPLE_TIMER_DELAY
#  define CONFIG_EXAMPLE_TIMER_DELAY 100000
#endif

#ifndef CONFIG_EXAMPLE_TIMER_NSAMPLES
#  define CONFIG_EXAMPLE_TIMER_NSAMPLES 20
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * timer_handler
 ****************************************************************************/

static bool timer_handler(FAR uint32_t *next_interval_us)
{
  /* This handler may:
   *
   * (1) Modify the timeout value to change the frequency dynamically, or
   * (2) Return false to stop the timer.
   */

  return true;
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

  printf("  flags: %08lx timeout: %lu timeleft: %lu\n",
         (unsigned long)status.flags, (unsigned long)status.timeout,
         (unsigned long)status.timeleft);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * timer_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int timer_main(int argc, char *argv[])
#endif
{
  struct timer_sethandler_s handler;
  int ret;
  int fd;
  int i;

  /* Open the timer device */

  printf("Open %s\n", CONFIG_EXAMPLE_TIMER_DEVNAME);

  fd = open(CONFIG_EXAMPLE_TIMER_DEVNAME, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_EXAMPLE_TIMER_DEVNAME, errno);
      return EXIT_FAILURE;
    }

  /* Show the timer status before setting the timer interval */

  timer_status(fd);

  /* Set the timer interval */

  printf("Set timer interval to %lu\n",
         (unsigned long)CONFIG_EXAMPLE_TIMER_INTERVAL);

  ret = ioctl(fd, TCIOC_SETTIMEOUT, CONFIG_EXAMPLE_TIMER_INTERVAL);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to set the timer interval: %d\n", errno);
      close(fd);
      return EXIT_FAILURE;
    }

  /* Show the timer status before attaching the timer handler */

  timer_status(fd);

  /* Attach the timer handler
   *
   * NOTE: If no handler is attached, the timer stop at the first interrupt.
   */

  printf("Attach timer handler\n");

  handler.newhandler = timer_handler;
  handler.oldhandler = NULL;

  ret = ioctl(fd, TCIOC_SETHANDLER, (unsigned long)((uintptr_t)&handler));
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

  for (i = 0; i < CONFIG_EXAMPLE_TIMER_NSAMPLES; i++)
    {
      usleep(CONFIG_EXAMPLE_TIMER_DELAY);
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

  /* Show the timer status before starting */

  timer_status(fd);

  /* Close the timer driver */

  printf("Finished\n");
  close(fd);
  return EXIT_SUCCESS;
}
