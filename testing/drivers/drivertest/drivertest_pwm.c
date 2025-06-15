/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_pwm.c
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
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <string.h>
#include <inttypes.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

#include <nuttx/timers/pwm.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PWM_DEFAULT_COUNT      0
#define PWM_DEFAULT_DURATION   5
#define PWM_DEFAULT_DUTY       50
#define PWM_DEFAULT_FREQUENCY  100
#define PWM_DEFAULT_DEVPATH    "/dev/pwm0"

#define OPTARG_TO_VALUE(value, type, base)                            \
  do                                                                  \
    {                                                                 \
      FAR char *ptr;                                                  \
      value = (type)strtoul(optarg, &ptr, base);                      \
      if (*ptr != '\0')                                               \
        {                                                             \
          printf("Parameter error: -%c %s\n", ch, optarg);            \
          pwm_help(argv[0], pwm_state, EXIT_FAILURE);                 \
        }                                                             \
    } while (0)

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

struct pwm_state_s
{
  char devpath[PATH_MAX];
  uint8_t duty;
  uint32_t freq;
#ifdef CONFIG_PWM_PULSECOUNT
  uint32_t count;
#endif
  int duration;
};

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
 * Name: pwm_help
 ****************************************************************************/

static void pwm_help(FAR const char *progname,
                     FAR struct pwm_state_s *pwm_state, int exitcode)
{
  printf("Usage: %s"
         " -p <devpath> -f <frequency> -d <duty> -n <count> -t <duration>\n",
         progname);
  printf("\nArguments are \"sticky\".  "
         "For example, once the PWM frequency is\n");
  printf("specified, that frequency will be re-used until it is changed.\n");
  printf("\n\"sticky\" OPTIONS include:\n");
  printf("  [-p devpath] selects the PWM device.  "
         "Default: %s Current: %s\n", PWM_DEFAULT_DEVPATH,
         pwm_state->devpath);
  printf("  [-f frequency] selects the pulse frequency.  "
         "Default: %d Hz Current: %" PRIu32 " Hz\n",
         PWM_DEFAULT_FREQUENCY, pwm_state->freq);
  printf("  [-d duty] selects the pulse duty as a percentage.  "
         "Default: %d %% Current: %d %%\n",
         PWM_DEFAULT_DUTY, pwm_state->duty);

#ifdef CONFIG_PWM_PULSECOUNT
  printf("  [-n count] selects the pulse count.  "
         "Default: %d Current: %" PRIx32 "\n",
         PWM_DEFAULT_COUNT, pwm_state->count);
#endif
  printf("  [-t duration] is the duration of the pulse train in seconds.  "
         "Default: %d Current: %d\n",
         PWM_DEFAULT_DURATION, pwm_state->duration);
  printf("  [-h] shows this message and exits\n");

  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(FAR struct pwm_state_s *pwm_state, int argc,
                              FAR char **argv)
{
  int ch;
  int converted;

  while ((ch = getopt(argc, argv, "p:d:n:f:t:h")) != ERROR)
    {
      switch (ch)
        {
          case 'p':
            strlcpy(pwm_state->devpath, optarg, sizeof(pwm_state->devpath));
            break;
          case 'd':
            OPTARG_TO_VALUE(converted, uint8_t, 10);
            if (converted < 0 || converted > 100)
              {
                printf("Duty out of range: %d\n", converted);
                pwm_help(argv[0], pwm_state, EXIT_FAILURE);
              }

            pwm_state->duty = (uint8_t)converted;
            break;
#ifdef CONFIG_PWM_PULSECOUNT
          case 'n':
            OPTARG_TO_VALUE(converted, uint32_t, 10);
            if (converted < 0)
              {
                printf("Count must be non-negative: %ld\n", converted);
                pwm_help(argv[0], pwm_state, EXIT_FAILURE);
              }

            pwm_state->count = (uint32_t)value;
            break;
#endif
          case 'f':
            OPTARG_TO_VALUE(converted, uint32_t, 10);
            if (converted < 0)
              {
                printf("Frequency out of range: %d\n", converted);
                pwm_help(argv[0], pwm_state, EXIT_FAILURE);
              }

            pwm_state->freq = (uint32_t)converted;
            break;
          case 't':
            OPTARG_TO_VALUE(converted, int, 10);
            if (converted < 1 || converted > INT_MAX)
              {
                printf("Duty out of range: %d\n", converted);
                pwm_help(argv[0], pwm_state, EXIT_FAILURE);
              }

            pwm_state->duration = (int)converted;
            break;
          case 'h':
            pwm_help(argv[0], pwm_state, EXIT_FAILURE);
            break;
          case '?':
            printf("Unsupported option: %s\n", optarg);
            pwm_help(argv[0], pwm_state, EXIT_FAILURE);
            break;
        }
    }

  printf("devname = %s\n"
         "duty = %d\n"
         "frenquency = %" PRIu32 "\n"
         "duration = %d\n",
         pwm_state->devpath,
         pwm_state->duty,
         pwm_state->freq,
         pwm_state->duration);

#ifdef CONFIG_PWM_PULSECOUNT
  printf("count = %" PRIu32 "\n", pwm_state->count);
#endif
}

/****************************************************************************
 * Name: test_case_pwm
 ****************************************************************************/

static void drivertest_pwm(FAR void **state)
{
  int fd;
  int ret;
#ifdef CONFIG_PWM_MULTICHAN
  int i;
#endif
  struct pwm_info_s info;
  FAR struct pwm_state_s *pwm_state;
  pwm_state = (FAR struct pwm_state_s *)*state;

  /* Open the PWM device for reading */

  fd = open(pwm_state->devpath, O_RDONLY);
  assert_true(fd > 0);

  /* Configure the characteristics of the pulse train */

  memset(&info, 0, sizeof(info));

  info.frequency = pwm_state->freq;

#ifdef CONFIG_PWM_MULTICHAN
  for (i = 0; i < CONFIG_PWM_NCHANNELS; i++)
    {
      info.channels[i].channel = i + 1;
      info.channels[i].duty = b16divi(uitoub16(pwm_state->duty), 100);
    }
#else
  info.duty = b16divi(uitoub16(pwm_state->duty), 100);
#endif

#ifdef CONFIG_PWM_PULSECOUNT
  info.count = pwm_state.count;
#endif

  ret = ioctl(fd, PWMIOC_SETCHARACTERISTICS,
              (unsigned long)((uintptr_t)&info));
  assert_return_code(ret, OK);

  /* Then start the pulse train.  Since the driver was opened in blocking
   * mode, this call will block if the count value is greater than zero.
   */

  ret = ioctl(fd, PWMIOC_START, 0);
  assert_return_code(ret, OK);

  /* It a non-zero count was not specified, then wait for the selected
   * duration, then stop the PWM output.
   */

#ifdef CONFIG_PWM_PULSECOUNT
  if (info.count == 0)
#endif
    {
      /* Wait for the specified duration */

      sleep(pwm_state->duration);

      /* Then stop the pulse train */

      ret = ioctl(fd, PWMIOC_STOP, 0);
      assert_return_code(ret, OK);
    }

  close(fd);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Initialize the state data */

  struct pwm_state_s pwm_state =
  {
      .devpath = PWM_DEFAULT_DEVPATH,
      .duty = PWM_DEFAULT_DUTY,
      .freq = PWM_DEFAULT_FREQUENCY,
      .duration = PWM_DEFAULT_DURATION,
#ifdef CONFIG_PWM_PULSECOUNT
      .count = PWM_DEFAULT_COUNT,
#endif
  };

  parse_commandline(&pwm_state, argc, argv);

  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_prestate(drivertest_pwm, &pwm_state)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
