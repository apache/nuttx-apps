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
    { "fmode", required_argument, 0, 'f' },
    { "mmode", required_argument, 0, 'm' },
#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
    { "torq", required_argument, 0, 'r' },
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
    { "vel", required_argument, 0, 'v' },
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
    { "pos", required_argument, 0, 'x' },
#endif
    { "state", required_argument, 0, 's' },
    { "en", required_argument, 0, 'j' },
#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
    { "oqset", required_argument, 0, 'o' },
#endif
#ifdef CONFIG_EXAMPLES_FOC_CONTROL_PI
    { "fkp", required_argument, 0, OPT_FKP },
    { "fki", required_argument, 0, OPT_FKI },
#endif
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
  PRINTF("  [-f] FOC run mode\n");
  PRINTF("       1 - IDLE mode\n");
  PRINTF("       2 - voltage mode\n");
  PRINTF("       3 - current mode\n");
  PRINTF("  [-m] controller mode\n");
  PRINTF("       1 - torqe control\n");
  PRINTF("       2 - velocity control\n");
  PRINTF("       3 - position control\n");
#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
  PRINTF("  [-r] torque [x1000]\n");
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  PRINTF("  [-v] velocity [x1000]\n");
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
  PRINTF("  [-x] position [x1000]\n");
#endif
  PRINTF("  [-s] motor state\n");
  PRINTF("       1 - motor free\n");
  PRINTF("       2 - motor stop\n");
  PRINTF("       3 - motor CW\n");
  PRINTF("       4 - motor CCW\n");
  PRINTF("  [-j] enable specific instances\n");
#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  PRINTF("  [-o] openloop Vq/Iq setting [x1000]\n");
#endif
#ifdef CONFIG_EXAMPLES_FOC_CONTROL_PI
  PRINTF("  [--fki] PI Kp coefficient [x1000]\n");
  PRINTF("  [--fkp] PI Ki coefficient [x1000]\n");
#endif
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
      c = getopt_long(argc, argv, "ht:f:m:o:r:v:x:s:j:", g_long_options,
                      &option_index);

      if (c == -1)
        {
          break;
        }

      switch (c)
        {
#ifdef CONFIG_EXAMPLES_FOC_CONTROL_PI
          case OPT_FKP:
            {
              args->foc_pi_kp = atoi(optarg);
              break;
            }

          case OPT_FKI:
            {
              args->foc_pi_ki = atoi(optarg);
              break;
            }
#endif

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

          case 'f':
            {
              args->fmode = atoi(optarg);
              break;
            }

          case 'm':
            {
              args->mmode = atoi(optarg);
              break;
            }

#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
          case 'r':
            {
              args->torqmax = atoi(optarg);
              break;
            }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
          case 'v':
            {
              args->velmax = atoi(optarg);
              break;
            }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
          case 'x':
            {
              args->posmax = atoi(optarg);
              break;
            }
#endif

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
