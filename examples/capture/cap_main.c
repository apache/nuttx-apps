/****************************************************************************
 * apps/examples/capture/cap_main.c
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
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <inttypes.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/timers/capture.h>

#include "cap.h"

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

struct cap_example_s g_capexample;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cap_devpath
 ****************************************************************************/

static void cap_devpath(FAR const char *devpath)
{
  /* Get rid of any old device path */

  if (g_capexample.devpath)
    {
      free(g_capexample.devpath);
    }

  /* The set-up the new device path by copying the string */

  g_capexample.devpath = strdup(devpath);
}

/****************************************************************************
 * Name: cap_help
 ****************************************************************************/

static void cap_help(void)
{
  printf("\nUsage: cap [OPTIONS]\n\n");
  printf("OPTIONS include:\n");
  printf("  [-p devpath] Capture device path\n");
  printf("  [-n samples] Number of samples\n");
  printf("  [-t msec]    Delay between samples (msec)\n");
  printf("  [-h]         Shows this message and exits\n\n");
}

/****************************************************************************
 * Name: arg_string
 ****************************************************************************/

static int arg_string(FAR char **arg, FAR char **value)
{
  FAR char *ptr = *arg;

  if (ptr[2] == '\0')
    {
      *value = arg[1];
      return 2;
    }
  else
    {
      *value = &ptr[2];
      return 1;
    }
}

/****************************************************************************
 * Name: arg_decimal
 ****************************************************************************/

static int arg_decimal(FAR char **arg, FAR long *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 10);
  return ret;
}

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

static void parse_args(int argc, FAR char **argv)
{
  FAR char *ptr;
  FAR char *str;
  long value;
  int index;
  int nargs;

  g_capexample.nloops = CONFIG_EXAMPLES_CAPTURE_NSAMPLES;
  g_capexample.delay  = CONFIG_EXAMPLES_CAPTURE_DELAY;

  for (index = 1; index < argc; )
    {
      ptr = argv[index];
      if (ptr[0] != '-')
        {
          printf("Invalid options format: %s\n", ptr);
          exit(0);
        }

      switch (ptr[1])
        {
          case 'n':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 0 || value > INT_MAX)
              {
                printf("Sample count out of range: %ld\n", value);
                exit(1);
              }

            g_capexample.nloops = (unsigned int)value;
            index += nargs;
            break;

          case 'p':
            nargs = arg_string(&argv[index], &str);
            cap_devpath(str);
            index += nargs;
            break;

          case 't':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 0 || value > INT_MAX)
              {
                printf("Sample delay out of range: %ld\n", value);
                exit(1);
              }

            g_capexample.delay = (unsigned int)value;
            index += nargs;
            break;

          case 'h':
            cap_help();
            exit(EXIT_SUCCESS);

          default:
            printf("Unsupported option: %s\n", ptr);
            cap_help();
            exit(EXIT_FAILURE);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cap_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int8_t dutycycle;
  int32_t frequence;
  int fd;
  int exitval = EXIT_SUCCESS;
  int ret;
  int nloops;

  /* Set the default values */

  cap_devpath(CONFIG_EXAMPLES_CAPTURE_DEVPATH);

  /* Parse command line arguments */

  parse_args(argc, argv);

  /* Open the capture device for reading */

  printf("cap_main: Hardware initialized. Opening the capture device: %s\n",
         g_capexample.devpath);

  fd = open(g_capexample.devpath, O_RDONLY);
  if (fd < 0)
    {
      printf("cap_main: open %s failed: %d\n", g_capexample.devpath, errno);
      exitval = EXIT_FAILURE;
      goto errout;
    }

  /* Now loop the appropriate number of times, displaying the collected
   * encoder samples.
   */

  printf("cap_main: Number of samples: %u\n", g_capexample.nloops);
  for (nloops = 0;
       !g_capexample.nloops || nloops < g_capexample.nloops;
       nloops++)
    {
      /* Flush any output before the loop entered or from the previous pass
       * through the loop.
       */

      fflush(stdout);

      /* Get the dutycycle data using the ioctl */

      ret = ioctl(fd, CAPIOC_DUTYCYCLE,
                  (unsigned long)((uintptr_t)&dutycycle));
      if (ret < 0)
        {
          printf("cap_main: ioctl(CAPIOC_DUTYCYCLE) failed: %d\n", errno);
          exitval = EXIT_FAILURE;
          goto errout_with_dev;
        }

      /* Print the sample data on successful return */

      else
        {
          printf("pwm duty cycle: %d % \n", dutycycle);
        }

      /* Get the frequence data using the ioctl */

      ret = ioctl(fd, CAPIOC_FREQUENCE,
                  (unsigned long)((uintptr_t)&frequence));
      if (ret < 0)
        {
          printf("cap_main: ioctl(CAPIOC_FREQUENCE) failed: %d\n", errno);
          exitval = EXIT_FAILURE;
          goto errout_with_dev;
        }

      /* Print the sample data on successful return */

      else
        {
          printf("pwm frequence: %d Hz \n", frequence);
        }

      /* Delay a little bit */

      usleep(g_capexample.delay * 1000);
    }

errout_with_dev:
  close(fd);

errout:
  printf("Terminating!\n");
  fflush(stdout);
  return exitval;
}
