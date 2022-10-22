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
#include "foc_thr.h"
#include "foc_parseargs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define OPT_FKI     (SCHAR_MAX + 1)
#define OPT_FKP     (SCHAR_MAX + 2)

#define OPT_IRKI    (SCHAR_MAX + 3)
#define OPT_IRC     (SCHAR_MAX + 4)
#define OPT_IRS     (SCHAR_MAX + 5)
#define OPT_IIV     (SCHAR_MAX + 6)
#define OPT_IIS     (SCHAR_MAX + 7)

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
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
    { "irki", required_argument, 0, OPT_IRKI },
    { "irc", required_argument, 0, OPT_IRC },
    { "irs", required_argument, 0, OPT_IRS },
    { "iiv", required_argument, 0, OPT_IIV },
    { "iis", required_argument, 0, OPT_IIS },
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
  PRINTF("  [-t] run time (default: %d)\n",
         CONFIG_EXAMPLES_FOC_TIME_DEFAULT);
  PRINTF("  [-h] shows this message and exits\n");
  PRINTF("  [-f] FOC run mode (default: %d)\n",
         CONFIG_EXAMPLES_FOC_FMODE);
  PRINTF("       1 - IDLE mode\n");
  PRINTF("       2 - voltage mode\n");
  PRINTF("       3 - current mode\n");
  PRINTF("  [-m] controller mode (default: %d)\n",
         CONFIG_EXAMPLES_FOC_MMODE);
  PRINTF("       1 - torqe control\n");
  PRINTF("       2 - velocity control\n");
  PRINTF("       3 - position control\n");
#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
  PRINTF("       4 - align only\n");
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
  PRINTF("       5 - ident only\n");
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
  PRINTF("  [-r] torque [x1000]\n");
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  PRINTF("  [-v] velocity [x1000]\n");
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
  PRINTF("  [-x] position [x1000]\n");
#endif
  PRINTF("  [-s] motor state init (default: %d)\n",
         CONFIG_EXAMPLES_FOC_STATE_INIT);
  PRINTF("       1 - motor free\n");
  PRINTF("       2 - motor stop\n");
  PRINTF("       3 - motor CW\n");
  PRINTF("       4 - motor CCW\n");
  PRINTF("  [-j] enable specific instances\n");
#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  PRINTF("  [-o] openloop Vq/Iq setting [x1000] (default: %d)\n",
         CONFIG_EXAMPLES_FOC_OPENLOOP_Q);
#endif
#ifdef CONFIG_EXAMPLES_FOC_CONTROL_PI
  PRINTF("  [--fki] PI Kp coefficient [x1000] (default: %d)\n",
         CONFIG_EXAMPLES_FOC_IDQ_KP);
  PRINTF("  [--fkp] PI Ki coefficient [x1000] (default: %d)\n",
         CONFIG_EXAMPLES_FOC_IDQ_KI);
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
  PRINTF("  [--irki] res Ki coefficient [x1000] (default: %d)\n",
         CONFIG_EXAMPLES_FOC_IDENT_RES_KI);
  PRINTF("  [--irc] res current [x1000] (default: %d)\n",
         CONFIG_EXAMPLES_FOC_IDENT_RES_CURRENT);
  PRINTF("  [--irs] res sec (default: %d)\n",
         CONFIG_EXAMPLES_FOC_IDENT_RES_SEC);
  PRINTF("  [--iiv] ind voltage [x1000] (default: %d)\n",
         CONFIG_EXAMPLES_FOC_IDENT_IND_VOLTAGE);
  PRINTF("  [--iis] ind sec (default: %d)\n",
         CONFIG_EXAMPLES_FOC_IDENT_IND_SEC);
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
              args->cfg.foc_pi_kp = atoi(optarg);
              break;
            }

          case OPT_FKI:
            {
              args->cfg.foc_pi_ki = atoi(optarg);
              break;
            }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
          case OPT_IRKI:
            {
              args->cfg.ident_res_ki = atoi(optarg);
              break;
            }

          case OPT_IRC:
            {
              args->cfg.ident_res_curr = atoi(optarg);
              break;
            }

          case OPT_IRS:
            {
              args->cfg.ident_res_sec = atoi(optarg);
              break;
            }

          case OPT_IIV:
            {
              args->cfg.ident_ind_volt = atoi(optarg);
              break;
            }

          case OPT_IIS:
            {
              args->cfg.ident_ind_sec = atoi(optarg);
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
              args->cfg.fmode = atoi(optarg);
              break;
            }

          case 'm':
            {
              args->cfg.mmode = atoi(optarg);
              break;
            }

#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
          case 'r':
            {
              args->cfg.torqmax = atoi(optarg);
              break;
            }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
          case 'v':
            {
              args->cfg.velmax = atoi(optarg);
              break;
            }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
          case 'x':
            {
              args->cfg.posmax = atoi(optarg);
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
              args->cfg.qparam = atoi(optarg);
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

/****************************************************************************
 * Name: validate_args
 ****************************************************************************/

int validate_args(FAR struct args_s *args)
{
  int ret = -EINVAL;

#ifdef CONFIG_EXAMPLES_FOC_CONTROL_PI
  /* Current PI controller */

  if (args->cfg.foc_pi_kp == 0 && args->cfg.foc_pi_ki == 0)
    {
      PRINTF("ERROR: missing FOC Kp/Ki configuration\n");
      goto errout;
    }
#endif

  /* FOC operation mode */

  if (args->cfg.fmode != FOC_FMODE_IDLE &&
      args->cfg.fmode != FOC_FMODE_VOLTAGE &&
      args->cfg.fmode != FOC_FMODE_CURRENT)
    {
      PRINTF("Invalid op mode value %d s\n", args->cfg.fmode);
      goto errout;
    }

  /* Example control mode */

  if (
#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
    args->cfg.mmode != FOC_MMODE_TORQ &&
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
    args->cfg.mmode != FOC_MMODE_VEL &&
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
    args->cfg.mmode != FOC_MMODE_POS &&
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
    args->cfg.mmode != FOC_MMODE_ALIGN_ONLY &&
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
    args->cfg.mmode != FOC_MMODE_IDENT_ONLY &&
#endif
    1)
    {
      PRINTF("Invalid ctrl mode value %d s\n", args->cfg.mmode);
      goto errout;
    }

  /* Example state */

  if (args->state != FOC_EXAMPLE_STATE_FREE &&
      args->state != FOC_EXAMPLE_STATE_STOP &&
      args->state != FOC_EXAMPLE_STATE_CW &&
      args->state != FOC_EXAMPLE_STATE_CCW)
    {
      PRINTF("Invalid state value %d s\n", args->state);
      goto errout;
    }

  /* Time parameter */

  if (args->time <= 0 && args->time != -1)
    {
      PRINTF("Invalid time value %d s\n", args->time);
      goto errout;
    }

#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
  /* Motor identification parameters */

  if (args->cfg.ident_res_ki == 0 || args->cfg.ident_res_curr == 0 ||
      args->cfg.ident_res_sec == 0)
    {
      PRINTF("ERROR: missing motor res ident configuration\n");
      goto errout;
    }

  if (args->cfg.ident_ind_volt == 0 || args->cfg.ident_ind_sec == 0)
    {
      PRINTF("ERROR: missing motor ind ident configuration\n");
      goto errout;
    }
#endif

  /* Otherwise OK */

  ret = OK;

errout:
  return ret;
}
