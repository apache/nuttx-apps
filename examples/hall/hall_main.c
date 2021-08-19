/****************************************************************************
 * apps/examples/hall/hall_main.c
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

#include <nuttx/sensors/hall3ph.h>

#include "hall.h"

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

struct hall_example_s g_hallexample;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hall_devpath
 ****************************************************************************/

static void hall_devpath(FAR const char *devpath)
{
  /* Get rid of any old device path */

  if (g_hallexample.devpath)
    {
      free(g_hallexample.devpath);
    }

  /* The set-up the new device path by copying the string */

  g_hallexample.devpath = strdup(devpath);
}

/****************************************************************************
 * Name: hall_help
 ****************************************************************************/

static void hall_help(void)
{
  printf("\nUsage: hall [OPTIONS]\n\n");
  printf("OPTIONS include:\n");
  printf("  [-p devpath] HALL device path\n");
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

  g_hallexample.nloops = CONFIG_EXAMPLES_HALL_NSAMPLES;
  g_hallexample.delay  = CONFIG_EXAMPLES_HALL_DELAY;

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

            g_hallexample.nloops = (unsigned int)value;
            index += nargs;
            break;

          case 'p':
            nargs = arg_string(&argv[index], &str);
            hall_devpath(str);
            index += nargs;
            break;

          case 't':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 0 || value > INT_MAX)
              {
                printf("Sample delay out of range: %ld\n", value);
                exit(1);
              }

            g_hallexample.delay = (unsigned int)value;
            index += nargs;
            break;

          case 'h':
            hall_help();
            exit(EXIT_SUCCESS);

          default:
            printf("Unsupported option: %s\n", ptr);
            hall_help();
            exit(EXIT_FAILURE);
        }
    }
}

/****************************************************************************
 * Name: decode_hall
 ****************************************************************************/

uint8_t decode_hall(uint8_t hall)
{
  uint8_t sector = 0;

#if defined(CONFIG_EXAMPLES_HALL_SENSOR_120DEG)
  switch (hall)
    {
      case HALL3_120DEG_POS_1:
        {
          sector = 1;
          break;
        }

      case HALL3_120DEG_POS_2:
        {
          sector = 2;
          break;
        }

      case HALL3_120DEG_POS_3:
        {
          sector = 3;
          break;
        }

      case HALL3_120DEG_POS_4:
        {
          sector = 4;
          break;
        }

      case HALL3_120DEG_POS_5:
        {
          sector = 5;
          break;
        }

      case HALL3_120DEG_POS_6:
        {
          sector = 6;
          break;
        }

      default:
        {
          printf("ERROR: Invalid hall value %d\n", hall);
          sector = 0;
          break;
        }
    }
#elif defined(CONFIG_EXAMPLES_HALL_SENSOR_60DEG)
  switch (hall)
    {
      case HALL3_60DEG_POS_1:
        {
          sector = 1;
          break;
        }

      case HALL3_60DEG_POS_2:
        {
          sector = 2;
          break;
        }

      case HALL3_60DEG_POS_3:
        {
          sector = 3;
          break;
        }

      case HALL3_60DEG_POS_4:
        {
          sector = 4;
          break;
        }

      case HALL3_60DEG_POS_5:
        {
          sector = 5;
          break;
        }

      case HALL3_60DEG_POS_6:
        {
          sector = 6;
          break;
        }

      default:
        {
          printf("ERROR: Invalid hall value %d\n", hall);
          sector = 0;
          break;
        }
    }
#else
#  error
#endif

  return sector;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hall_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int     exitval  = EXIT_SUCCESS;
  uint8_t position = 0;
  int     fd       = 0;
  int     ret      = 0;
  int     nloops   = 0;
  int     sector   = 0;

  /* Set the default values */

  hall_devpath(CONFIG_EXAMPLES_HALL_DEVPATH);

  /* Parse command line arguments */

  parse_args(argc, argv);

  /* Open the encoder device for reading */

  printf("hall_main: Hardware initialized. Opening the encoder device: %s\n",
         g_hallexample.devpath);

  fd = open(g_hallexample.devpath, O_RDONLY);
  if (fd < 0)
    {
      printf("hall_main: open %s failed: %d\n", g_hallexample.devpath,
             errno);
      exitval = EXIT_FAILURE;
      goto errout;
    }

  /* Now loop the appropriate number of times, displaying the collected
   * encoder samples.
   */

  printf("hall_main: Number of samples: %u\n", g_hallexample.nloops);
  for (nloops = 0;
       !g_hallexample.nloops || nloops < g_hallexample.nloops;
       nloops++)
    {
      /* Flush any output before the loop entered or from the previous pass
       * through the loop.
       */

      fflush(stdout);

      /* Get the positions data using the ioctl */

      ret = ioctl(fd, SNIOC_GET_POSITION,
                  (unsigned long)((uintptr_t)&position));
      if (ret < 0)
        {
          printf("hall_main: ioctl(SNIOC_GET_POSITION) failed: %d\n", errno);
          exitval = EXIT_FAILURE;
          goto errout_with_dev;
        }

      /* Print the sample data on successful return */

      else
        {
          sector = decode_hall(position);
          printf("hall_main: %3d. hall=%" PRIu8 " sector=%" PRIu8 "\n",
                 nloops + 1, position, sector);
        }

      /* Delay a little bit */

      usleep(g_hallexample.delay * 1000);
    }

errout_with_dev:
  close(fd);

errout:
  printf("Terminating!\n");
  fflush(stdout);
  return exitval;
}
