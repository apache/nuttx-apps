/****************************************************************************
 * examples/smps/smps_main.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Mateusz Szafoni <raiden00@railab.me>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
#include <sys/boardctl.h>

#include <nuttx/fs/fs.h>
#include <nuttx/power/smps.h>

#if defined(CONFIG_EXAMPLES_SMPS)

#ifndef CONFIG_DRIVERS_SMPS
#  error "Smps example requires smps support"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_LIBC_FLOATINGPOINT
#  error "CONFIG_LIBC_FLOATINGPOINT must be set!"
#endif

#ifndef CONFIG_EXAMPLES_SMPS_TIME_DEFAULT
#  define CONFIG_EXAMPLES_SMPS_TIME_DEFAULT 10
#endif
#ifndef CONFIG_EXAMPLES_SMPS_OUT_VOLTAGE_DEFAULT
#  define CONFIG_EXAMPLES_SMPS_OUT_VOLTAGE_DEFAULT 0
#endif
#ifndef CONFIG_EXAMPLES_SMPS_OUT_CURRENT_DEFAULT
#  define CONFIG_EXAMPLES_SMPS_OUT_CURRENT_DEFAULT 0
#endif
#ifndef CONFIG_EXAMPLES_SMPS_OUT_POWER_DEFAULT
#  define CONFIG_EXAMPLES_SMPS_OUT_POWER_DEFAULT 0
#endif

#ifndef CONFIG_EXAMPLES_SMPS_OUT_VOLTAGE_LIMIT
#  define CONFIG_EXAMPLES_SMPS_OUT_VOLTAGE_LIMIT 0
#endif
#ifndef CONFIG_EXAMPLES_SMPS_OUT_CURRENT_LIMIT
#  define CONFIG_EXAMPLES_SMPS_OUT_CURRENT_LIMIT 0
#endif
#ifndef CONFIG_EXAMPLES_SMPS_OUT_POWER_LIMIT
#  define CONFIG_EXAMPLES_SMPS_OUT_POWER_LIMIT 0
#endif
#ifndef CONFIG_EXAMPLES_SMPS_IN_VOLTAGE_LIMIT
#  define CONFIG_EXAMPLES_SMPS_IN_VOLTAGE_LIMIT 0
#endif
#ifndef CONFIG_EXAMPLES_SMPS_IN_CURRENT_LIMIT
#  define CONFIG_EXAMPLES_SMPS_IN_CURRENT_LIMIT 0
#endif
#ifndef CONFIG_EXAMPLES_SMPS_IN_POWER_LIMIT
#  define CONFIG_EXAMPLES_SMPS_IN_POWER_LIMIT 0
#endif

/****************************************************************************
 * Private Type Definition
 ****************************************************************************/

/* Application arguments */

struct args_s
{
  int     time;                 /* Run time limit in sec, -1 if forever */
  float   current;              /* Output current for CC mode */
  float   voltage;              /* Output voltage for CV mode */
  float   power;                /* Output power for CP mode */
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
 * Name: smps_help
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
static void smps_help(FAR struct args_s *args)
{
  printf("Usage: smps [OPTIONS]\n\n");
  printf("  [-v voltage] output voltage in V\n");
  printf("       valid values from 0.0 to output voltage limit\n");
  printf("  [-c current] output current in A\n");
  printf("       valid values from 0.0 to output current limit\n");
  printf("  [-p power] output power in W\n");
  printf("       valid values from 0.0 to output power limit\n");
  printf("  [-t time] run time in seconds\n");
  printf("       valid values greather than 0 and\n");
  printf("       -1 for infinity [default]\n");
  printf("\n");
}
#endif

/****************************************************************************
 * Name: arg_string
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
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
#endif

/****************************************************************************
 * Name: arg_decimal
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
static int arg_decimal(FAR char **arg, FAR int *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = atoi(string);

  return ret;
}
#endif

/****************************************************************************
 * Name: arg_float
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
static int arg_float(FAR char **arg, FAR float *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = atof(string);

  return ret;
}
#endif

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
static void parse_args(FAR struct args_s *args, int argc, FAR char **argv)
{
  FAR char *ptr;
  float f_value;
  int i_value;
  int index;
  int nargs;

  for (index = 1; index < argc;)
    {
      ptr = argv[index];
      if (ptr[0] != '-')
        {
          printf("Invalid options format: %s\n", ptr);
          exit(0);
        }

      switch (ptr[1])
        {
            /* Get voltage */

          case 'v':
            {
              nargs = arg_float(&argv[index], &f_value);
              index += nargs;

              if (f_value <= 0.0 ||
                  (f_value * 1000 > CONFIG_EXAMPLES_SMPS_OUT_VOLTAGE_LIMIT))
                {
                  printf("Invalid voltage %.2f\n", f_value);
                  exit(1);
                }

              args->voltage = f_value;

              break;
            }

            /* Get current */

          case 'i':
            {
              nargs = arg_float(&argv[index], &f_value);
              index += nargs;

              if (f_value <= 0.0 ||
                  (f_value * 1000 > CONFIG_EXAMPLES_SMPS_OUT_CURRENT_LIMIT))
                {
                  printf("Invalid current %.2f\n", f_value);
                  exit(1);
                }

              args->current = f_value;

              break;
            }

            /* Get power */

          case 'p':
            {
              nargs = arg_float(&argv[index], &f_value);
              index += nargs;

              if (f_value <= 0.0 ||
                  (f_value * 1000 > CONFIG_EXAMPLES_SMPS_OUT_POWER_LIMIT))
                {
                  printf("Invalid power %.2f\n", f_value);
                  exit(1);
                }

              args->power = f_value;

              break;
            }

            /* Get time */

          case 't':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              if (i_value <= 0 && i_value != -1)
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
              smps_help(args);
              exit(0);
            }

          default:
            {
              printf("Unsupported option: %s\n", ptr);
              smps_help(args);
              exit(1);
            }
        }
    }
}
#endif

/****************************************************************************
 * Name: validate_args
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
static int validate_args(FAR struct args_s *args)
{
  int ret = OK;

  if (args->current < 0 ||
      args->current > (((float)CONFIG_EXAMPLES_SMPS_OUT_CURRENT_LIMIT)/1000.0))
    {
      printf("Not valid current value: %.2f\n", args->current);
      goto errout;
    }

  if (args->voltage < 0 ||
      args->voltage > (((float)CONFIG_EXAMPLES_SMPS_OUT_VOLTAGE_LIMIT)/1000.0))
    {
      printf("Not valid voltage value: %.2f\n", args->voltage);
      goto errout;
    }

  if (args->power < 0 ||
      args->power > (((float)CONFIG_EXAMPLES_SMPS_OUT_POWER_LIMIT)/1000.0))
    {
      printf("Not valid power value: %.2f\n", args->power);
      goto errout;
    }

errout:
  return ret;
}
#endif

/****************************************************************************
 * Name: feedback_print
 ****************************************************************************/

static void feedback_print(FAR struct smps_feedback_s *fb)
{
#ifdef CONFIG_SMPS_HAVE_INPUT_VOLTAGE
  printf("v_out: %.3f\t", fb->v_out);
#endif
#ifdef CONFIG_SMPS_HAVE_OUTPUT_VOLTAGE
  printf("v_in: %.3f\t", fb->v_in);
#endif
#ifdef CONFIG_SMPS_HAVE_OUTPUT_CURRENT
  printf("i_out: %.3f\t", fb->i_out);
#endif
#ifdef CONFIG_SMPS_HAVE_INPUT_CURRENT
  printf("i_in: %.3f\t", fb->i_in);
#endif
#ifdef CONFIG_SMPS_HAVE_OUTPUT_POWER
  printf("p_in: %.3f\t", fb->p_in);
#endif
#ifdef CONFIG_SMPS_HAVE_INPUT_POWER
  printf("p_out: %.3f\t", fb->p_out);
#endif
#ifdef CONFIG_SMPS_HAVE_EFFICIENCY
  printf("eff: %.3f\t", fb->eff);
#endif
  printf("\n");
}

static void print_info(struct smps_limits_s *limits, struct smps_params_s *params,
                       uint8_t *mode, struct args_s *args)
{
  printf("-------------------------------------\n");
  printf("Current SMPS settings:\n");
  printf("\n");

#if CONFIG_EXAMPLES_SMPS_OUT_VOLTAGE_LIMIT > 0
  printf("  Output voltage limit set to %.2f\n", limits->v_out);
#endif
#if CONFIG_EXAMPLES_SMPS_IN_VOLTAGE_LIMIT > 0
  printf("  Input voltage limit set to %.2f\n", limits->v_in);
#endif
#if CONFIG_EXAMPLES_SMPS_OUT_CURRENT_LIMIT > 0
  printf("  Output current limit set to %.2f\n", limits->i_out);
#endif
#if CONFIG_EXAMPLES_SMPS_IN_CURRENT_LIMIT > 0
  printf("  Input current limit set to %.2f\n", limits->i_in);
#endif
#if CONFIG_EXAMPLES_SMPS_OUT_POWER_LIMIT > 0
  printf("  Output power limit set to %.2f\n", limits->p_out);
#endif
#if CONFIG_EXAMPLES_SMPS_IN_POWER_LIMIT > 0
  printf("  Input power limit set to %.2f\n", limits->p_in);
#endif

  printf("\n");
  printf("  Demo time: %d sec\n", args->time);
  printf("  Mode set to %d\n", *mode);

  if (params->v_out > 0)
    {
      printf("  Output voltage set to %.2f\n", params->v_out);
    }

  if (params->i_out > 0)
    {
      printf("  Output current set to %.2f\n", params->i_out);
    }

  if (params->p_out > 0)
    {
      printf("  Output power set to %.2f\n", params->p_out);
    }

  printf("-------------------------------------\n\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: smps_main
 ****************************************************************************/

int smps_main(int argc, char *argv[])
{
  struct smps_limits_s smps_limits;
  struct smps_params_s smps_params;
  struct smps_state_s  smps_state;
  struct args_s *args = &g_args;
  uint8_t smps_mode;
  bool terminate;
  int time = 0;
  int ret = 0;
  int fd = 0;

  /* Initialize smps stuctures */

  memset(&smps_limits, 0, sizeof(struct smps_limits_s));
  memset(&smps_params, 0, sizeof(struct smps_params_s));
  memset(args, 0, sizeof(struct args_s));

  /* Initialize variables */

  terminate = false;

  /* Initialize SMPS limits */

#if CONFIG_EXAMPLES_SMPS_OUT_VOLTAGE_LIMIT > 0
  smps_limits.v_out = (float)CONFIG_EXAMPLES_SMPS_OUT_VOLTAGE_LIMIT/1000.0;
#endif
#if CONFIG_EXAMPLES_SMPS_IN_VOLTAGE_LIMIT > 0
  smps_limits.v_in  = (float)CONFIG_EXAMPLES_SMPS_IN_VOLTAGE_LIMIT/1000.0;
#endif
#if CONFIG_EXAMPLES_SMPS_OUT_CURRENT_LIMIT > 0
  smps_limits.i_out = (float)CONFIG_EXAMPLES_SMPS_OUT_CURRENT_LIMIT/1000.0;
#endif
#if CONFIG_EXAMPLES_SMPS_IN_CURRENT_LIMIT > 0
  smps_limits.i_in  = (float)CONFIG_EXAMPLES_SMPS_IN_CURRENT_LIMIT/1000.0;
#endif
#if CONFIG_EXAMPLES_SMPS_OUT_CURRENT_LIMIT > 0
  smps_limits.p_out = (float)CONFIG_EXAMPLES_SMPS_OUT_POWER_LIMIT/1000.0;
#endif
#if CONFIG_EXAMPLES_SMPS_IN_CURRENT_LIMIT > 0
  smps_limits.p_in  = (float)CONFIG_EXAMPLES_SMPS_IN_POWER_LIMIT/1000.0;
#endif

  /* Parse the command line */

#ifdef CONFIG_NSH_BUILTIN_APPS
  parse_args(args, argc, argv);
#endif

  /* Validate arguments */

#ifdef CONFIG_NSH_BUILTIN_APPS
  ret = validate_args(args);
  if (ret != OK)
    {
      printf("powerled_main: validate arguments failed!\n");
      goto errout;
    }
#endif

#ifndef CONFIG_NSH_BUILTIN_APPS
  /* Perform architecture-specific initialization (if configured) */

  (void)boardctl(BOARDIOC_INIT, 0);

#ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  (void)boardctl(BOARDIOC_FINALINIT, 0);
#endif
#endif

  /* Set SMPS mode */

  smps_mode = SMPS_OPMODE_CV;

  /* Set SMPS params */

  smps_params.v_out = (args->voltage > 0 ? args->voltage :
                       (float)CONFIG_EXAMPLES_SMPS_OUT_VOLTAGE_DEFAULT/1000.0);
  smps_params.i_out = (args->current > 0 ? args->current :
                       (float)CONFIG_EXAMPLES_SMPS_OUT_CURRENT_DEFAULT/1000.0);
  smps_params.p_out = (args->power > 0 ? args->power :
                       (float)CONFIG_EXAMPLES_SMPS_OUT_POWER_DEFAULT/1000.0);

  args->time = (args->time == 0 ? CONFIG_EXAMPLES_SMPS_TIME_DEFAULT : args->time);

  printf("\nStart smps_main application!\n\n");

  /* Print demo info */

  print_info(&smps_limits, &smps_params, &smps_mode, args);

  /* Open the SMPS driver */

  fd = open(CONFIG_EXAMPLES_SMPS_DEVPATH, 0);
  if (fd <= 0)
    {
      printf("smps_main: open %s failed %d\n", CONFIG_EXAMPLES_SMPS_DEVPATH, errno);
      goto errout;
    }

  /* Set SMPS limits */

  ret = ioctl(fd, PWRIOC_SET_LIMITS, (unsigned long)&smps_limits);
  if (ret != OK)
    {
      printf("IOCTL PWRIOC_LIMITS failed %d!\n", ret);
      goto errout;
    }

  /* Set SMPS mode */

  ret = ioctl(fd, PWRIOC_SET_MODE, (unsigned long)smps_mode);
  if (ret != OK)
    {
      printf("IOCTL PWRIOC_MODE failed %d!\n", ret);
      goto errout;
    }

  /* Set SMPS params */

  ret = ioctl(fd, PWRIOC_SET_PARAMS, (unsigned long)&smps_params);
  if (ret != OK)
    {
      printf("IOCTL PWRIOC_PARAMS failed %d!\n", ret);
      goto errout;
    }

  /* Start SMPS driver */

  ret = ioctl(fd, PWRIOC_START, (unsigned long)0);
  if (ret != OK)
    {
      printf("IOCTL PWRIOC_START failed %d!\n", ret);
      goto errout;
    }

  /* Main loop */

  while(terminate != true)
    {
      /* Get current SMPS state */

      ret = ioctl(fd, PWRIOC_GET_STATE, (unsigned long)&smps_state);
      if (ret < 0)
        {
          printf("Failed to get state %d \n", ret);
        }

      /* Terminate if fault state */

      if (smps_state.state > SMPS_STATE_RUN)
        {
          printf("Smps state = %d, fault = %d\n", smps_state.state,
                 smps_state.fault);
          terminate = true;
        }

      /* Print feedback state */

      if (time % 2 == 0)
        {
          feedback_print(&smps_state.fb);
        }

      /* Handle run time */

      if (terminate != true)
        {
          /* Wait 1 sec */

          sleep(1);

          if (args->time != -1)
            {
              time += 1;

              if (time >= args->time)
                {
                  /* Exit loop */

                  terminate = true;
                }
            }
        }
    }

errout:

  if (fd > 0)
    {
      printf("Stop smps driver\n");

      /* Stop SMPS driver */

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

#endif /* CONFIG_EXAMPLE_SMPS */
