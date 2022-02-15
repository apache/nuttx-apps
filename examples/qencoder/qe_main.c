/****************************************************************************
 * apps/examples/qencoder/qe_main.c
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

#include <nuttx/sensors/qencoder.h>

#include "qe.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_QENCODER_HAVE_MAXPOS
#  if CONFIG_EXAMPLES_QENCODER_MAXPOS == 0
#    error CONFIG_EXAMPLES_QENCODER_MAXPOS not specified
#  endif
#endif

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

struct qe_example_s g_qeexample;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: qe_devpath
 ****************************************************************************/

static void qe_devpath(FAR const char *devpath)
{
  /* Get rid of any old device path */

  if (g_qeexample.devpath)
    {
      free(g_qeexample.devpath);
    }

  /* The set-up the new device path by copying the string */

  g_qeexample.devpath = strdup(devpath);
}

/****************************************************************************
 * Name: qe_help
 ****************************************************************************/

static void qe_help(void)
{
  printf("\nUsage: qe [OPTIONS]\n\n");
  printf("OPTIONS include:\n");
  printf("  [-p devpath] QE device path\n");
  printf("  [-n samples] Number of samples\n");
  printf("  [-t msec]    Delay between samples (msec)\n");
  printf("  [-r]         Reset the position to zero\n");
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

  g_qeexample.reset  = false;
  g_qeexample.nloops = CONFIG_EXAMPLES_QENCODER_NSAMPLES;
  g_qeexample.delay  = CONFIG_EXAMPLES_QENCODER_DELAY;

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

            g_qeexample.nloops = (unsigned int)value;
            index += nargs;
            break;

          case 'p':
            nargs = arg_string(&argv[index], &str);
            qe_devpath(str);
            index += nargs;
            break;

          case 't':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 0 || value > INT_MAX)
              {
                printf("Sample delay out of range: %ld\n", value);
                exit(1);
              }

            g_qeexample.delay = (unsigned int)value;
            index += nargs;
            break;

          case 'r':
            g_qeexample.reset = true;
            index++;
            break;

          case 'h':
            qe_help();
            exit(EXIT_SUCCESS);

          default:
            printf("Unsupported option: %s\n", ptr);
            qe_help();
            exit(EXIT_FAILURE);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: qe_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int32_t position;
  int fd;
  int exitval = EXIT_SUCCESS;
  int ret;
  int nloops;

  /* Set the default values */

  qe_devpath(CONFIG_EXAMPLES_QENCODER_DEVPATH);

  /* Parse command line arguments */

  parse_args(argc, argv);

  /* Open the encoder device for reading */

  printf("qe_main: Hardware initialized. Opening the encoder device: %s\n",
         g_qeexample.devpath);

  fd = open(g_qeexample.devpath, O_RDONLY);
  if (fd < 0)
    {
      printf("qe_main: open %s failed: %d\n", g_qeexample.devpath, errno);
      exitval = EXIT_FAILURE;
      goto errout;
    }

#ifdef CONFIG_EXAMPLES_QENCODER_HAVE_MAXPOS
  /* Set the maximum encoder positions */

  ret = ioctl(fd, QEIOC_SETPOSMAX,
              (unsigned long)CONFIG_EXAMPLES_QENCODER_MAXPOS);
  if (ret < 0)
    {
      printf("qe_main: ioctl(QEIOC_SETMAXPOS) failed: %d\n", errno);
      exitval = EXIT_FAILURE;
      goto errout_with_dev;
    }
#endif

  /* Reset the count if so requested */

  if (g_qeexample.reset)
    {
      printf("qe_main: Resetting the count...\n");
      ret = ioctl(fd, QEIOC_RESET, 0);
      if (ret < 0)
        {
          printf("qe_main: ioctl(QEIOC_RESET) failed: %d\n", errno);
          exitval = EXIT_FAILURE;
          goto errout_with_dev;
        }
    }

  /* Now loop the appropriate number of times, displaying the collected
   * encoder samples.
   */

  printf("qe_main: Number of samples: %u\n", g_qeexample.nloops);
  for (nloops = 0;
       !g_qeexample.nloops || nloops < g_qeexample.nloops;
       nloops++)
    {
      /* Flush any output before the loop entered or from the previous pass
       * through the loop.
       */

      fflush(stdout);

      /* Get the positions data using the ioctl */

      ret = ioctl(fd, QEIOC_POSITION, (unsigned long)((uintptr_t)&position));
      if (ret < 0)
        {
          printf("qe_main: ioctl(QEIOC_POSITION) failed: %d\n", errno);
          exitval = EXIT_FAILURE;
          goto errout_with_dev;
        }

      /* Print the sample data on successful return */

      else
        {
          printf("qe_main: %3d. %" PRIi32 "\n", nloops + 1, position);
        }

      /* Delay a little bit */

      usleep(g_qeexample.delay * 1000);
    }

errout_with_dev:
  close(fd);

errout:
  printf("Terminating!\n");
  fflush(stdout);
  return exitval;
}
