/****************************************************************************
 * apps/examples/adc/adc_main.c
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
#include <errno.h>
#include <debug.h>

#include <nuttx/analog/adc.h>
#include <nuttx/analog/ioctl.h>

#include "adc.h"

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

static struct adc_state_s g_adcstate;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: adc_devpath
 ****************************************************************************/

static void adc_devpath(FAR struct adc_state_s *adc, FAR const char *devpath)
{
  /* Get rid of any old device path */

  if (adc->devpath)
    {
      free(adc->devpath);
    }

  /* Then set-up the new device path by copying the string */

  adc->devpath = strdup(devpath);
}

/****************************************************************************
 * Name: adc_help
 ****************************************************************************/

static void adc_help(FAR struct adc_state_s *adc)
{
  printf("Usage: adc [OPTIONS]\n");
  printf("\nArguments are \"sticky\".  "
         "For example, once the ADC device is\n");
  printf("specified, that device will be re-used until it is changed.\n");
  printf("\n\"sticky\" OPTIONS include:\n");
  printf("  [-p devpath] selects the ADC device.  "
         "Default: %s Current: %s\n",
         CONFIG_EXAMPLES_ADC_DEVPATH,
         g_adcstate.devpath ? g_adcstate.devpath : "NONE");
  printf("  [-n count] selects the samples to collect.  "
         "Default: 1 Current: %d\n", adc->count);
  printf("  [-h] shows this message and exits\n");
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

static void parse_args(FAR struct adc_state_s *adc, int argc,
                       FAR char **argv)
{
  FAR char *ptr;
  FAR char *str;
  long value;
  int index;
  int nargs;

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
            if (value < 0)
              {
                printf("Count must be non-negative: %ld\n", value);
                exit(1);
              }

            adc->count = (uint32_t)value;
            index += nargs;
            break;

          case 'p':
            nargs = arg_string(&argv[index], &str);
            adc_devpath(adc, str);
            index += nargs;
            break;

          case 'h':
            adc_help(adc);
            exit(0);

          default:
            printf("Unsupported option: %s\n", ptr);
            adc_help(adc);
            exit(1);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: adc_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct adc_msg_s sample[CONFIG_EXAMPLES_ADC_GROUPSIZE];
  size_t readsize;
  ssize_t nbytes;
  int fd;
  int errval = 0;
  int ret;
  int i;

  UNUSED(ret);

  /* Check if we have initialized */

  if (!g_adcstate.initialized)
    {
      /* Initialization of the ADC hardware must be performed by
       * board-specific logic prior to running this test.
       */

      /* Set the default values */

      adc_devpath(&g_adcstate, CONFIG_EXAMPLES_ADC_DEVPATH);

      g_adcstate.initialized = true;
    }

  g_adcstate.count = CONFIG_EXAMPLES_ADC_NSAMPLES;

  /* Parse the command line */

  parse_args(&g_adcstate, argc, argv);

  /* If this example is configured as an NX add-on, then limit the number of
   * samples that we collect before returning.  Otherwise, we never return
   */

  printf("adc_main: g_adcstate.count: %d\n", g_adcstate.count);

  /* Open the ADC device for reading */

  printf("adc_main: Hardware initialized. Opening the ADC device: %s\n",
         g_adcstate.devpath);

  fd = open(g_adcstate.devpath, O_RDONLY);
  if (fd < 0)
    {
      printf("adc_main: open %s failed: %d\n", g_adcstate.devpath, errno);
      errval = 2;
      goto errout;
    }

  /* Now loop the appropriate number of times, displaying the collected
   * ADC samples.
   */

  for (; ; )
    {
      /* Flush any output before the loop entered or from the previous pass
       * through the loop.
       */

      fflush(stdout);

#ifdef CONFIG_EXAMPLES_ADC_SWTRIG
      /* Issue the software trigger to start ADC conversion */

      ret = ioctl(fd, ANIOC_TRIGGER, 0);
      if (ret < 0)
        {
          int errcode = errno;
          printf("adc_main: ANIOC_TRIGGER ioctl failed: %d\n", errcode);
        }
#endif

      /* Read up to CONFIG_EXAMPLES_ADC_GROUPSIZE samples */

      readsize = CONFIG_EXAMPLES_ADC_GROUPSIZE * sizeof(struct adc_msg_s);
      nbytes = read(fd, sample, readsize);

      /* Handle unexpected return values */

      if (nbytes < 0)
        {
          errval = errno;
          if (errval != EINTR)
            {
              printf("adc_main: read %s failed: %d\n",
                     g_adcstate.devpath, errval);
              errval = 3;
              goto errout_with_dev;
            }

          printf("adc_main: Interrupted read...\n");
        }
      else if (nbytes == 0)
        {
          printf("adc_main: No data read, Ignoring\n");
        }

      /* Print the sample data on successful return */

      else
        {
          int nsamples = nbytes / sizeof(struct adc_msg_s);
          if (nsamples * sizeof(struct adc_msg_s) != nbytes)
            {
              printf("adc_main: read size=%ld is not a multiple of "
                     "sample size=%d, Ignoring\n",
                     (long)nbytes, sizeof(struct adc_msg_s));
            }
          else
            {
              printf("Sample:\n");
              for (i = 0; i < nsamples; i++)
                {
                  printf("%d: channel: %d value: %" PRId32 "\n",
                         i + 1, sample[i].am_channel, sample[i].am_data);
                }
            }
        }

      if (g_adcstate.count && --g_adcstate.count <= 0)
        {
          break;
        }
    }

  close(fd);
  return OK;

  /* Error exits */

errout_with_dev:
  close(fd);

errout:
  printf("Terminating!\n");
  fflush(stdout);
  return errval;
}
