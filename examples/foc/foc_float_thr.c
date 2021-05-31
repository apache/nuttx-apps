/****************************************************************************
 * apps/examples/foc/foc_float_thr.c
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

#include <dsp.h>

#include "foc_mq.h"
#include "foc_thr.h"
#include "foc_cfg.h"
#include "foc_adc.h"

#include "foc_debug.h"

#include "industry/foc/foc_utils.h"
#include "industry/foc/foc_common.h"
#include "industry/foc/float/foc_handler.h"
#include "industry/foc/float/foc_ramp.h"
#include "industry/foc/float/foc_angle.h"
#include "industry/foc/float/foc_velocity.h"
#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
#  include "industry/foc/float/foc_model.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_INDUSTRY_FOC_FLOAT
#  error
#endif

#ifndef CONFIG_INDUSTRY_FOC_ANGLE_OPENLOOP
#  error For now only open-loop supported
#endif

/****************************************************************************
 * Private Type Definition
 ****************************************************************************/

/* FOC motor data */

struct foc_motor_f32_s
{
  FAR struct foc_ctrl_env_s    *envp;         /* Thread env */
  bool                          fault;        /* Fault flag */
#ifdef CONFIG_INDUSTRY_FOC_ANGLE_OPENLOOP
  bool                          openloop_now; /* Open-loop now */
#endif
  int                           foc_mode;     /* FOC mode */
  float                         vbus;         /* Power bus voltage */
  float                         angle_now;    /* Phase angle now */
  float                         angle_ol;     /* Phase angle open-loop */
  float                         vel_set;      /* Velocity setting now */
  float                         vel_now;      /* Velocity now */
  float                         vel_des;      /* Velocity destination */
  float                         dir;          /* Motor's direction */
  float                         per;          /* Controller period in seconds */
  float                         iphase_adc;   /* Iphase ADC scaling factor */
  dq_frame_f32_t                dq_ref;       /* DQ reference */
  dq_frame_f32_t                vdq_comp;     /* DQ voltage compensation */
  foc_handler_f32_t             handler;      /* FOC controller */
  struct foc_mq_s               mq;           /* MQ data */
  struct foc_info_s             info;         /* Device info */
  struct foc_state_f32_s        foc_state;    /* FOC controller sate */
  struct foc_state_s            dev_state;    /* FOC dev state */
  struct foc_params_s           dev_params;   /* FOC dev params */
  struct foc_ramp_f32_s         ramp;         /* Velocity ramp data */
#ifdef CONFIG_INDUSTRY_FOC_ANGLE_OPENLOOP
  foc_angle_f32_t               openloop;     /* Open-loop angle handler */
#endif
#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  struct foc_model_f32_s        model;         /* Model handler */
  struct foc_model_state_f32_s  model_state;   /* PMSM model state */
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_motor_init
 ****************************************************************************/

static int foc_motor_init(FAR struct foc_motor_f32_s *motor,
                          FAR struct foc_ctrl_env_s *envp)
{
#ifdef CONFIG_INDUSTRY_FOC_ANGLE_OPENLOOP
  struct foc_openloop_cfg_f32_s ol_cfg;
#endif
  int                           ret = OK;

  DEBUGASSERT(motor);
  DEBUGASSERT(envp);

  /* Reset data */

  memset(motor, 0, sizeof(struct foc_motor_f32_s));

  /* Connect envp with motor handler */

  motor->envp = envp;

  /* Get device info */

  ret = foc_dev_getinfo(envp->dev.fd, &motor->info);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_dev_getinfo failed %d!\n", ret);
      goto errout;
    }

  /* Initialize motor data */

  motor->per        = (float)(1.0f / CONFIG_EXAMPLES_FOC_NOTIFIER_FREQ);
  motor->iphase_adc = ((CONFIG_EXAMPLES_FOC_IPHASE_ADC) / 100000.0f);

#ifdef CONFIG_INDUSTRY_FOC_ANGLE_OPENLOOP
  /* Initialize open-loop angle handler */

  foc_angle_init_f32(&motor->openloop,
                     &g_foc_angle_ol_f32);

  /* Configure open-loop angle handler */

  ol_cfg.per = motor->per;
  foc_angle_cfg_f32(&motor->openloop, &ol_cfg);
#endif

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_mode_init
 ****************************************************************************/

static int foc_mode_init(FAR struct foc_motor_f32_s *motor)
{
  int ret = OK;

  switch (motor->envp->mode)
    {
      case FOC_OPMODE_IDLE:
        {
          motor->foc_mode     = FOC_HANDLER_MODE_IDLE;
          break;
        }

#ifdef CONFIG_INDUSTRY_FOC_ANGLE_OPENLOOP
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

static int foc_motor_configure(FAR struct foc_motor_f32_s *motor)
{
#ifdef CONFIG_INDUSTRY_FOC_CONTROL_PI
  struct foc_initdata_f32_s ctrl_cfg;
#endif
#ifdef CONFIG_INDUSTRY_FOC_MODULATION_SVM3
  struct foc_mod_cfg_f32_s mod_cfg;
#endif
#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  struct foc_model_pmsm_cfg_f32_s pmsm_cfg;
#endif
  struct foc_cfg_s cfg;
  int              ret  = OK;

  DEBUGASSERT(motor);

  /* Initialize velocity ramp */

  ret = foc_ramp_init_f32(&motor->ramp,
                          motor->per,
                          RAMP_CFG_THR,
                          RAMP_CFG_ACC,
                          RAMP_CFG_ACC);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_ramp_init failed %d\n", ret);
      goto errout;
    }

  /* Initialize FOC handler */

  ret = foc_handler_init_f32(&motor->handler,
                             &g_foc_control_pi_f32,
                             &g_foc_mod_svm3_f32);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_handler_init failed %d\n", ret);
      goto errout;
    }

#ifdef CONFIG_INDUSTRY_FOC_CONTROL_PI
  /* Get PI controller configuration */

  ctrl_cfg.id_kp = (motor->envp->pi_kp / 1000.0f);
  ctrl_cfg.id_ki = (motor->envp->pi_ki / 1000.0f);
  ctrl_cfg.iq_kp = (motor->envp->pi_kp / 1000.0f);
  ctrl_cfg.iq_ki = (motor->envp->pi_ki / 1000.0f);
#endif

#ifdef CONFIG_INDUSTRY_FOC_MODULATION_SVM3
  /* Get SVM3 modulation configuration */

  mod_cfg.pwm_duty_max = FOCDUTY_TO_FLOAT(motor->info.hw_cfg.pwm_max);
#endif

  /* Configure FOC handler */

  foc_handler_cfg_f32(&motor->handler, &ctrl_cfg, &mod_cfg);

#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  /* Initialize PMSM model */

  ret = foc_model_init_f32(&motor->model,
                           &g_foc_model_pmsm_ops_f32);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_model_init failed %d\n", ret);
      goto errout;
    }

  /* Get PMSM model configuration */

  pmsm_cfg.poles      = FOC_MODEL_POLES;
  pmsm_cfg.res        = FOC_MODEL_RES;
  pmsm_cfg.ind        = FOC_MODEL_IND;
  pmsm_cfg.iner       = FOC_MODEL_INER;
  pmsm_cfg.flux_link  = FOC_MODEL_FLUX;
  pmsm_cfg.ind_d      = FOC_MODEL_INDD;
  pmsm_cfg.ind_q      = FOC_MODEL_INDQ;
  pmsm_cfg.per        = motor->per;
  pmsm_cfg.iphase_adc = motor->iphase_adc;

  /* Configure PMSM model */

  foc_model_cfg_f32(&motor->model, &pmsm_cfg);
#endif

  /* Get FOC device configuration */

  cfg.pwm_freq      = (CONFIG_EXAMPLES_FOC_PWM_FREQ);
  cfg.notifier_freq = (CONFIG_EXAMPLES_FOC_NOTIFIER_FREQ);

  /* Print FOC device configuration */

  foc_cfg_print(&cfg);

  /* Configure FOC device */

  ret = foc_dev_setcfg(motor->envp->dev.fd, &cfg);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_dev_setcfg %d!\n", ret);
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_motor_start
 ****************************************************************************/

static int foc_motor_start(FAR struct foc_motor_f32_s *motor, bool start)
{
  int ret = OK;

  DEBUGASSERT(motor);

  if (start == true)
    {
      /* Start device if VBUS data present */

      if (motor->mq.vbus > 0)
        {
          /* Configure FOC device */

          PRINTF("Configure FOC device %d!\n", motor->envp->id);

          ret = foc_motor_configure(motor);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_motor_configure failed %d!\n", ret);
              goto errout;
            }

          /* Start device */

          PRINTF("Start FOC device %d!\n", motor->envp->id);

          ret = foc_dev_start(motor->envp->dev.fd);
          if (ret < 0)
            {
              PRINTFV("ERROR: foc_dev_start failed %d!\n", ret);
              goto errout;
            }
        }
      else
        {
          /* Return error if no VBUS data */

          PRINTF("ERROR: start request without VBUS!\n");
          goto errout;
        }
    }
  else
    {
      /* Stop FOC device */

      PRINTF("Stop FOC device %d!\n", motor->envp->id);

      ret = foc_dev_stop(motor->envp->dev.fd);
      if (ret < 0)
        {
          PRINTFV("ERROR: foc_dev_stop failed %d!\n", ret);
          goto errout;
        }
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_motor_deinit
 ****************************************************************************/

static int foc_motor_deinit(FAR struct foc_motor_f32_s *motor)
{
  int ret = OK;

  DEBUGASSERT(motor);

#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  /* Deinitialize PMSM model */

  ret = foc_model_deinit_f32(&motor->model);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_model_deinit failed %d\n", ret);
      goto errout;
    }
#endif

  /* Deinitialize FOC handler */

  ret = foc_handler_deinit_f32(&motor->handler);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_handler_deinit failed %d\n", ret);
      goto errout;
    }

  /* Reset data */

  memset(motor, 0, sizeof(struct foc_motor_f32_s));

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_motor_vbus
 ****************************************************************************/

static int foc_motor_vbus(FAR struct foc_motor_f32_s *motor, uint32_t vbus)
{
  DEBUGASSERT(motor);

  /* Update motor VBUS */

  motor->vbus = (vbus * VBUS_ADC_SCALE);

  return OK;
}

/****************************************************************************
 * Name: foc_motor_vel
 ****************************************************************************/

static int foc_motor_vel(FAR struct foc_motor_f32_s *motor, uint32_t vel)
{
  DEBUGASSERT(motor);

  /* Update motor velocity destination */

  motor->vel_des = (vel * VEL_ADC_SCALE * motor->envp->velmax / 1000.0f);

  return OK;
}

/****************************************************************************
 * Name: foc_motor_state
 ****************************************************************************/

static int foc_motor_state(FAR struct foc_motor_f32_s *motor, int state)
{
  int ret = OK;

  DEBUGASSERT(motor);

  /* Get open-loop currents
   * NOTE: Id always set to 0
   */

  motor->dq_ref.q = (motor->envp->qparam / 1000.0f);
  motor->dq_ref.d = 0.0f;

  /* Update motor state */

  switch (state)
    {
      case FOC_EXAMPLE_STATE_FREE:
        {
          motor->vel_set = 0.0f;
          motor->dir     = DIR_NONE;

          /* Force DQ vector to zero */

          motor->dq_ref.q = 0.0f;
          motor->dq_ref.d = 0.0f;

          break;
        }

      case FOC_EXAMPLE_STATE_STOP:
        {
          motor->vel_set = 0.0f;
          motor->dir     = DIR_NONE;

          /* DQ vector not zero - active brake */

          break;
        }

      case FOC_EXAMPLE_STATE_CW:
        {
          motor->vel_set = 0.0f;
          motor->dir     = DIR_CW;

          break;
        }

      case FOC_EXAMPLE_STATE_CCW:
        {
          motor->vel_set = 0.0f;
          motor->dir     = DIR_CCW;

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
 * Name: foc_motor_run
 ****************************************************************************/

static int foc_motor_run(FAR struct foc_motor_f32_s *motor)
{
  struct foc_angle_in_f32_s  ain;
  struct foc_angle_out_f32_s aout;
  int                        ret = OK;

  DEBUGASSERT(motor);

  /* No velocity feedback - assume that velocity now is velocity set
   * TODO: velocity observer or sensor
   */

  motor->vel_now = motor->vel_set;

  /* Run velocity ramp controller */

  ret = foc_ramp_run_f32(&motor->ramp, motor->vel_des,
                         motor->vel_now, &motor->vel_set);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_ramp_run failed %d\n", ret);
      goto errout;
    }

  /* Update open-loop angle handler */

  ain.vel   = motor->vel_set;
  ain.angle = motor->angle_now;
  ain.dir   = motor->dir;

#ifdef CONFIG_INDUSTRY_FOC_ANGLE_OPENLOOP
  foc_angle_run_f32(&motor->openloop, &ain, &aout);

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

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_handler_run
 ****************************************************************************/

static int foc_handler_run(FAR struct foc_motor_f32_s *motor)
{
  struct foc_handler_input_f32_s  input;
  struct foc_handler_output_f32_s output;
  float                           current[CONFIG_MOTOR_FOC_PHASES];
  int                             ret   = OK;
  int                             i     = 0;

  DEBUGASSERT(motor);

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
      current[i] = (motor->iphase_adc * motor->dev_state.curr[i]);
    }

  /* Get input for FOC handler */

  input.current  = current;
  input.dq_ref   = &motor->dq_ref;
  input.vdq_comp = &motor->vdq_comp;
  input.angle    = motor->angle_now;
  input.vbus     = motor->vbus;
  input.mode     = motor->foc_mode;

  /* Run FOC controller */

  ret = foc_handler_run_f32(&motor->handler, &input, &output);

  /* Get duty from controller */

  for (i = 0; i < CONFIG_MOTOR_FOC_PHASES; i += 1)
    {
      motor->dev_params.duty[i] = FOCDUTY_FROM_FLOAT(output.duty[i]);
    }

  /* Get FOC handler state */

  foc_handler_state_f32(&motor->handler, &motor->foc_state);

  return ret;
}

/****************************************************************************
 * Name: foc_dev_state_get
 ****************************************************************************/

static int foc_dev_state_get(FAR struct foc_motor_f32_s *motor)
{
  int ret = OK;
#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  int i;
#endif

  DEBUGASSERT(motor);

  /* Get FOC state - blocking */

  ret = foc_dev_getstate(motor->envp->dev.fd, &motor->dev_state);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_dev_getstate failed %d!\n", ret);
      goto errout;
    }

#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  /* Get model state */

  foc_model_state_f32(&motor->model, &motor->model_state);

  /* Get model currents */

  for (i = 0; i < CONFIG_MOTOR_FOC_PHASES; i += 1)
    {
      motor->dev_state.curr[i] = motor->model_state.curr_raw[i];
    }
#endif

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_dev_params_set
 ****************************************************************************/

static int foc_dev_params_set(FAR struct foc_motor_f32_s *motor)
{
  int ret = OK;

  DEBUGASSERT(motor);

  /* Write FOC parameters */

  ret = foc_dev_setparams(motor->envp->dev.fd, &motor->dev_params);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_dev_setparams failed %d!\n", ret);
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_state_handle
 ****************************************************************************/

static int foc_state_handle(FAR struct foc_motor_f32_s *motor)
{
  DEBUGASSERT(motor);

  if (motor->dev_state.fault > 0)
    {
      PRINTF("FAULT = %d\n", motor->dev_state.fault);
      motor->fault = true;
    }
  else
    {
      motor->fault = false;
    }

  return OK;
}

#ifdef FOC_STATE_PRINT_PRE

/****************************************************************************
 * Name: foc_state_print
 ****************************************************************************/

static int foc_state_print(FAR struct foc_motor_f32_s *motor)
{
  DEBUGASSERT(motor);

  PRINTF("f32 inst %d:\n", motor->envp->inst);

  foc_handler_state_print_f32(&motor->foc_state);

  return OK;
}
#endif

/****************************************************************************
 * Name: foc_motor_handle
 ****************************************************************************/

static int foc_motor_handle(FAR struct foc_motor_f32_s *motor,
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

  if (motor->mq.vel != handle->vel)
    {
      PRINTFV("Set vel=%" PRIu32 " for FOC driver %d!\n",
              handle->vel, motor->envp->id);

      ret = foc_motor_vel(motor, handle->vel);
      if (ret < 0)
        {
          PRINTF("ERROR: foc_motor_vel failed %d!\n", ret);
          goto errout;
        }

      motor->mq.vel = handle->vel;
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

  /* Start/stop motor */

  if (motor->mq.start != handle->start)
    {
      PRINTFV("Set start=%d for FOC driver %d!\n",
              handle->start, motor->envp->id);

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
 * Name: foc_float_thr
 ****************************************************************************/

int foc_float_thr(FAR struct foc_ctrl_env_s *envp)
{
  struct foc_mq_s         handle;
  struct foc_motor_f32_s  motor;
  int                     time      = 0;
  int                     ret       = OK;

  DEBUGASSERT(envp);

  PRINTFV("foc_float_thr, id=%d\n", envp->id);

  /* Reset data */

  memset(&handle, 0, sizeof(struct foc_mq_s));

  /* Initialize motor controller */

  ret = foc_motor_init(&motor, envp);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_motor_init failed %d!\n", ret);
      goto errout;
    }

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
      PRINTFV("foc_float_thr %d %d\n", envp->id, time);

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

      /* Run control logic if controller started */

      if (motor.mq.start == true)
        {
          /* Get FOC device state */

          ret = foc_dev_state_get(&motor);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_dev_state_get failed %d!\n", ret);
              goto errout;
            }

          /* Handle controller state */

          ret = foc_state_handle(&motor);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_state_handle failed %d!\n", ret);
              goto errout;
            }

          if (motor.dev_state.fault > 0)
            {
              /* Clear fault state */

              ret = foc_dev_clearfault(envp->dev.fd);
              if (ret != OK)
                {
                  goto errout;
                }
            }

          /* Run motor controller */

          ret = foc_motor_run(&motor);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_motor_run failed %d!\n", ret);
              goto errout;
            }

          /* Run FOC */

          ret = foc_handler_run(&motor);
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

          foc_model_run_f32(&motor.model,
                            FOC_MODEL_LOAD,
                            &motor.foc_state.vab);
#endif

          /* Set FOC device parameters */

          ret = foc_dev_params_set(&motor);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_dev_prams_set failed %d!\n", ret);
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

  /* Stop FOC device */

  ret = foc_dev_stop(envp->dev.fd);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_dev_stop failed %d!\n", ret);
    }

  PRINTF("foc_float_thr %d exit\n", envp->id);

  return ret;
}
