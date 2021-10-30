/****************************************************************************
 * apps/examples/foc/foc_parseargs.c
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

#include <unistd.h>
#include <stdlib.h>

#include "foc_debug.h"
#include "foc_parseargs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_help
 ****************************************************************************/

static void foc_help(void)
{
  PRINTF("Usage: foc [OPTIONS]\n");
  PRINTF("  [-t] run time\n");
  PRINTF("  [-h] shows this message and exits\n");
  PRINTF("  [-m] controller mode\n");
  PRINTF("       1 - IDLE mode\n");
  PRINTF("       2 - voltage open-loop velocity \n");
  PRINTF("       3 - current open-loop velocity \n");
  PRINTF("  [-o] openloop Vq/Iq setting [x1000]\n");
  PRINTF("  [-i] PI Ki coefficient [x1000]\n");
  PRINTF("  [-p] KI Kp coefficient [x1000]\n");
  PRINTF("  [-v] velocity [x1000]\n");
  PRINTF("  [-s] motor state\n");
  PRINTF("       1 - motor free\n");
  PRINTF("       2 - motor stop\n");
  PRINTF("       3 - motor CW\n");
  PRINTF("       4 - motor CCW\n");
  PRINTF("  [-j] enable specific instnaces\n");
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
  int       ret;

  ret = arg_string(arg, &string);
  *value = atoi(string);

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

void parse_args(FAR struct args_s *args, int argc, FAR char **argv)
{
  FAR char *ptr;
  int       index;
  int       nargs;
  int       i_value;

  for (index = 1; index < argc; )
    {
      ptr = argv[index];
      if (ptr[0] != '-')
        {
          PRINTF("Invalid options format: %s\n", ptr);
          exit(0);
        }

      switch (ptr[1])
        {
          /* Get time */

          case 't':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->time = i_value;
              break;
            }

          case 'm':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->mode = i_value;
              break;
            }

          case 'o':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->qparam = i_value;
              break;
            }

          case 'p':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->pi_kp = i_value;
              break;
            }

          case 'i':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->pi_ki = i_value;
              break;
            }

          case 'v':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->velmax = i_value;
              break;
            }

          case 's':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->state = i_value;
              break;
            }

          case 'j':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->en = i_value;
              break;
            }

          case 'h':
            {
              foc_help();
              exit(0);
            }

          default:
            {
              PRINTF("Unsupported option: %s\n", ptr);
              foc_help();
              exit(1);
            }
        }
    }
}
