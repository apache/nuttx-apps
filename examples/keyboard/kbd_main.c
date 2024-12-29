/****************************************************************************
 * apps/examples/keyboard/kbd_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <nuttx/input/keyboard.h>

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
 * Name: kbd_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct keyboard_event_s sample;
  ssize_t nbytes;
  long nsamples = 0;
  int fd;
  int errval = 0;

  if (argc > 1)
    {
      nsamples = strtol(argv[1], NULL, 10);
    }

  printf("kbd_main: nsamples: %ld\n", nsamples);

  /* Open the keyboard device for reading */

  printf("kbd_main: Opening %s\n", CONFIG_EXAMPLES_KEYBOARD_DEVPATH);
  fd = open(CONFIG_EXAMPLES_KEYBOARD_DEVPATH, O_RDONLY);
  if (fd < 0)
    {
      printf("kbd_main: open %s failed: %d\n",
             CONFIG_EXAMPLES_KEYBOARD_DEVPATH, errno);
      errval = 2;
      goto errout;
    }

  /* Now loop the appropriate number of times, displaying the collected
   * keyboard samples.
   */

  for (; ; )
    {
      /* Flush any output before the loop entered or from the previous pass
       * through the loop.
       */

      fflush(stdout);

      /* Read one sample */

      iinfo("Reading...\n");
      nbytes = read(fd, &sample, sizeof(struct keyboard_event_s));
      iinfo("Bytes read: %zd\n", nbytes);

      /* Handle unexpected return values */

      if (nbytes < 0)
        {
          errval = errno;
          if (errval != EINTR)
            {
              printf("kbd_main: read %s failed: %d\n",
                     CONFIG_EXAMPLES_KEYBOARD_DEVPATH, errval);
              errval = 3;
              goto errout_with_dev;
            }

          printf("kbd_main: Interrupted read...\n");
        }
      else if (nbytes != sizeof(struct keyboard_event_s))
        {
          printf("kbd_main: Unexpected read size=%zd, expected=%zd, "
                 "Ignoring\n",
                 nbytes, sizeof(struct keyboard_event_s));
        }

      /* Print the sample data on successful return */

      else
        {
          printf("Sample  :\n");
          printf("   code : %d\n",   sample.code);
          printf("   type : %d\n",   sample.type);
        }

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
