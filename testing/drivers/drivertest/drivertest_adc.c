/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_adc.c
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

#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <sys/time.h>
#include <stdlib.h>

#include <cmocka.h>
#include <nuttx/analog/adc.h>
#include <nuttx/analog/ioctl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define OPTARG_TO_VALUE(value, type, base)                   \
    do {                                                     \
        FAR char* ptr;                                       \
        value = (type)strtoul(optarg, &ptr, base);           \
        if (*ptr != '\0') {                                  \
            printf("Parameter error: -%c %s\n", ch, optarg); \
            adc_help(argv[0]);                               \
        }                                                    \
    } while (0)

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

struct adc_state_s
{
  char devpath[PATH_MAX]; /* device to adc device path */
  int adc_diff;           /* adc value difference */
  int duration;           /* duration of sampling adc in seconds */
  bool soft_trigger;      /* soft trigger : true or false */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void adc_help(FAR char *process_name)
{
  printf("Usage: %s [OPTIONS]\n", process_name);
  printf("  -p adc device path , default : /dev/adc0\n");
  printf("  -d sample adc changes over set-up value. default : 200\n");
  printf("  -t duration of adc test [second] , default: 10s\n");
  printf("  -m adc conversion method, 1 --  soft trigger, "
        "0 -- interrupt , default: 1\n");
  exit(-1);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(FAR struct adc_state_s *adc_state, int argc,
                              FAR char **argv)
{
  int ch;
  int converted;

  while ((ch = getopt(argc, argv, "p:d:m:t:h")) != ERROR)
    {
      switch (ch)
        {
        case 'p':
            strlcpy(adc_state->devpath, optarg, sizeof(adc_state->devpath));
            adc_state->devpath[sizeof(adc_state->devpath) - 1] = '\0';
            break;
        case 'd':
            OPTARG_TO_VALUE(converted, int, 10);
            if (converted < 0 || converted > 5000)
              {
                printf("sample adc value changes over: %d\n", converted);
                adc_help(argv[0]);
              }

            adc_state->adc_diff = (uint8_t)converted;
            break;
        case 't':
            OPTARG_TO_VALUE(converted, int, 10);
            if (converted < 1 || converted > INT_MAX)
              {
                printf("Duty out of range: %d\n", converted);
                adc_help(argv[0]);
              }

            adc_state->duration = (int)converted;
            break;
        case 'm':
            OPTARG_TO_VALUE(converted, uint8_t, 10);
            adc_state->soft_trigger = converted ? true : false;
            break;
        case '?':
            printf("Unsupported option: %s\n", optarg);

        case 'h':
            adc_help(argv[0]);
            break;
        }
    }
}

int32_t adc_read_one_sample(int fd, bool soft_trigger)
{
  struct adc_msg_s sample;
  int ret;
  int nbytes;

  /* software trigger to start one ADC conversion */

  if (soft_trigger)
    {
      ret = ioctl(fd, ANIOC_TRIGGER, 0);
      assert_return_code(ret, OK);
    }

  /* Read one samples */

  nbytes = read(fd, &sample, sizeof(struct adc_msg_s));

  /* Handle unexpected return values */

  assert_true(nbytes == sizeof(struct adc_msg_s));

  return sample.am_data;
}

/****************************************************************************
 * Name: drivertest_adc
 ****************************************************************************/

static void drivertest_adc(FAR void** state)
{
  int fd;
  bool succ = false;
  int32_t value1;
  int32_t value2;
  struct timeval tv1;
  struct timeval tv2;
  struct timeval res;
  FAR struct adc_state_s *adc_state;
  adc_state = (FAR struct adc_state_s *)*state;

  /* Open the ADC device for reading */

  fd = open(adc_state->devpath, O_RDONLY);
  assert_true(fd > 0);

  value1 = adc_read_one_sample(fd, adc_state->soft_trigger);

  /* ADC sample value should be changed in duration [seconds] */

  gettimeofday(&tv1, NULL);
  while (true)
    {
      value2 = adc_read_one_sample(fd, adc_state->soft_trigger);
      if (abs(value2 - value1) > adc_state->adc_diff)
        {
          succ = true;
          break;
        }

      gettimeofday(&tv2, NULL);
      timersub(&tv2, &tv1, &res);
      if (res.tv_sec >= adc_state->duration)
        {
          printf("adc test timed out\n");
          break;
        }

      usleep(adc_state->soft_trigger ? 1000000 : 200000);
    }

  close(fd);
  assert_true(succ);
}

/****************************************************************************
 * drivertest_adc_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Initialize the state data */

  struct adc_state_s adc_state = {
      .devpath = "/dev/adc0",
      .adc_diff = 200,
      .duration = 10,
      .soft_trigger = true,
  };

  parse_commandline(&adc_state, argc, argv);

  const struct CMUnitTest tests[] = {
      cmocka_unit_test_prestate(drivertest_adc, &adc_state),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
