/****************************************************************************
 * apps/examples/powerled/powerled_main.c
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
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <debug.h>
#include <sys/ioctl.h>

#include <nuttx/fs/fs.h>

#include <nuttx/power/powerled.h>

#if defined(CONFIG_EXAMPLES_POWERLED)

#ifndef CONFIG_DRIVERS_POWERLED
#  error "Powerled example requires powerled support"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_POWERLED_CURRENT_LIMIT
#  error "LED current limit must be set!"
#endif

#ifndef CONFIG_LIBC_FLOATINGPOINT
#  error "CONFIG_LIBC_FLOATINGPOINT must be set!"
#endif

#define DEMO_CONT_BRIGHTNESS_MAX   100.0
#define DEMO_CONT_BRIGHTNESS_MIN   0.0
#define DEMO_CONT_BRIGHTNESS_STEP  10.0

#define DEMO_FLASH_FREQUENCY_STEP  0.5
#define DEMO_FLASH_FREQUENCY_MIN   1.0
#define DEMO_FLASH_FREQUENCY_MAX   5.0

#define DEMO_FLASH_DUTY_SET        50.0
#define DEMO_FLASH_BRIGHTNESS_SET  100.0

/****************************************************************************
 * Private Type Definition
 ****************************************************************************/

/* Example modes */

enum powerled_main_mode_e
{
  POWERLED_MAIN_DEMO = 0,
  POWERLED_MAIN_CONT,
  POWERLED_MAIN_FLASH
};

/* Application arguments */

struct args_s
{
  uint8_t mode;
  int     time;
  float   duty;
  float   brightness;
  float   frequency;
};

/****************************************************************************
 * Private Function Protototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct args_s g_args;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: powerled_help
 ****************************************************************************/

static void powerled_help(FAR struct args_s *args)
{
  printf("Usage: powerled [OPTIONS]\n\n");
  printf("  [-m mode] select mode\n");
  printf("       0 - demo mode [default]\n");
  printf("       1 - continuous mode\n");
  printf("       2 - flash mode\n");
  printf("  [-d duty] selects duty cycle for flash mode in %%\n");
  printf("       valid values from 0.0 to 100.0\n");
  printf("  [-b brightness] selects LED brightness in %%\n");
  printf("       valid values from 0.0 to 100.0\n");
  printf("  [-f frequency] selects frequency for flash mode in Hz\n");
  printf("       valid values greater than 0.0\n");
  printf("  [-t time] selects time for continuous and flash mode in s\n");
  printf("       valid values greater than 0 and\n");
  printf("       -1 for infinity [default]\n");
  printf("\n");
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

static int arg_decimal(FAR char **arg, FAR int *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = atoi(string);

  return ret;
}

/****************************************************************************
 * Name: arg_float
 ****************************************************************************/

static int arg_float(FAR char **arg, FAR float *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = atof(string);

  return ret;
}

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

static void parse_args(FAR struct args_s *args, int argc, FAR char **argv)
{
  FAR char *ptr;
  int index;
  int nargs;
  int i_value;
  float f_value;

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
            /* Get operation mode */

          case 'm':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              if ((i_value != POWERLED_MAIN_DEMO) &&
                  (i_value != POWERLED_MAIN_CONT) &&
                  (i_value != POWERLED_MAIN_FLASH))
                {
                  printf("Unsupported mode %d\n", i_value);
                  exit(1);
                }

              args->mode = i_value;

              break;
            }

            /* Get duty cycle */

          case 'd':
            {
              nargs = arg_float(&argv[index], &f_value);
              index += nargs;

              if (f_value <= 0.0 || f_value > 100.0)
                {
                  printf("Invalid duty value %.2f%%\n", f_value);
                  exit(1);
                }

              args->duty = f_value;

              break;
            }

            /* Get LED brightness */

          case 'b':
            {
              nargs = arg_float(&argv[index], &f_value);
              index += nargs;

              if (f_value <= 0.0 || f_value > 100.0)
                {
                  printf("Invalid brightness value %.2f%%\n", f_value);
                  exit(1);
                }

              args->brightness = f_value;

              break;
            }

            /* Get flashing frequency */

          case 'f':
            {
              nargs = arg_float(&argv[index], &f_value);
              index += nargs;

              if (f_value <= 0.0)
                {
                  printf("Invalid frequency value %.2f Hz\n", f_value);
                  exit(1);
                }

              args->frequency = f_value;

              break;
            }

            /* Get work time */

          case 't':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              if (i_value <= 0)
                {
                  printf("Invalid time value %d s\n", i_value);
                  exit(1);
                }

              args->time = i_value;

              break;
            }

            /* Print help message */

          case 'h':
            {
              powerled_help(args);
              exit(0);
            }

          default:
            {
              printf("Unsupported option: %s\n", ptr);
              powerled_help(args);
              exit(1);
            }
        }
    }
}

/****************************************************************************
 * Name: validate_args
 ****************************************************************************/

static int validate_args(FAR struct args_s *args)
{
  int ret = OK;

  /* Validate parameters for demo mode */

  if (args->mode == POWERLED_MAIN_DEMO)
    {
      printf("Powerled demo selected!\n");

      /* Demo mode does not use time parameter */

      args->time = -1;
    }

  /* Validate parameters for continuous mode */

  if (args->mode == POWERLED_MAIN_CONT)
    {
      printf("Powerled continuous mode selected!\n");
      printf("  brightnes: %.2f\n", args->brightness);
      printf("  time: %d\n", args->time);

      /* Needs brightness */

      if (args->brightness <= 0.0)
        {
          printf("Continuous mode needs brightness parameter!\n");

          ret = -EINVAL;
          goto errout;
        }
    }

  /* Validate parameters for flash mode */

  if (args->mode == POWERLED_MAIN_FLASH)
    {
      printf("Powerled flash mode selected!\n");
      printf("  brightness: %.2f\n", args->brightness);
      printf("  frequency: %.2f\n", args->frequency);
      printf("  duty: %.2f\n", args->duty);
      printf("  time: %d\n", args->time);

      /* Needs brightness */

      if (args->brightness <= 0.0)
        {
          printf("Flash mode needs brightness parameter!\n");

          ret = -EINVAL;
          goto errout;
        }

      /* Needs frequency */

      if (args->frequency <= 0.0)
        {
          printf("Flash mode needs frequency parameter!\n");

          ret = -EINVAL;
          goto errout;
        }

      /* Needs duty */

      if (args->duty <= 0.0)
        {
          printf("Flash mode needs duty parameter!\n");

          ret = -EINVAL;
          goto errout;
        }
    }

errout:
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: powerled_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct powerled_limits_s powerled_limits;
  struct powerled_params_s powerled_params;
  struct powerled_state_s  powerled_state;
  struct args_s *args = &g_args;
  uint8_t powerled_mode;
  uint8_t demo;
  bool terminate;
  bool config;
  int ret;
  int fd = -1;

  /* Initialize variables */

  g_args.mode       = POWERLED_MAIN_DEMO;
  g_args.time       = -1;
  g_args.duty       = 0.0;
  g_args.brightness = 0.0;
  g_args.frequency  = 0.0;

  demo      = POWERLED_OPMODE_CONTINUOUS;
  terminate = false;
  config    = true;

  /* Initialize powerled structures */

  memset(&powerled_limits, 0, sizeof(struct powerled_limits_s));
  memset(&powerled_params, 0, sizeof(struct powerled_params_s));

  /* Parse the command line */

  parse_args(args, argc, argv);

  /* Validate arguments */

  ret = validate_args(args);
  if (ret != OK)
    {
      printf("powerled_main: validate arguments failed!\n");
      goto errout;
    }

#ifndef CONFIG_NSH_ARCHINIT
  /* Perform architecture-specific initialization (if configured) */

  boardctl(BOARDIOC_INIT, 0);

#ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#endif
#endif

  /* Set LED current limit */

  powerled_limits.current =
          (((float)CONFIG_EXAMPLES_POWERLED_CURRENT_LIMIT) / 1000.0);

  printf("\nStart powerled_main application!\n");

  /* Open the Powerled driver */

  fd = open(CONFIG_EXAMPLES_POWERLED_DEVPATH, 0);
  if (fd <= 0)
    {
      printf("powerled_main: open %s failed %d\n",
             CONFIG_EXAMPLES_POWERLED_DEVPATH, errno);
      goto errout;
    }

  /* Set LED current limit */

  printf("Set max current: %.3f A\n", powerled_limits.current);

  ret = ioctl(fd, PWRIOC_SET_LIMITS, (unsigned long)&powerled_limits);
  if (ret < 0)
    {
      printf("failed to set limits %d. Force exit!\n", ret);
      goto errout;
    }

  /* Main loop */

  while (terminate != true)
    {
      switch (args->mode)
        {
          case POWERLED_MAIN_DEMO:
            {
              /* Continuous mode demo */

              if (demo == POWERLED_OPMODE_CONTINUOUS)
                {
                  if (config == true)
                    {
                      printf("\nConfigure continuous mode demo\n");

                      powerled_mode = POWERLED_OPMODE_CONTINUOUS;

                      /* Set Powerled continuous mode */

                      ret = ioctl(fd, PWRIOC_SET_MODE,
                                 (unsigned long)powerled_mode);
                      if (ret < 0)
                        {
                          printf("failed to set powerled mode %d\n", ret);
                        }

                      config = false;
                    }

                  printf("Brightness is %.2f\n", powerled_params.brightness);

                  /* Set Powerled parameters */

                  ret = ioctl(fd, PWRIOC_SET_PARAMS,
                             (unsigned long)&powerled_params);
                  if (ret < 0)
                    {
                      printf("failed to set params %d\n", ret);
                    }

                  /* Start Powerled */

                  ret = ioctl(fd, PWRIOC_START, (unsigned long)0);
                  if (ret != OK)
                    {
                      printf("IOCTL PWRIOC_START failed %d!\n", ret);
                    }

                  /* Increase brightness */

                  powerled_params.brightness += DEMO_CONT_BRIGHTNESS_STEP;

                  /* Change mode if max brightness */

                  if (powerled_params.brightness > DEMO_CONT_BRIGHTNESS_MAX)
                    {
                      demo = POWERLED_OPMODE_FLASH;
                      config = true;
                    }

                  /* Next step after 1 second */

                  sleep(1);
                }

              if (demo == POWERLED_OPMODE_FLASH)
                {
                  /* Flash mode demo */

                  if (config == true)
                    {
                      printf("\nConfigure flash mode demo\n");

                      powerled_params.brightness = DEMO_FLASH_BRIGHTNESS_SET;
                      powerled_params.duty       = DEMO_FLASH_DUTY_SET;
                      powerled_params.frequency  = DEMO_FLASH_FREQUENCY_MIN;

                      powerled_mode = POWERLED_OPMODE_FLASH;

                      /* Set Powerled flash mode */

                      ret = ioctl(fd, PWRIOC_SET_MODE,
                                 (unsigned long)powerled_mode);
                      if (ret < 0)
                        {
                          printf("failed to set powerled mode %d\n", ret);
                        }

                      printf("Brightness is %.2f\n",
                             powerled_params.brightness);
                      printf("Duty is %.2f\n", powerled_params.duty);

                      config = false;
                    }

                  printf("Frequency is %.2f\n", powerled_params.frequency);

                  /* Set Powerled parameters */

                  ret = ioctl(fd, PWRIOC_SET_PARAMS,
                             (unsigned long)&powerled_params);
                  if (ret < 0)
                    {
                      printf("failed to set params %d\n", ret);
                    }

                  /* Start Powerled */

                  ret = ioctl(fd, PWRIOC_START, (unsigned long)0);
                  if (ret != OK)
                    {
                      printf("IOCTL PWRIOC_START failed %d!\n", ret);
                    }

                  /* Increase flash frequency */

                  powerled_params.frequency += DEMO_FLASH_FREQUENCY_STEP;

                  /* Terinate demo if max frequency */

                  if (powerled_params.frequency > DEMO_FLASH_FREQUENCY_MAX)
                    {
                      terminate = true;
                      config    = false;
                    }

                  /* Next step after 1 second */

                  sleep(2);
                }

              break;
            }

          case POWERLED_MAIN_CONT:
            {
              /* Configure only once */

              if (config == true)
                {
                  printf("\nStart Continuous mode!\n");

                  powerled_mode = POWERLED_OPMODE_CONTINUOUS;

                  /* Set Powerled continuous mode */

                  ret = ioctl(fd, PWRIOC_SET_MODE,
                             (unsigned long)powerled_mode);
                  if (ret < 0)
                    {
                      printf("failed to set powerled mode %d\n", ret);
                    }

                  powerled_params.brightness = args->brightness;
                  powerled_params.duty       = 0;
                  powerled_params.frequency  = 0;

                  /* Set Powerled parameters */

                  ret = ioctl(fd, PWRIOC_SET_PARAMS,
                             (unsigned long)&powerled_params);
                  if (ret < 0)
                    {
                      printf("failed to set params %d\n", ret);
                    }

                  /* Start Powerled */

                  ret = ioctl(fd, PWRIOC_START, (unsigned long)0);
                  if (ret != OK)
                    {
                      printf("IOCTL PWRIOC_START failed %d!\n", ret);
                    }

                  config = false;
                }

              break;
            }

          case POWERLED_MAIN_FLASH:
            {
              /* Configure only once */

              if (config == true)
                {
                  printf("\nStart Flash mode!\n");

                  powerled_mode = POWERLED_OPMODE_FLASH;

                  /* Set Powerled flash mode */

                  ret = ioctl(fd, PWRIOC_SET_MODE,
                             (unsigned long)powerled_mode);
                  if (ret < 0)
                    {
                      printf("failed to set powerled mode %d\n", ret);
                    }

                  powerled_params.brightness = args->brightness;
                  powerled_params.duty       = args->duty;
                  powerled_params.frequency  = args->frequency;

                  /* Set Powerled parameters */

                  ret = ioctl(fd, PWRIOC_SET_PARAMS,
                             (unsigned long)&powerled_params);
                  if (ret < 0)
                    {
                      printf("failed to set params %d\n", ret);
                    }

                  /* Start Powerled */

                  ret = ioctl(fd, PWRIOC_START, (unsigned long)0);
                  if (ret < 0)
                    {
                      printf("IOCTL PWRIOC_START failed %d!\n", ret);
                    }

                  config = false;
                }

              break;
            }

          default:
            {
              printf("Unsupported powerled mode %d!\n", args->mode);
              goto errout;
            }
        }

      /* Get driver state */

      ret = ioctl(fd, PWRIOC_GET_STATE, (unsigned long)&powerled_state);
      if (ret < 0)
        {
          printf("Failed to get state %d\n", ret);
        }

      /* Terminate if fault state */

      if (powerled_state.state > POWERLED_STATE_RUN)
        {
          printf("Powerled state = %d, fault = %d\n", powerled_state.state,
                 powerled_state.fault);
          terminate = true;
        }

      if (args->time != -1 && terminate != true)
        {
          printf("wait %d s ...\n", args->time);

          /* Wait */

          sleep(args->time);

          /* Exit loop */

          terminate = true;
        }
    }

errout:

  fflush(stdout);
  fflush(stderr);

  if (fd > 0)
    {
      printf("Stop powerled driver\n");

      /* Stop powerled driver */

      ret = ioctl(fd, PWRIOC_STOP, (unsigned long)0);
      if (ret != OK)
        {
          printf("IOCTL PWRIOC_STOP failed %d!\n", ret);
        }

      /* Close file */

      close(fd);
    }

  return 0;
}

#endif /* CONFIG_EXAMPLE_POWERLED */
