/****************************************************************************
 * apps/examples/pwm/pwm_main.c
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
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <string.h>
#include <inttypes.h>

#include <nuttx/timers/pwm.h>

#include "pwm.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifdef CONFIG_PWM_MULTICHAN
#  if CONFIG_PWM_NCHANNELS > 1
#    if CONFIG_EXAMPLES_PWM_CHANNEL1 == CONFIG_EXAMPLES_PWM_CHANNEL2
#      error "Channel numbers must be unique"
#    endif
#  endif
#  if CONFIG_PWM_NCHANNELS > 2
#    if CONFIG_EXAMPLES_PWM_CHANNEL1 == CONFIG_EXAMPLES_PWM_CHANNEL3 || \
        CONFIG_EXAMPLES_PWM_CHANNEL2 == CONFIG_EXAMPLES_PWM_CHANNEL3
#      error "Channel numbers must be unique"
#    endif
#  endif
#  if CONFIG_PWM_NCHANNELS > 3
#    if CONFIG_EXAMPLES_PWM_CHANNEL1 == CONFIG_EXAMPLES_PWM_CHANNEL4 || \
        CONFIG_EXAMPLES_PWM_CHANNEL2 == CONFIG_EXAMPLES_PWM_CHANNEL4 || \
        CONFIG_EXAMPLES_PWM_CHANNEL3 == CONFIG_EXAMPLES_PWM_CHANNEL4
#      error "Channel numbers must be unique"
#    endif
#  endif
#  if CONFIG_PWM_NCHANNELS > 4
#    if CONFIG_EXAMPLES_PWM_CHANNEL1 == CONFIG_EXAMPLES_PWM_CHANNEL5 || \
        CONFIG_EXAMPLES_PWM_CHANNEL2 == CONFIG_EXAMPLES_PWM_CHANNEL5 || \
        CONFIG_EXAMPLES_PWM_CHANNEL3 == CONFIG_EXAMPLES_PWM_CHANNEL5 || \
        CONFIG_EXAMPLES_PWM_CHANNEL4 == CONFIG_EXAMPLES_PWM_CHANNEL5
#      error "Channel numbers must be unique"
#    endif
#  endif
#  if CONFIG_PWM_NCHANNELS > 5
#    if CONFIG_EXAMPLES_PWM_CHANNEL1 == CONFIG_EXAMPLES_PWM_CHANNEL6 || \
        CONFIG_EXAMPLES_PWM_CHANNEL2 == CONFIG_EXAMPLES_PWM_CHANNEL6 || \
        CONFIG_EXAMPLES_PWM_CHANNEL3 == CONFIG_EXAMPLES_PWM_CHANNEL6 || \
        CONFIG_EXAMPLES_PWM_CHANNEL4 == CONFIG_EXAMPLES_PWM_CHANNEL6 || \
        CONFIG_EXAMPLES_PWM_CHANNEL5 == CONFIG_EXAMPLES_PWM_CHANNEL6
#      error "Channel numbers must be unique"
#    endif
#  endif
#  if CONFIG_PWM_NCHANNELS > 6
#    error "Too many PWM channels"
#  endif
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pwm_state_s
{
  bool      initialized;
  FAR char *devpath;
#ifdef CONFIG_PWM_MULTICHAN
  uint8_t   channels[CONFIG_PWM_NCHANNELS];
  uint8_t   duties[CONFIG_PWM_NCHANNELS];
#else
  uint8_t   duty;
#endif
  uint32_t  freq;
#ifdef CONFIG_PWM_PULSECOUNT
  uint32_t  count;
#endif
  int       duration;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct pwm_state_s g_pwmstate;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwm_devpath
 ****************************************************************************/

static void pwm_devpath(FAR struct pwm_state_s *pwm, FAR const char *devpath)
{
  /* Get rid of any old device path */

  if (pwm->devpath)
    {
      free(pwm->devpath);
    }

  /* Then set-up the new device path by copying the string */

  pwm->devpath = strdup(devpath);
}

/****************************************************************************
 * Name: pwm_help
 ****************************************************************************/

static void pwm_help(FAR struct pwm_state_s *pwm)
{
#ifdef CONFIG_PWM_MULTICHAN
  uint8_t channels[CONFIG_PWM_NCHANNELS] =
  {
    CONFIG_EXAMPLES_PWM_CHANNEL1,
#if CONFIG_PWM_NCHANNELS > 1
    CONFIG_EXAMPLES_PWM_CHANNEL2,
#endif
#if CONFIG_PWM_NCHANNELS > 2
    CONFIG_EXAMPLES_PWM_CHANNEL3,
#endif
#if CONFIG_PWM_NCHANNELS > 3
    CONFIG_EXAMPLES_PWM_CHANNEL4,
#endif
#if CONFIG_PWM_NCHANNELS > 4
    CONFIG_EXAMPLES_PWM_CHANNEL5,
#endif
#if CONFIG_PWM_NCHANNELS > 5
    CONFIG_EXAMPLES_PWM_CHANNEL6,
#endif
  };

  uint8_t duties[CONFIG_PWM_NCHANNELS] =
  {
    CONFIG_EXAMPLES_PWM_DUTYPCT1,
#if CONFIG_PWM_NCHANNELS > 1
    CONFIG_EXAMPLES_PWM_DUTYPCT2,
#endif
#if CONFIG_PWM_NCHANNELS > 2
    CONFIG_EXAMPLES_PWM_DUTYPCT3,
#endif
#if CONFIG_PWM_NCHANNELS > 3
    CONFIG_EXAMPLES_PWM_DUTYPCT4,
#endif
#if CONFIG_PWM_NCHANNELS > 4
    CONFIG_EXAMPLES_PWM_DUTYPCT5,
#endif
#if CONFIG_PWM_NCHANNELS > 5
    CONFIG_EXAMPLES_PWM_DUTYPCT6,
#endif
  };

  int i;
#endif

  printf("Usage: pwm [OPTIONS]\n");
  printf("\nArguments are \"sticky\".  "
         "For example, once the PWM frequency is\n");
  printf("specified, that frequency will be re-used until it is changed.\n");
  printf("\n\"sticky\" OPTIONS include:\n");
  printf("  [-p devpath] selects the PWM device.  "
         "Default: %s Current: %s\n",
         CONFIG_EXAMPLES_PWM_DEVPATH, pwm->devpath ? pwm->devpath : "NONE");
  printf("  [-f frequency] selects the pulse frequency.  "
         "Default: %d Hz Current: %" PRIu32 " Hz\n",
         CONFIG_EXAMPLES_PWM_FREQUENCY, pwm->freq);
#ifdef CONFIG_PWM_MULTICHAN
  printf("  [[-c channel1] [[-c channel2] ...]] "
         "selects the channel number for each channel.  ");
  printf("Default:");
  for (i = 0; i < CONFIG_PWM_NCHANNELS; i++)
    {
      printf(" %d", channels[i]);
    }

  printf(" Current:");
  for (i = 0; i < CONFIG_PWM_NCHANNELS; i++)
    {
      printf(" %d", pwm->channels[i]);
    }

  printf("\n");

  printf("  [[-d duty1] [[-d duty2] ...]] "
         "selects the pulse duty as a percentage.  ");
  printf("Default:");
  for (i = 0; i < CONFIG_PWM_NCHANNELS; i++)
    {
      printf(" %d %%", duties[i]);
    }

  printf(" Current:");
  for (i = 0; i < CONFIG_PWM_NCHANNELS; i++)
    {
      printf(" %d %%", pwm->duties[i]);
    }

  printf("\n");
#else
  printf("  [-d duty] selects the pulse duty as a percentage.  "
         "Default: %d %% Current: %d %%\n",
         CONFIG_EXAMPLES_PWM_DUTYPCT, pwm->duty);
#endif
#ifdef CONFIG_PWM_PULSECOUNT
  printf("  [-n count] selects the pulse count.  "
         "Default: %d Current: %" PRIx32 "\n",
         CONFIG_EXAMPLES_PWM_PULSECOUNT, pwm->count);
#endif
  printf("  [-t duration] is the duration of the pulse train in seconds.  "
         "Default: %d Current: %d\n",
         CONFIG_EXAMPLES_PWM_DURATION, pwm->duration);
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

static void parse_args(FAR struct pwm_state_s *pwm, int argc,
                       FAR char **argv)
{
  FAR char *ptr;
  FAR char *str;
  long value;
  int index;
  int nargs;
#ifdef CONFIG_PWM_MULTICHAN
  int nchannels = 0;
  int nduties   = 0;
#endif

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
          case 'f':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1)
              {
                printf("Frequency out of range: %ld\n", value);
                exit(1);
              }

            pwm->freq = (uint32_t)value;
            index += nargs;
            break;

#ifdef CONFIG_PWM_MULTICHAN
          case 'c':
            nargs = arg_decimal(&argv[index], &value);
            if (value < -1 || value > CONFIG_PWM_NCHANNELS)
              {
                printf("Channel out of range: %ld\n", value);
                exit(1);
              }

            if (nchannels < CONFIG_PWM_NCHANNELS)
              {
                nchannels++;
              }
            else
              {
                memmove(pwm->channels, pwm->channels + 1,
                        CONFIG_PWM_NCHANNELS - 1);
              }

            pwm->channels[nchannels - 1] = (int8_t)value;
            index += nargs;
            break;
#endif

          case 'd':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 0 || value > 100)
              {
                printf("Duty out of range: %ld\n", value);
                exit(1);
              }

#ifdef CONFIG_PWM_MULTICHAN
            if (nduties < CONFIG_PWM_NCHANNELS)
              {
                nduties++;
              }
            else
              {
                memmove(pwm->duties, pwm->duties + 1,
                        CONFIG_PWM_NCHANNELS - 1);
              }

            pwm->duties[nduties - 1] = (uint8_t)value;
#else
            pwm->duty = (uint8_t)value;
#endif
            index += nargs;
            break;

#ifdef CONFIG_PWM_PULSECOUNT
          case 'n':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 0)
              {
                printf("Count must be non-negative: %ld\n", value);
                exit(1);
              }

            pwm->count = (uint32_t)value;
            index += nargs;
            break;
#endif

          case 'p':
            nargs = arg_string(&argv[index], &str);
            pwm_devpath(pwm, str);
            index += nargs;
            break;

          case 't':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1 || value > INT_MAX)
              {
                printf("Duration out of range: %ld\n", value);
                exit(1);
              }

            pwm->duration = (int)value;
            index += nargs;
            break;

          case 'h':
            pwm_help(pwm);
            exit(0);

          default:
            printf("Unsupported option: %s\n", ptr);
            pwm_help(pwm);
            exit(1);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwm_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct pwm_info_s info;
  int fd;
  int ret;
#ifdef CONFIG_PWM_MULTICHAN
  int i;
  int j;
#endif

  /* Initialize the state data */

  if (!g_pwmstate.initialized)
    {
#ifdef CONFIG_PWM_MULTICHAN
      g_pwmstate.channels[0] = CONFIG_EXAMPLES_PWM_CHANNEL1;
      g_pwmstate.duties[0]   = CONFIG_EXAMPLES_PWM_DUTYPCT1;
#if CONFIG_PWM_NCHANNELS > 1
      g_pwmstate.channels[1] = CONFIG_EXAMPLES_PWM_CHANNEL2;
      g_pwmstate.duties[1]   = CONFIG_EXAMPLES_PWM_DUTYPCT2;
#endif
#if CONFIG_PWM_NCHANNELS > 2
      g_pwmstate.channels[2] = CONFIG_EXAMPLES_PWM_CHANNEL3;
      g_pwmstate.duties[2]   = CONFIG_EXAMPLES_PWM_DUTYPCT3;
#endif
#if CONFIG_PWM_NCHANNELS > 3
      g_pwmstate.channels[3] = CONFIG_EXAMPLES_PWM_CHANNEL4;
      g_pwmstate.duties[3]   = CONFIG_EXAMPLES_PWM_DUTYPCT4;
#endif
#if CONFIG_PWM_NCHANNELS > 4
      g_pwmstate.channels[4] = CONFIG_EXAMPLES_PWM_CHANNEL5;
      g_pwmstate.duties[4]   = CONFIG_EXAMPLES_PWM_DUTYPCT5;
#endif
#if CONFIG_PWM_NCHANNELS > 5
      g_pwmstate.channels[5] = CONFIG_EXAMPLES_PWM_CHANNEL6;
      g_pwmstate.duties[5]   = CONFIG_EXAMPLES_PWM_DUTYPCT6;
#endif
#else
      g_pwmstate.duty        = CONFIG_EXAMPLES_PWM_DUTYPCT;
#endif
      g_pwmstate.freq        = CONFIG_EXAMPLES_PWM_FREQUENCY;
      g_pwmstate.duration    = CONFIG_EXAMPLES_PWM_DURATION;
#ifdef CONFIG_PWM_PULSECOUNT
      g_pwmstate.count       = CONFIG_EXAMPLES_PWM_PULSECOUNT;
#endif
      g_pwmstate.initialized = true;
    }

  /* Parse the command line */

  parse_args(&g_pwmstate, argc, argv);

#ifdef CONFIG_PWM_MULTICHAN
  for (i = 0; i < CONFIG_PWM_MULTICHAN; i++)
    {
      for (j = i + 1; j < CONFIG_PWM_MULTICHAN; j++)
        {
          if (g_pwmstate.channels[j] == g_pwmstate.channels[i])
            {
              printf("pwm_main: channel numbers must be unique\n");
              goto errout;
            }
        }
    }
#endif

  /* Has a device been assigned? */

  if (!g_pwmstate.devpath)
    {
      /* No.. use the default device */

      pwm_devpath(&g_pwmstate, CONFIG_EXAMPLES_PWM_DEVPATH);
    }

  /* Open the PWM device for reading */

  fd = open(g_pwmstate.devpath, O_RDONLY);
  if (fd < 0)
    {
      printf("pwm_main: open %s failed: %d\n", g_pwmstate.devpath, errno);
      goto errout;
    }

  /* Configure the characteristics of the pulse train */

  info.frequency = g_pwmstate.freq;
#ifdef CONFIG_PWM_MULTICHAN
  printf("pwm_main: starting output with frequency: %" PRIu32,
         info.frequency);

  for (i = 0; i < CONFIG_PWM_NCHANNELS; i++)
    {
      info.channels[i].channel = g_pwmstate.channels[i];
      info.channels[i].duty = b16divi(uitoub16(g_pwmstate.duties[i]), 100);
      printf(" channel: %d duty: %08" PRIx32,
        info.channels[i].channel, info.channels[i].duty);
    }

  printf("\n");

#else
  info.duty  = b16divi(uitoub16(g_pwmstate.duty), 100);
#  ifdef CONFIG_PWM_PULSECOUNT
  info.count = g_pwmstate.count;

  printf("pwm_main: starting output "
         "with frequency: %" PRIu32 " duty: %08" PRIx32
         " count: %" PRIx32 "\n",
         info.frequency, (uint32_t)info.duty, info.count);

#  else
  printf("pwm_main: starting output "
         "with frequency: %" PRIu32 " duty: %08" PRIx32 "\n",
         info.frequency, (uint32_t)info.duty);

#  endif
#endif

  ret = ioctl(fd, PWMIOC_SETCHARACTERISTICS,
              (unsigned long)((uintptr_t)&info));
  if (ret < 0)
    {
      printf("pwm_main: ioctl(PWMIOC_SETCHARACTERISTICS) failed: %d\n",
             errno);
      goto errout_with_dev;
    }

  /* Then start the pulse train.  Since the driver was opened in blocking
   * mode, this call will block if the count value is greater than zero.
   */

  ret = ioctl(fd, PWMIOC_START, 0);
  if (ret < 0)
    {
      printf("pwm_main: ioctl(PWMIOC_START) failed: %d\n", errno);
      goto errout_with_dev;
    }

  /* It a non-zero count was not specified, then wait for the selected
   * duration, then stop the PWM output.
   */

#ifdef CONFIG_PWM_PULSECOUNT
  if (info.count == 0)
#endif
    {
      /* Wait for the specified duration */

      sleep(g_pwmstate.duration);

      /* Then stop the pulse train */

      printf("pwm_main: stopping output\n");

      ret = ioctl(fd, PWMIOC_STOP, 0);
      if (ret < 0)
        {
          printf("pwm_main: ioctl(PWMIOC_STOP) failed: %d\n", errno);
          goto errout_with_dev;
        }
    }

  close(fd);
  fflush(stdout);
  return OK;

errout_with_dev:
  close(fd);
errout:
  fflush(stdout);
  return ERROR;
}
