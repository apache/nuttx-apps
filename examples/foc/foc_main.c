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
 * Private Data
 ****************************************************************************/

struct args_s g_args =
{
  .time  = CONFIG_EXAMPLES_FOC_TIME_DEFAULT,
  .state = CONFIG_EXAMPLES_FOC_STATE_INIT,
  .en    = INST_EN_DEFAULT,
  .cfg =
  {
    .fmode = CONFIG_EXAMPLES_FOC_FMODE,
    .mmode = CONFIG_EXAMPLES_FOC_MMODE,
#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
    .qparam = CONFIG_EXAMPLES_FOC_OPENLOOP_Q,
#endif
#ifdef CONFIG_EXAMPLES_FOC_CONTROL_PI
    .foc_pi_kp = CONFIG_EXAMPLES_FOC_IDQ_KP,
    .foc_pi_ki = CONFIG_EXAMPLES_FOC_IDQ_KI,
#endif
#ifdef CONFIG_EXAMPLES_FOC_SETPOINT_CONST
#  ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
    .torqmax = CONFIG_EXAMPLES_FOC_SETPOINT_CONST_VALUE,
#  endif
#  ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
    .velmax = CONFIG_EXAMPLES_FOC_SETPOINT_CONST_VALUE,
#  endif
#  ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
    .posmax = CONFIG_EXAMPLES_FOC_SETPOINT_CONST_VALUE,
#  endif
#else
#  ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
    .torqmax = CONFIG_EXAMPLES_FOC_SETPOINT_MAX,
#  endif
#  ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
    .velmax = CONFIG_EXAMPLES_FOC_SETPOINT_MAX,
#  endif
#  ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
    .posmax = CONFIG_EXAMPLES_FOC_SETPOINT_MAX,
#  endif
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
    .ident_res_ki = CONFIG_EXAMPLES_FOC_IDENT_RES_KI,
    .ident_res_curr = CONFIG_EXAMPLES_FOC_IDENT_RES_CURRENT,
    .ident_res_sec = CONFIG_EXAMPLES_FOC_IDENT_RES_SEC,
    .ident_ind_volt = CONFIG_EXAMPLES_FOC_IDENT_IND_VOLTAGE,
    .ident_ind_sec = CONFIG_EXAMPLES_FOC_IDENT_IND_SEC,
#endif
  }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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

  tmp = *((FAR uint32_t *)data);

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
  struct foc_intf_data_s data;
  int                    ret          = OK;
  int                    i            = 0;
  int                    time         = 0;

  /* Reset some data */

  memset(mqd, 0, sizeof(mqd_t) * CONFIG_MOTOR_FOC_INST);
  memset(foc, 0, sizeof(struct foc_ctrl_env_s) * CONFIG_MOTOR_FOC_INST);
  memset(threads, 0, sizeof(pthread_t) * CONFIG_MOTOR_FOC_INST);
  memset(&data, 0, sizeof(struct foc_intf_data_s));

#ifdef CONFIG_BUILTIN
  /* Parse the command line */

  parse_args(&g_args, argc, argv);
#endif

  /* Validate arguments */

  ret = validate_args(&g_args);
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

      foc[i].cfg = &g_args.cfg;

      if (g_args.en & (1 << i))
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

  data.state = g_args.state;

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
              if (g_args.en & (1 << i))
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
              if (g_args.en & (1 << i))
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
              if (g_args.en & (1 << i))
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
              if (g_args.en & (1 << i))
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

      if (g_args.time != -1)
        {
          if (time >= (g_args.time * (1000000 / MAIN_LOOP_USLEEP)))
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
      if (g_args.en & (1 << i))
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
      if (g_args.en & (1 << i))
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
