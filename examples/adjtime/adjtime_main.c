/****************************************************************************
 * apps/examples/adjtime/adjtime_main.c
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
#include <stdio.h>
#include <sys/time.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwm_help
 ****************************************************************************/

static void adjtime_help(void)
{
  printf("Usage: adjtime [OPTIONS]\n");
  printf("  [-s delta->tv_sec] sets delta in seconds.\n");
  printf("  [-u delta->tv_usec] sets delta in micro seconds.\n");
  printf("  [-h] shows this message and exits\n");
  printf("Both delta->tv_sec and delta->tv_usec are set to 0 by default\n");
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

static int arg_decimal(FAR char **arg, FAR unsigned long *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = strtoul(string, NULL, 10);
  return ret;
}

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

static void parse_args(FAR struct timeval *delta, int argc,
                       FAR char **argv)
{
  FAR char *ptr;
  unsigned long value;
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
          case 's':
            nargs = arg_decimal(&argv[index], &value);

            delta->tv_sec = value;
            index += nargs;
            break;

          case 'u':
            nargs = arg_decimal(&argv[index], &value);

            delta->tv_usec = value;
            index += nargs;
            break;

          case 'h':
            adjtime_help();
            exit(0);

          default:
            printf("Unsupported option: %s\n", ptr);
            adjtime_help();
            exit(1);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * adjtime_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct timeval delta;
  struct timeval olddelta;
  int ret;

  /* Set delta between real time and system tick. */

  delta.tv_sec = 0;
  delta.tv_usec = 0;

  parse_args(&delta, argc, argv);

  printf("Delta time is %ld seconds and %ld micro seconds.\n",
         (long)delta.tv_sec, delta.tv_usec);

  /* Call adjtime function. */

  ret = adjtime(&delta, &olddelta);
  if (ret < 0)
    {
      printf("ERROR: adjtime() failed: %d\n", ret);
    }
  else
    {
      printf("Returned olddelta is %ld seconds and %ld micro seconds.\n",
             (long)olddelta.tv_sec, olddelta.tv_usec);
    }

  return ret;
}
