/****************************************************************************
 * apps/examples/touchscreen/tc_main.c
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

  printf("tc_main: nsamples: %ld\n", nsamples);

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

  for (; ; )
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
#ifdef CONFIG_INPUT_MOUSE_WHEEL
          printf("     wheel : %d\n",   sample.wheel);
#endif
        }
#else
      /* Read one sample */

      iinfo("Reading...\n");
      nbytes = read(fd, &sample, sizeof(struct touch_sample_s));
      iinfo("Bytes read: %zd\n", nbytes);

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
          printf("tc_main: Unexpected read size=%zd, expected=%zd, "
                 "Ignoring\n",
                 nbytes, sizeof(struct touch_sample_s));
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
