/****************************************************************************
 * apps/examples/foc/foc_fixed16_thr.c
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
#include <fcntl.h>
#include <assert.h>

#include <dspb16.h>

#include "foc_mq.h"
#include "foc_thr.h"
#include "foc_cfg.h"
#include "foc_adc.h"

#include "foc_debug.h"

#include "industry/foc/foc_utils.h"
#include "industry/foc/foc_common.h"
#include "industry/foc/fixed16/foc_handler.h"
#include "industry/foc/fixed16/foc_ramp.h"
#include "industry/foc/fixed16/foc_angle.h"
#include "industry/foc/fixed16/foc_velocity.h"
#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
#  include "industry/foc/fixed16/foc_model.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_INDUSTRY_FOC_FIXED16
#  error
#endif

/****************************************************************************
 * Private Type Definition
 ****************************************************************************/

/* FOC motor data */

struct foc_motor_b16_s
{
  FAR struct foc_ctrl_env_s    *envp;         /* Thread env */
  bool                          fault;        /* Fault flag */
  bool                          startstop;    /* Start/stop request */
#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  bool                          openloop_now; /* Open-loop now */
  b16_t                         angle_ol;     /* Phase angle open-loop */
  foc_angle_b16_t               openloop;     /* Open-loop angle handler */
#endif
  int                           foc_mode;     /* FOC mode */
  b16_t                         vbus;         /* Power bus voltage */
  b16_t                         angle_now;    /* Phase angle now */
  b16_t                         vel_set;      /* Velocity setting now */
  b16_t                         vel_now;      /* Velocity now */
  b16_t                         vel_des;      /* Velocity destination */
  b16_t                         dir;          /* Motor's direction */
  b16_t                         per;          /* Controller period in seconds */
  b16_t                         iphase_adc;   /* Iphase ADC scaling factor */
  b16_t                         pwm_duty_max; /* PWM duty max */
  dq_frame_b16_t                dq_ref;       /* DQ reference */
  dq_frame_b16_t                vdq_comp;     /* DQ voltage compensation */
  foc_handler_b16_t             handler;      /* FOC controller */
  struct foc_mq_s               mq;           /* MQ data */
  struct foc_state_b16_s        foc_state;    /* FOC controller sate */
  struct foc_ramp_b16_s         ramp;         /* Velocity ramp data */
#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  struct foc_model_b16_s        model;         /* Model handler */
  struct foc_model_state_b16_s  model_state;   /* PMSM model state */
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_motor_init
 ****************************************************************************/

static int foc_motor_init(FAR struct foc_motor_b16_s *motor,
                          FAR struct foc_ctrl_env_s *envp)
{
#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  struct foc_openloop_cfg_b16_s ol_cfg;
#endif
  int                           ret = OK;

  DEBUGASSERT(motor);
  DEBUGASSERT(envp);

  /* Reset data */

  memset(motor, 0, sizeof(struct foc_motor_b16_s));

  /* Connect envp with motor handler */

  motor->envp = envp;

  /* Initialize motor data */

  motor->per        = b16divi(b16ONE, CONFIG_EXAMPLES_FOC_NOTIFIER_FREQ);
  motor->iphase_adc = ftob16((CONFIG_EXAMPLES_FOC_IPHASE_ADC) / 100000.0f);

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  /* Initialize open-loop angle handler */

  foc_angle_init_b16(&motor->openloop,
                     &g_foc_angle_ol_b16);

  /* Configure open-loop angle handler */

  ol_cfg.per = motor->per;
  foc_angle_cfg_b16(&motor->openloop, &ol_cfg);
#endif

  return ret;
}

/****************************************************************************
 * Name: foc_mode_init
 ****************************************************************************/

static int foc_mode_init(FAR struct foc_motor_b16_s *motor)
{
  int ret = OK;

  switch (motor->envp->mode)
    {
      case FOC_OPMODE_IDLE:
        {
          motor->foc_mode = FOC_HANDLER_MODE_IDLE;
          break;
        }

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
      case FOC_OPMODE_OL_V_VEL:
        {
          motor->foc_mode     = FOC_HANDLER_MODE_VOLTAGE;
          motor->openloop_now = true;
          break;
        }

      case FOC_OPMODE_OL_C_VEL:
        {
          motor->foc_mode     = FOC_HANDLER_MODE_CURRENT;
          motor->openloop_now = true;
          break;
        }
#endif

      default:
        {
          PRINTF("ERROR: unsupported op mode %d\n", motor->envp->mode);
          ret = -EINVAL;
          goto errout;
        }
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_motor_configure
 ****************************************************************************/

static int foc_motor_configure(FAR struct foc_motor_b16_s *motor)
{
#ifdef CONFIG_INDUSTRY_FOC_CONTROL_PI
  struct foc_initdata_b16_s ctrl_cfg;
#endif
#ifdef CONFIG_INDUSTRY_FOC_MODULATION_SVM3
  struct foc_mod_cfg_b16_s mod_cfg;
#endif
#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  struct foc_model_pmsm_cfg_b16_s pmsm_cfg;
#endif
  int              ret  = OK;

  DEBUGASSERT(motor);

  /* Initialize velocity ramp */

  ret = foc_ramp_init_b16(&motor->ramp,
                          motor->per,
                          ftob16(RAMP_CFG_THR),
                          ftob16(RAMP_CFG_ACC),
                          ftob16(RAMP_CFG_ACC));
  if (ret < 0)
    {
      PRINTF("ERROR: foc_ramp_init failed %d\n", ret);
      goto errout;
    }

  /* Initialize FOC handler */

  ret = foc_handler_init_b16(&motor->handler,
                             &g_foc_control_pi_b16,
                             &g_foc_mod_svm3_b16);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_handler_init failed %d\n", ret);
      goto errout;
    }

#ifdef CONFIG_INDUSTRY_FOC_CONTROL_PI
  /* Get PI controller configuration */

  ctrl_cfg.id_kp = ftob16(motor->envp->pi_kp / 1000.0f);
  ctrl_cfg.id_ki = ftob16(motor->envp->pi_ki / 1000.0f);
  ctrl_cfg.iq_kp = ftob16(motor->envp->pi_kp / 1000.0f);
  ctrl_cfg.iq_ki = ftob16(motor->envp->pi_ki / 1000.0f);
#endif

#ifdef CONFIG_INDUSTRY_FOC_MODULATION_SVM3
  /* Get SVM3 modulation configuration */

  mod_cfg.pwm_duty_max = motor->pwm_duty_max;
#endif

  /* Configure FOC handler */

  foc_handler_cfg_b16(&motor->handler, &ctrl_cfg, &mod_cfg);

#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  /* Initialize PMSM model */

  ret = foc_model_init_b16(&motor->model,
                           &g_foc_model_pmsm_ops_b16);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_model_init failed %d\n", ret);
      goto errout;
    }

  /* Get PMSM model configuration */

  pmsm_cfg.poles      = FOC_MODEL_POLES;
  pmsm_cfg.res        = ftob16(FOC_MODEL_RES);
  pmsm_cfg.ind        = ftob16(FOC_MODEL_IND);
  pmsm_cfg.iner       = ftob16(FOC_MODEL_INER);
  pmsm_cfg.flux_link  = ftob16(FOC_MODEL_FLUX);
  pmsm_cfg.ind_d      = ftob16(FOC_MODEL_INDD);
  pmsm_cfg.ind_q      = ftob16(FOC_MODEL_INDQ);
  pmsm_cfg.per        = motor->per;
  pmsm_cfg.iphase_adc = motor->iphase_adc;

  /* Configure PMSM model */

  foc_model_cfg_b16(&motor->model, &pmsm_cfg);
#endif

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_motor_start
 ****************************************************************************/

static int foc_motor_start(FAR struct foc_motor_b16_s *motor, bool start)
{
  int ret = OK;

  DEBUGASSERT(motor);

  if (start == true)
    {
      /* Start motor if VBUS data present */

      if (motor->mq.vbus > 0)
        {
          /* Configure motor controller */

          PRINTF("Configure motor %d!\n", motor->envp->id);

          ret = foc_motor_configure(motor);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_motor_configure failed %d!\n", ret);
              goto errout;
            }

          /* Start/stop FOC dev request */

          motor->startstop = true;
        }
    }
  else
    {
      /* Start/stop FOC dev request */

      motor->startstop = true;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_motor_deinit
 ****************************************************************************/

static int foc_motor_deinit(FAR struct foc_motor_b16_s *motor)
{
  int ret = OK;

  DEBUGASSERT(motor);

#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  /* Deinitialize PMSM model */

  ret = foc_model_deinit_b16(&motor->model);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_model_deinit failed %d\n", ret);
      goto errout;
    }
#endif

  /* Deinitialize FOC handler */

  ret = foc_handler_deinit_b16(&motor->handler);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_handler_deinit failed %d\n", ret);
      goto errout;
    }

  /* Reset data */

  memset(motor, 0, sizeof(struct foc_motor_b16_s));

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_motor_vbus
 ****************************************************************************/

static int foc_motor_vbus(FAR struct foc_motor_b16_s *motor, uint32_t vbus)
{
  DEBUGASSERT(motor);

  /* Update motor VBUS */

  motor->vbus = b16muli(vbus, ftob16(VBUS_ADC_SCALE));

  return OK;
}

/****************************************************************************
 * Name: foc_motor_vel
 ****************************************************************************/

static int foc_motor_vel(FAR struct foc_motor_b16_s *motor, uint32_t vel)
{
  b16_t tmp1 = 0;
  b16_t tmp2 = 0;

  DEBUGASSERT(motor);

  /* Update motor velocity destination
   * NOTE: velmax may not fit in b16_t so we can't use b16idiv()
   */

  tmp1 = itob16(motor->envp->velmax / 1000);
  tmp2 = b16mulb16(ftob16(SETPOINT_ADC_SCALE), tmp1);

  motor->vel_des = b16muli(tmp2, vel);

  return OK;
}

/****************************************************************************
 * Name: foc_motor_state
 ****************************************************************************/

static int foc_motor_state(FAR struct foc_motor_b16_s *motor, int state)
{
  int ret = OK;

  DEBUGASSERT(motor);

  /* Get open-loop currents
   * NOTE: Id always set to 0
   */

  motor->dq_ref.q = b16idiv(motor->envp->qparam, 1000);
  motor->dq_ref.d = 0;

  /* Update motor state */

  switch (state)
    {
      case FOC_EXAMPLE_STATE_FREE:
        {
          motor->vel_set = 0;
          motor->dir     = DIR_NONE_B16;

          /* Force DQ vector to zero */

          motor->dq_ref.q = 0;
          motor->dq_ref.d = 0;

          break;
        }

      case FOC_EXAMPLE_STATE_STOP:
        {
          motor->vel_set = 0;
          motor->dir     = DIR_NONE_B16;

          /* DQ vector not zero - active brake */

          break;
        }

      case FOC_EXAMPLE_STATE_CW:
        {
          motor->vel_set = 0;
          motor->dir     = DIR_CW_B16;

          break;
        }

      case FOC_EXAMPLE_STATE_CCW:
        {
          motor->vel_set = 0;
          motor->dir     = DIR_CCW_B16;

          break;
        }

      default:
        {
          ret = -EINVAL;
          goto errout;
        }
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_motor_get
 ****************************************************************************/

static int foc_motor_get(FAR struct foc_motor_b16_s *motor)
{
  struct foc_angle_in_b16_s  ain;
  struct foc_angle_out_b16_s aout;
  int                        ret = OK;

  DEBUGASSERT(motor);

  /* Update open-loop angle handler */

  ain.vel   = motor->vel_set;
  ain.angle = motor->angle_now;
  ain.dir   = motor->dir;

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  foc_angle_run_b16(&motor->openloop, &ain, &aout);

  /* Store open-loop angle */

  motor->angle_ol = aout.angle;

  /* Get phase angle now */

  if (motor->openloop_now == true)
    {
      motor->angle_now = motor->angle_ol;
    }
  else
#endif
    {
      /* TODO: get phase angle from observer or sensor */

      ASSERT(0);
    }

  return ret;
}

/****************************************************************************
 * Name: foc_motor_control
 ****************************************************************************/

static int foc_motor_control(FAR struct foc_motor_b16_s *motor)
{
  int ret = OK;

  DEBUGASSERT(motor);

  /* No velocity feedback - assume that velocity now is velocity set
   * TODO: velocity observer or sensor
   */

  motor->vel_now = motor->vel_set;

  /* Run velocity ramp controller */

  ret = foc_ramp_run_b16(&motor->ramp, motor->vel_des,
                         motor->vel_now, &motor->vel_set);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_ramp_run failed %d\n", ret);
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_handler_run
 ****************************************************************************/

static int foc_handler_run(FAR struct foc_motor_b16_s *motor,
                           FAR struct foc_device_s *dev)
{
  struct foc_handler_input_b16_s  input;
  struct foc_handler_output_b16_s output;
  b16_t                           current[CONFIG_MOTOR_FOC_PHASES];
  int                             ret   = OK;
  int                             i     = 0;

  DEBUGASSERT(motor);
  DEBUGASSERT(dev);

  /* FOC device fault */

  if (motor->fault == true)
    {
      /* Stop motor */

      motor->dq_ref.q   = 0;
      motor->dq_ref.d   = 0;
      motor->angle_now  = 0;
      motor->vbus       = 0;

      /* Force velocity to zero */

      motor->vel_des = 0;
    }

  /* Get real currents */

  for (i = 0; i < CONFIG_MOTOR_FOC_PHASES; i += 1)
    {
      current[i] = b16muli(motor->iphase_adc, dev->state.curr[i]);
    }

  /* Get input for FOC handler */

  input.current  = current;
  input.dq_ref   = &motor->dq_ref;
  input.vdq_comp = &motor->vdq_comp;
  input.angle    = motor->angle_now;
  input.vbus     = motor->vbus;
  input.mode     = motor->foc_mode;

  /* Run FOC controller */

  ret = foc_handler_run_b16(&motor->handler, &input, &output);

  /* Get duty from controller */

  for (i = 0; i < CONFIG_MOTOR_FOC_PHASES; i += 1)
    {
      dev->params.duty[i] = FOCDUTY_FROM_FIXED16(output.duty[i]);
    }

  /* Get FOC handler state */

  foc_handler_state_b16(&motor->handler, &motor->foc_state);

  return ret;
}

#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
/****************************************************************************
 * Name: foc_model_state_get
 ****************************************************************************/

static int foc_model_state_get(FAR struct foc_motor_b16_s *motor,
                               FAR struct foc_device_s *dev)
{
  int i = 0;

  DEBUGASSERT(motor);
  DEBUGASSERT(dev);

  /* Get model state */

  foc_model_state_b16(&motor->model, &motor->model_state);

  /* Get model currents */

  for (i = 0; i < CONFIG_MOTOR_FOC_PHASES; i += 1)
    {
      dev->state.curr[i] = motor->model_state.curr_raw[i];
    }

  return OK;
}
#endif

#ifdef FOC_STATE_PRINT_PRE
/****************************************************************************
 * Name: foc_state_print
 ****************************************************************************/

static int foc_state_print(FAR struct foc_motor_b16_s *motor)
{
  DEBUGASSERT(motor);

  PRINTF("b16 inst %d:\n", motor->envp->inst);

  foc_handler_state_print_b16(&motor->foc_state);

  return OK;
}
#endif

/****************************************************************************
 * Name: foc_motor_handle
 ****************************************************************************/

static int foc_motor_handle(FAR struct foc_motor_b16_s *motor,
                            FAR struct foc_mq_s *handle)
{
  int ret = OK;

  DEBUGASSERT(motor);
  DEBUGASSERT(handle);

  /* Terminate */

  if (handle->quit == true)
    {
      motor->mq.quit  = true;
    }

  /* Update motor VBUS */

  if (motor->mq.vbus != handle->vbus)
    {
      PRINTFV("Set vbus=%" PRIu32 " for FOC driver %d!\n",
              handle->vbus, motor->envp->id);

      ret = foc_motor_vbus(motor, handle->vbus);
      if (ret < 0)
        {
          PRINTF("ERROR: foc_motor_vbus failed %d!\n", ret);
          goto errout;
        }

      motor->mq.vbus = handle->vbus;
    }

  /* Update motor velocity destination */

  if (motor->mq.setpoint != handle->setpoint)
    {
      PRINTFV("Set setpoint=%" PRIu32 " for FOC driver %d!\n",
              handle->setpoint, motor->envp->id);

      ret = foc_motor_vel(motor, handle->setpoint);
      if (ret < 0)
        {
          PRINTF("ERROR: foc_motor_vel failed %d!\n", ret);
          goto errout;
        }

      motor->mq.setpoint = handle->setpoint;
    }

  /* Update motor state */

  if (motor->mq.app_state != handle->app_state)
    {
      PRINTFV("Set app_state=%d for FOC driver %d!\n",
              handle->app_state, motor->envp->id);

      ret = foc_motor_state(motor, handle->app_state);
      if (ret < 0)
        {
          PRINTF("ERROR: foc_motor_state failed %d!\n", ret);
          goto errout;
        }

      motor->mq.app_state = handle->app_state;
    }

  /* Start/stop controller */

  if (motor->mq.start != handle->start)
    {
      PRINTFV("Set start=%d for FOC driver %d!\n",
              handle->start, motor->envp->id);

      /* Start/stop motor controller */

      ret = foc_motor_start(motor, handle->start);
      if (ret < 0)
        {
          PRINTF("ERROR: foc_motor_start failed %d!\n", ret);
          goto errout;
        }

      motor->mq.start = handle->start;
    }

errout:
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_fixed16_thr
 ****************************************************************************/

int foc_fixed16_thr(FAR struct foc_ctrl_env_s *envp)
{
  struct foc_mq_s         handle;
  struct foc_motor_b16_s  motor;
  struct foc_device_s     dev;
  int                     time      = 0;
  int                     ret       = OK;

  DEBUGASSERT(envp);

  PRINTFV("foc_fixed_thr, id=%d\n", envp->id);

  /* Reset data */

  memset(&handle, 0, sizeof(struct foc_mq_s));

  /* Initialize motor controller */

  ret = foc_motor_init(&motor, envp);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_motor_init failed %d!\n", ret);
      goto errout;
    }

  /* Initialize FOC device as blocking */

  ret = foc_device_init(&dev, envp->id);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_device_init failed %d!\n", ret);
      goto errout;
    }

  /* Get PWM max duty */

  motor.pwm_duty_max = FOCDUTY_TO_FIXED16(dev.info.hw_cfg.pwm_max);

  /* Initialize controller mode */

  ret = foc_mode_init(&motor);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_mode_init failed %d!\n", ret);
      goto errout;
    }

  /* Start with motor free */

  handle.app_state = FOC_EXAMPLE_STATE_FREE;

  /* Wait some time */

  usleep(1000);

  /* Control loop */

  while (motor.mq.quit == false)
    {
      PRINTFV("foc_fixed16_thr %d %d\n", envp->id, time);

      /* Handle mqueue */

      ret = foc_mq_handle(envp->mqd, &handle);
      if (ret < 0)
        {
          PRINTF("ERROR: foc_mq_handle failed %d!\n", ret);
          goto errout;
        }

      /* Handle motor data */

      ret = foc_motor_handle(&motor, &handle);
      if (ret < 0)
        {
          PRINTF("ERROR: foc_motor_handle failed %d!\n", ret);
          goto errout;
        }

      if (motor.startstop == true)
        {
          /* Start or stop device */

          PRINTF("Start FOC device %d state=%d!\n",
                 motor.envp->id, motor.mq.start);

          ret = foc_device_start(&dev, motor.mq.start);
          if (ret < 0)
            {
              PRINTFV("ERROR: foc_device_start failed %d!\n", ret);
              goto errout;
            }

          motor.startstop = false;
        }

      /* Run control logic if controller started */

      if (motor.mq.start == true)
        {
          /* Get FOC device state */

          ret = foc_dev_state_get(&dev);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_dev_state_get failed %d!\n", ret);
              goto errout;
            }

#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
          /* Get model state */

          ret = foc_model_state_get(&motor, &dev);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_model_state_get failed %d!\n", ret);
              goto errout;
            }
#endif

          /* Handle controller state */

          ret = foc_dev_state_handle(&dev, &motor.fault);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_dev_state_handle failed %d!\n", ret);
              goto errout;
            }

          /* Get motor state */

          ret = foc_motor_get(&motor);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_motor_get failed %d!\n", ret);
              goto errout;
            }

          /* Motor control */

          ret = foc_motor_control(&motor);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_motor_control failed %d!\n", ret);
              goto errout;
            }

          /* Run FOC */

          ret = foc_handler_run(&motor, &dev);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_handler_run failed %d!\n", ret);
              goto errout;
            }

#ifdef FOC_STATE_PRINT_PRE
          /* Print state if configured */

          if (time % FOC_STATE_PRINT_PRE == 0)
            {
              foc_state_print(&motor);
            }
#endif

#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
          /* Feed FOC model with data */

          foc_model_run_b16(&motor.model,
                            ftob16(FOC_MODEL_LOAD),
                            &motor.foc_state.vab);
#endif

          /* Set FOC device parameters */

          ret = foc_dev_params_set(&dev);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_dev_params_set failed %d!\n", ret);
              goto errout;
            }
        }
      else
        {
          usleep(1000);
        }

      /* Increase counter */

      time += 1;
    }

errout:

  /* Deinit motor controller */

  ret = foc_motor_deinit(&motor);
  if (ret != OK)
    {
      PRINTF("ERROR: foc_motor_deinit failed %d!\n", ret);
    }

  PRINTF("Stop FOC device %d!\n", envp->id);

  /* De-initialize FOC device */

  ret = foc_device_deinit(&dev);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_device_deinit %d failed %d\n", envp->id, ret);
    }

  PRINTF("foc_fixed16_thr %d exit\n", envp->id);

  return ret;
}
