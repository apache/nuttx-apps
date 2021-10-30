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
#include <getopt.h>

#include "foc_debug.h"
#include "foc_parseargs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define OPT_FKI     (SCHAR_MAX + 1)
#define OPT_FKP     (SCHAR_MAX + 2)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct option g_long_options[] =
  {
    { "time", required_argument, 0, 't' },
    { "help", no_argument, 0, 'h' },
    { "mode", required_argument, 0, 'm' },
    { "vel", required_argument, 0, 'v' },
    { "state", required_argument, 0, 's' },
    { "en", required_argument, 0, 'j' },
    { "oqset", required_argument, 0, 'o' },
    { "fkp", required_argument, 0, OPT_FKP },
    { "fki", required_argument, 0, OPT_FKI },
    { 0, 0, 0, 0 }
  };

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
  PRINTF("  [-v] velocity [x1000]\n");
  PRINTF("  [-s] motor state\n");
  PRINTF("       1 - motor free\n");
  PRINTF("       2 - motor stop\n");
  PRINTF("       3 - motor CW\n");
  PRINTF("       4 - motor CCW\n");
  PRINTF("  [-j] enable specific instnaces\n");
#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  PRINTF("  [-o] openloop Vq/Iq setting [x1000]\n");
#endif
  PRINTF("  [--fki] PI Kp coefficient [x1000]\n");
  PRINTF("  [--fkp] PI Ki coefficient [x1000]\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

void parse_args(FAR struct args_s *args, int argc, FAR char **argv)
{
  int option_index = 0;
  int c = 0;

  while (1)
    {
      c = getopt_long(argc, argv, "ht:m:o:v:s:j:", g_long_options,
                      &option_index);

      if (c == -1)
        {
          break;
        }

      switch (c)
        {
          case OPT_FKP:
            {
              args->pi_kp = atoi(optarg);
              break;
            }

          case OPT_FKI:
            {
              args->pi_ki = atoi(optarg);
              break;
            }

          case 't':
            {
              args->time = atoi(optarg);
              break;
            }

          case 'h':
            {
              foc_help();
              exit(0);
            }

          case 'm':
            {
              args->mode = atoi(optarg);
              break;
            }

          case 'v':
            {
              args->velmax = atoi(optarg);
              break;
            }

          case 's':
            {
              args->state = atoi(optarg);
              break;
            }

          case 'j':
            {
              args->en = atoi(optarg);
              break;
            }

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
          case 'o':
            {
              args->qparam = atoi(optarg);
              break;
            }
#endif

          case '?':
          default:
            {
              if (argv[optind - 1] == NULL)
                {
                  PRINTF("ERROR: invalid option argument for %s\n",
                         argv[optind - 2]);
                }
              else
                {
                  PRINTF("ERROR: invalid option %s\n", argv[optind - 1]);
                }

              foc_help();
              exit(1);
            }
        }
    }
}
