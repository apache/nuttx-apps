/****************************************************************************
 * apps/examples/foc/foc_main.c
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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/boardctl.h>

#include "foc_mq.h"
#include "foc_thr.h"
#include "foc_cfg.h"
#include "foc_debug.h"
#include "foc_parseargs.h"
#include "foc_intf.h"

#include "industry/foc/foc_common.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Main loop sleep */

#define MAIN_LOOP_USLEEP (200000)

/* Enabled instances default state */

#define INST_EN_DEFAULT (0xff)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: init_args
 ****************************************************************************/

static void init_args(FAR struct args_s *args)
{
  args->time =
    (args->time == 0 ? CONFIG_EXAMPLES_FOC_TIME_DEFAULT : args->time);
  args->fmode =
    (args->fmode == 0 ? CONFIG_EXAMPLES_FOC_FMODE : args->fmode);
  args->mmode =
    (args->mmode == 0 ? CONFIG_EXAMPLES_FOC_MMODE : args->mmode);
#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  args->qparam =
    (args->qparam == 0 ? CONFIG_EXAMPLES_FOC_OPENLOOP_Q : args->qparam);
#endif

#ifdef CONFIG_EXAMPLES_FOC_CONTROL_PI
  args->foc_pi_kp =
    (args->foc_pi_kp == 0 ? CONFIG_EXAMPLES_FOC_IDQ_KP : args->foc_pi_kp);
  args->foc_pi_ki =
    (args->foc_pi_ki == 0 ? CONFIG_EXAMPLES_FOC_IDQ_KI : args->foc_pi_ki);
#endif

  /* Setpoint configuration */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
#ifdef CONFIG_EXAMPLES_FOC_SETPOINT_CONST
  args->torqmax =
    (args->torqmax == 0 ?
     CONFIG_EXAMPLES_FOC_SETPOINT_CONST_VALUE : args->torqmax);
#else
  args->torqmax =
    (args->torqmax == 0 ?
     CONFIG_EXAMPLES_FOC_SETPOINT_MAX : args->torqmax);
#endif
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
#ifdef CONFIG_EXAMPLES_FOC_SETPOINT_CONST
  args->velmax =
    (args->velmax == 0 ?
     CONFIG_EXAMPLES_FOC_SETPOINT_CONST_VALUE : args->velmax);
#else
  args->velmax =
    (args->velmax == 0 ?
     CONFIG_EXAMPLES_FOC_SETPOINT_MAX : args->velmax);
#endif
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
#ifdef CONFIG_EXAMPLES_FOC_SETPOINT_CONST
  args->posmax =
    (args->posmax == 0 ?
     CONFIG_EXAMPLES_FOC_SETPOINT_CONST_VALUE : args->posmax);
#else
  args->posmax =
    (args->posmax == 0 ?
     CONFIG_EXAMPLES_FOC_SETPOINT_MAX : args->posmax);
#endif
#endif

  args->state =
    (args->state == 0 ? CONFIG_EXAMPLES_FOC_STATE_INIT : args->state);
  args->en = (args->en == -1 ? INST_EN_DEFAULT : args->en);
}

/****************************************************************************
 * Name: validate_args
 ****************************************************************************/

static int validate_args(FAR struct args_s *args)
{
  int ret = -EINVAL;

#ifdef CONFIG_EXAMPLES_FOC_CONTROL_PI
  /* Current PI controller */

  if (args->foc_pi_kp == 0 && args->foc_pi_ki == 0)
    {
      PRINTF("ERROR: missing FOC Kp/Ki configuration\n");
      goto errout;
    }
#endif

  /* FOC operation mode */

  if (args->fmode != FOC_FMODE_IDLE &&
      args->fmode != FOC_FMODE_VOLTAGE &&
      args->fmode != FOC_FMODE_CURRENT)
    {
      PRINTF("Invalid op mode value %d s\n", args->fmode);
      goto errout;
    }

  /* Example control mode */

  if (
#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
    args->mmode != FOC_MMODE_TORQ &&
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
    args->mmode != FOC_MMODE_VEL &&
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
    args->mmode != FOC_MMODE_POS &&
#endif
    1)
    {
      PRINTF("Invalid ctrl mode value %d s\n", args->mmode);
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

  /* Otherwise OK */

  ret = OK;

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_mq_send
 ****************************************************************************/

static int foc_mq_send(mqd_t mqd, uint8_t msg, FAR void *data)
{
  int      ret = OK;
  uint8_t  buffer[5];
  uint32_t tmp = 0;

  DEBUGASSERT(data);

  /* Data max 4B */

  tmp = *((FAR uint32_t *) data);

  buffer[0] = msg;
  buffer[1] = ((tmp & 0x000000ff) >> 0);
  buffer[2] = ((tmp & 0x0000ff00) >> 8);
  buffer[3] = ((tmp & 0x00ff0000) >> 16);
  buffer[4] = ((tmp & 0xff000000) >> 24);

  ret = mq_send(mqd, (FAR char *)buffer, 5, 42);
  if (ret < 0)
    {
      PRINTF("foc_main: mq_send failed %d\n", errno);
      ret = -errno;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_vbus_send
 ****************************************************************************/

static int foc_vbus_send(mqd_t mqd, uint32_t vbus)
{
  return foc_mq_send(mqd, CONTROL_MQ_MSG_VBUS, (FAR void *)&vbus);
}

/****************************************************************************
 * Name: foc_setpoint_send
 ****************************************************************************/

static int foc_setpoint_send(mqd_t mqd, uint32_t setpoint)
{
  return foc_mq_send(mqd, CONTROL_MQ_MSG_SETPOINT, (FAR void *)&setpoint);
}

/****************************************************************************
 * Name: foc_state_send
 ****************************************************************************/

static int foc_state_send(mqd_t mqd, uint32_t state)
{
  return foc_mq_send(mqd, CONTROL_MQ_MSG_APPSTATE, (FAR void *)&state);
}

/****************************************************************************
 * Name: foc_start_send
 ****************************************************************************/

static int foc_start_send(mqd_t mqd)
{
  int tmp = 0;
  return foc_mq_send(mqd, CONTROL_MQ_MSG_START, (FAR void *)&tmp);
}

/****************************************************************************
 * Name: foc_kill_send
 ****************************************************************************/

static int foc_kill_send(mqd_t mqd)
{
  int tmp = 0;
  return foc_mq_send(mqd, CONTROL_MQ_MSG_KILL, (FAR void *)&tmp);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct foc_ctrl_env_s  foc[CONFIG_MOTOR_FOC_INST];
  pthread_t              threads[CONFIG_MOTOR_FOC_INST];
  mqd_t                  mqd[CONFIG_MOTOR_FOC_INST];
  struct args_s          args;
  struct foc_intf_data_s data;
  int                    ret          = OK;
  int                    i            = 0;
  int                    time         = 0;

  /* Reset some data */

  memset(&args, 0, sizeof(struct args_s));
  memset(mqd, 0, sizeof(mqd_t) * CONFIG_MOTOR_FOC_INST);
  memset(foc, 0, sizeof(struct foc_ctrl_env_s) * CONFIG_MOTOR_FOC_INST);
  memset(threads, 0, sizeof(pthread_t) * CONFIG_MOTOR_FOC_INST);
  memset(&data, 0, sizeof(struct foc_intf_data_s));

  /* Initialize args before parse */

  args.en = -1;

#ifdef CONFIG_BUILTIN
  /* Parse the command line */

  parse_args(&args, argc, argv);
#endif

  /* Initialize args */

  init_args(&args);

  /* Validate arguments */

  ret = validate_args(&args);
  if (ret < 0)
    {
      PRINTF("ERROR: validate args failed\n");
      goto errout_no_mutex;
    }

#ifndef CONFIG_NSH_ARCHINIT
  /* Perform architecture-specific initialization (if configured) */

  boardctl(BOARDIOC_INIT, 0);

#  ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#  endif
#endif

  PRINTF("\nStart foc_main application!\n\n");

  /* Initialize threads */

  ret = foc_threads_init();
  if (ret < 0)
    {
      PRINTF("ERROR: failed to initialize threads %d\n", ret);
      goto errout_no_mutex;
    }

  /* Initialize control interface */

  ret = foc_intf_init();
  if (ret < 0)
    {
      PRINTF("ERROR: failed to initialize control interface %d\n", ret);
      goto errout;
    }

  /* Initialzie FOC controllers */

  for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
    {
      /* Get configuration */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
      foc[i].qparam   = args.qparam;
#endif
      foc[i].fmode    = args.fmode;
      foc[i].mmode    = args.mmode;
#ifdef CONFIG_EXAMPLES_FOC_CONTROL_PI
      foc[i].foc_pi_kp = args.foc_pi_kp;
      foc[i].foc_pi_ki = args.foc_pi_ki;
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
      foc[i].torqmax  = args.torqmax;
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
      foc[i].velmax   = args.velmax;
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
      foc[i].posmax   = args.posmax;
#endif

      if (args.en & (1 << i))
        {
          /* Initialize controller thread if enabled */

          ret = foc_ctrlthr_init(&foc[i], i, &mqd[i], &threads[i]);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_ctrlthr_init failed %d!\n", ret);
              goto errout;
            }
        }
    }

  /* Wait some time to finish all controllers initialziation */

  usleep(10000);

  /* Initial update for VBUS and SETPOINT */

#ifndef CONFIG_EXAMPLES_FOC_VBUS_ADC
  data.vbus_update  = true;
  data.vbus_raw     = VBUS_CONST_VALUE;
#endif
#ifndef CONFIG_EXAMPLES_FOC_SETPOINT_ADC
  data.sp_update    = true;
  data.sp_raw       = 1;
#endif
  data.state_update = true;

  /* Controller state */

  data.state = args.state;

  /* Auxliary control loop */

  while (data.terminate != true)
    {
      PRINTFV("foc_main loop %d\n", time);

      /* Update control interface */

      ret = foc_intf_update(&data);
      if (ret < 0)
        {
          PRINTF("ERROR: foc_intf_update failed: %d\n", ret);
          goto errout;
        }

      /* 1. Update VBUS */

      if (data.vbus_update == true)
        {
          for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
            {
              if (args.en & (1 << i))
                {
                  PRINTFV("Send vbus to %d\n", i);

                  /* Send VBUS to thread */

                  ret = foc_vbus_send(mqd[i], data.vbus_raw);
                  if (ret < 0)
                    {
                      PRINTF("ERROR: foc_vbus_send failed %d\n", ret);
                      goto errout;
                    }
                }
            }

          /* Reset flag */

          data.vbus_update = false;
        }

      /* 2. Update motor state */

      if (data.state_update == true)
        {
          for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
            {
              if (args.en & (1 << i))
                {
                  PRINTFV("Send state %" PRIu32 " to %d\n", data.state, i);

                  /* Send STATE to thread */

                  ret = foc_state_send(mqd[i], data.state);
                  if (ret < 0)
                    {
                      PRINTF("ERROR: foc_state_send failed %d\n", ret);
                      goto errout;
                    }
                }
            }

          /* Reset flag */

          data.state_update = false;
        }

      /* 3. Update motor velocity */

      if (data.sp_update == true)
        {
          for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
            {
              if (args.en & (1 << i))
                {
                  PRINTFV("Send setpoint = %" PRIu32 "to %d\n",
                          data.sp_raw, i);

                  /* Send setpoint to threads */

                  ret = foc_setpoint_send(mqd[i], data.sp_raw);
                  if (ret < 0)
                    {
                      PRINTF("ERROR: foc_setpoint_send failed %d\n", ret);
                      goto errout;
                    }
                }
            }

          /* Reset flag */

          data.sp_update = false;
        }

      /* 4. One time start */

      if (data.started == false)
        {
          for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
            {
              if (args.en & (1 << i))
                {
                  PRINTFV("Send start to %d\n", i);

                  /* Send START to threads */

                  ret = foc_start_send(mqd[i]);
                  if (ret < 0)
                    {
                      PRINTF("ERROR: foc_start_send failed %d\n", ret);
                      goto errout;
                    }
                }
            }

          /* Set flag */

          data.started = true;
        }

      /* Handle run time */

      time += 1;

      if (args.time != -1)
        {
          if (time >= (args.time * (1000000 / MAIN_LOOP_USLEEP)))
            {
              /* Exit loop */

              data.terminate = true;
            }
        }

      /* Terminate main loop if threads terminated */

      if (foc_threads_terminated() == true)
        {
          data.terminate = true;
        }

      usleep(MAIN_LOOP_USLEEP);
    }

errout:

  /* Stop FOC control threads */

  for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
    {
      if (args.en & (1 << i))
        {
          if (mqd[i] != (mqd_t)-1)
            {
              /* Stop thread message */

              ret = foc_kill_send(mqd[i]);
              if (ret < 0)
                {
                  PRINTF("ERROR: foc_kill_send failed %d\n", ret);
                }
            }
        }
    }

  /* Wait for threads termination */

  while (foc_threads_terminated() == false)
    {
      usleep(100000);
    }

  /* De-initialize all mq */

  for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
    {
      if (args.en & (1 << i))
        {
          /* Close FOC control thread queue */

          if (mqd[i] != (mqd_t)-1)
            {
              mq_close(mqd[i]);
            }
        }
    }

  /* De-initialize control interface */

  ret = foc_intf_deinit();
  if (ret < 0)
    {
      PRINTF("ERROR: foc_inf_deinit failed %d\n", ret);
      goto errout;
    }

errout_no_mutex:

  foc_threads_deinit();

  PRINTF("foc_main exit\n");

  return 0;
}
