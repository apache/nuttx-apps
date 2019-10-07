/****************************************************************************
 * examples/touchscreen/tc_main.c
 *
 *   Copyright (C) 2011, 2014-2025 Gregory Nutt. All rights reserved.
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

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#ifdef CONFIG_EXAMPLES_TOUCHSCREEN_MOUSE
#  include <nuttx/input/mouse.h>
#endif

#include <nuttx/input/touchscreen.h>

#include "tc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tc_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_EXAMPLES_TOUCHSCREEN_MOUSE
  struct mouse_report_s sample;
#else
  struct touch_sample_s sample;
#endif
  ssize_t nbytes;
  long nsamples;
  int fd;
  int errval = 0;

  /* If this example is configured as an NX add-on, then limit the number of
   * samples that we collect before returning.  Otherwise, we never return
   */

  nsamples = CONFIG_EXAMPLES_TOUCHSCREEN_NSAMPLES;
  if (argc > 1)
    {
      nsamples = strtol(argv[1], NULL, 10);
    }
  printf("tc_main: nsamples: %d\n", nsamples);

  /* Open the touchscreen device for reading */

  printf("tc_main: Opening %s\n", CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH);
  fd = open(CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH, O_RDONLY);
  if (fd < 0)
    {
      printf("tc_main: open %s failed: %d\n",
              CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH, errno);
      errval = 2;
      goto errout;
    }

  /* Now loop the appropriate number of times, displaying the collected
   * touchscreen samples.
   */

  for (;;)
  {
    /* Flush any output before the loop entered or from the previous pass
     * through the loop.
     */

    fflush(stdout);

#ifdef CONFIG_EXAMPLES_TOUCHSCREEN_MOUSE
    /* Read one sample */

    iinfo("Reading...\n");
    nbytes = read(fd, &sample, sizeof(struct mouse_report_s));
    iinfo("Bytes read: %d\n", nbytes);

    /* Handle unexpected return values */

    if (nbytes < 0)
      {
        errval = errno;
        if (errval != EINTR)
          {
            printf("tc_main: read %s failed: %d\n",
                   CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH, errval);
            errval = 3;
            goto errout_with_dev;
          }

        printf("tc_main: Interrupted read...\n");
      }
    else if (nbytes != sizeof(struct mouse_report_s))
      {
        printf("tc_main: Unexpected read size=%d, expected=%d, Ignoring\n",
               nbytes, sizeof(struct mouse_report_s));
      }

    /* Print the sample data on successful return */

    else
      {
        printf("Sample     :\n");
        printf("   buttons : %02x\n", sample.buttons);
        printf("         x : %d\n",   sample.x);
        printf("         y : %d\n",   sample.y);
#ifdef CONFIG_MOUSE_WHEEL
        printf("     wheel : %d\n",   sample.wheel);
#endif
      }
#else
    /* Read one sample */

    iinfo("Reading...\n");
    nbytes = read(fd, &sample, sizeof(struct touch_sample_s));
    iinfo("Bytes read: %d\n", nbytes);

    /* Handle unexpected return values */

    if (nbytes < 0)
      {
        errval = errno;
        if (errval != EINTR)
          {
            printf("tc_main: read %s failed: %d\n",
                   CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH, errval);
            errval = 3;
            goto errout_with_dev;
          }

        printf("tc_main: Interrupted read...\n");
      }
    else if (nbytes != sizeof(struct touch_sample_s))
      {
        printf("tc_main: Unexpected read size=%ld, expected=%d, Ignoring\n",
               (long)nbytes, sizeof(struct touch_sample_s));
      }

    /* Print the sample data on successful return */

    else
      {
        printf("Sample     :\n");
        printf("   npoints : %d\n",   sample.npoints);
        printf("Point 1    :\n");
        printf("        id : %d\n",   sample.point[0].id);
        printf("     flags : %02x\n", sample.point[0].flags);
        printf("         x : %d\n",   sample.point[0].x);
        printf("         y : %d\n",   sample.point[0].y);
        printf("         h : %d\n",   sample.point[0].h);
        printf("         w : %d\n",   sample.point[0].w);
        printf("  pressure : %d\n",   sample.point[0].pressure);
      }
#endif

    if (nsamples && --nsamples <= 0)
      {
        break;
      }
  }

errout_with_dev:
  close(fd);

errout:
  printf("Terminating!\n");
  fflush(stdout);
  return errval;
}
