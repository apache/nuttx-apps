/****************************************************************************
 * apps/examples/pulsecount/pulsecount_main.c
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

#include <sys/ioctl.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/timers/pulsecount.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pulsecount_state_s
{
  FAR char *devpath;
  uint32_t high_ns;
  uint32_t low_ns;
  uint32_t count;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void pulsecount_devpath(FAR struct pulsecount_state_s *state,
                               FAR const char *devpath)
{
  if (state->devpath != NULL)
    {
      free(state->devpath);
    }

  state->devpath = strdup(devpath);
}

static int arg_string(FAR char **arg, FAR char **value)
{
  FAR char *ptr = *arg;

  if (ptr[2] == '\0')
    {
      *value = arg[1];
      return 2;
    }

  *value = &ptr[2];
  return 1;
}

static int arg_decimal(FAR char **arg, FAR long *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 10);
  return ret;
}

static void pulsecount_help(FAR struct pulsecount_state_s *state)
{
  printf("Usage: pulsecount [OPTIONS]\n");
  printf("  [-p devpath] Default: %s Current: %s\n",
         CONFIG_EXAMPLES_PULSECOUNT_DEVPATH,
         state->devpath != NULL ? state->devpath : "NONE");
  printf("  [-H high-ns] Default: %d ns Current: %" PRIu32 " ns\n",
         CONFIG_EXAMPLES_PULSECOUNT_HIGH_NS, state->high_ns);
  printf("  [-L low-ns] Default: %d ns Current: %" PRIu32 " ns\n",
         CONFIG_EXAMPLES_PULSECOUNT_LOW_NS, state->low_ns);
  printf("  [-n count] Default: %d Current: %" PRIu32 "\n",
         CONFIG_EXAMPLES_PULSECOUNT_COUNT, state->count);
  printf("  [-h] shows this message and exits\n");
}

static void parse_args(FAR struct pulsecount_state_s *state, int argc,
                       FAR char **argv)
{
  FAR char *str;
  long value;
  int index;
  int nargs;

  for (index = 1; index < argc; )
    {
      if (argv[index][0] != '-')
        {
          printf("Invalid options format: %s\n", argv[index]);
          exit(1);
        }

      switch (argv[index][1])
        {
          case 'p':
            nargs = arg_string(&argv[index], &str);
            pulsecount_devpath(state, str);
            index += nargs;
            break;

          case 'H':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1)
              {
                printf("High time out of range: %ld\n", value);
                exit(1);
              }

            state->high_ns = (uint32_t)value;
            index += nargs;
            break;

          case 'L':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1)
              {
                printf("Low time out of range: %ld\n", value);
                exit(1);
              }

            state->low_ns = (uint32_t)value;
            index += nargs;
            break;

          case 'n':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1)
              {
                printf("Count out of range: %ld\n", value);
                exit(1);
              }

            state->count = (uint32_t)value;
            index += nargs;
            break;

          case 'h':
            pulsecount_help(state);
            exit(0);

          default:
            printf("Unsupported option: %s\n", argv[index]);
            pulsecount_help(state);
            exit(1);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct pulsecount_state_s state;
  struct pulsecount_info_s info;
  int fd;
  int ret;

  /* Defaults */

  memset(&state, 0, sizeof(state));
  state.high_ns = CONFIG_EXAMPLES_PULSECOUNT_HIGH_NS;
  state.low_ns = CONFIG_EXAMPLES_PULSECOUNT_LOW_NS;
  state.count = CONFIG_EXAMPLES_PULSECOUNT_COUNT;

  /* Parse args */

  parse_args(&state, argc, argv);

  if (state.devpath == NULL)
    {
      pulsecount_devpath(&state, CONFIG_EXAMPLES_PULSECOUNT_DEVPATH);
    }

  fd = open(state.devpath, O_RDONLY);
  if (fd < 0)
    {
      printf("pulsecount: open %s failed: %d\n", state.devpath, errno);
      goto errout;
    }

  memset(&info, 0, sizeof(info));
  info.high_ns = state.high_ns;
  info.low_ns = state.low_ns;
  info.count = state.count;

  printf("pulsecount: high: %" PRIu32 " ns low: %" PRIu32
         " ns count: %" PRIu32 "\n",
         info.high_ns, info.low_ns, info.count);

  ret = ioctl(fd, PULSECOUNTIOC_SETCHARACTERISTICS,
              (unsigned long)((uintptr_t)&info));
  if (ret < 0)
    {
      printf("pulsecount: ioctl(PULSECOUNTIOC_SETCHARACTERISTICS) "
             "failed: %d\n", errno);
      goto errout_with_dev;
    }

  ret = ioctl(fd, PULSECOUNTIOC_START, 0);
  if (ret < 0)
    {
      printf("pulsecount: ioctl(PULSECOUNTIOC_START) failed: %d\n", errno);
      goto errout_with_dev;
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
