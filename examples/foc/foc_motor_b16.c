/****************************************************************************
 * apps/examples/foc/foc_motor_b16.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <assert.h>
#include <string.h>
#include <dspb16.h>

#include "foc_cfg.h"
#include "foc_debug.h"
#include "foc_motor_b16.h"
#ifdef CONFIG_EXAMPLES_FOC_FEEDFORWARD
#  include "industry/foc/float/foc_feedforward.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FOC_FLOAT_IDENT_RES_MIN ftob16(1e-6)
#define FOC_FLOAT_IDENT_RES_MAX ftob16(2.0f)

#define FOC_FLOAT_IDENT_IND_MIN ftob16(1e-9)
#define FOC_FLOAT_IDENT_IND_MAX ftob16(2.0f)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
/****************************************************************************
 * Name: foc_align_dir_cb
 ****************************************************************************/

static int foc_align_zero_cb(FAR void *priv, b16_t offset)
{
  FAR foc_angle_b16_t *angle = (FAR foc_angle_b16_t *)priv;

  DEBUGASSERT(angle);

  UNUSED(offset);

  return foc_angle_zero_b16(angle);
}

/****************************************************************************
 * Name: foc_align_dir_cb
 ****************************************************************************/

static int foc_align_dir_cb(FAR void *priv, b16_t dir)
{
  FAR foc_angle_b16_t *angle = (FAR foc_angle_b16_t *)priv;

  DEBUGASSERT(angle);

  return foc_angle_dir_b16(angle, dir);
}

/****************************************************************************
 * Name: foc_motor_align
 ****************************************************************************/

static int foc_motor_align(FAR struct foc_motor_b16_s *motor, FAR bool *done)
{
  struct foc_routine_in_b16_s          in;
  struct foc_routine_out_b16_s         out;
  struct foc_routine_aling_final_b16_s final;
  int                                  ret = OK;

  /* Get input */

  in.foc_state = &motor->foc_state;
  in.angle     = motor->angle_now;
  in.angle_m   = motor->angle_m;
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  in.vel       = motor->vel.now;
#endif
  in.vbus      = motor->vbus;

  /* Run align procedure */

  ret = foc_routine_run_b16(&motor->align, &in, &out);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_routine_run_b16 failed %d!\n", ret);
      goto errout;
    }

  if (ret == FOC_ROUTINE_RUN_DONE)
    {
      ret = foc_routine_final_b16(&motor->align, &final);
      if (ret < 0)
        {
          PRINTFV("ERROR: foc_routine_final_b16 failed %d!\n", ret);
          goto errout;
        }

      PRINTFV("Aling results:\n");
#ifdef CONFIG_INDUSTRY_FOC_ALIGN_DIR
      PRINTFV("  dir    = %.2f\n", b16tof(final.dir));
#endif
      PRINTFV("  offset = %.2f\n", b16tof(final.offset));

      *done = true;
    }

  /* Copy output */

  motor->dq_ref.d   = out.dq_ref.d;
  motor->dq_ref.q   = out.dq_ref.q;
  motor->vdq_comp.d = out.vdq_comp.d;
  motor->vdq_comp.q = out.vdq_comp.q;
  motor->angle_now  = out.angle;
  motor->foc_mode   = out.foc_mode;

errout:
  return ret;
}
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
/****************************************************************************
 * Name: foc_motor_ident
 ****************************************************************************/

static int foc_motor_ident(FAR struct foc_motor_b16_s *motor, FAR bool *done)
{
  struct foc_routine_in_b16_s          in;
  struct foc_routine_out_b16_s         out;
  struct foc_routine_ident_final_b16_s final;
  int                                  ret = OK;

  /* Get input */

  in.foc_state = &motor->foc_state;
  in.angle     = motor->angle_now;
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  in.vel       = motor->vel.now;
#endif
  in.vbus      = motor->vbus;

  /* Run ident procedure */

  ret = foc_routine_run_b16(&motor->ident, &in, &out);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_routine_run_b16 failed %d!\n", ret);
      goto errout;
    }

  if (ret == FOC_ROUTINE_RUN_DONE)
    {
      ret = foc_routine_final_b16(&motor->ident, &final);
      if (ret < 0)
        {
          PRINTFV("ERROR: foc_routine_final_b16 failed %d!\n", ret);
          goto errout;
        }

      PRINTF("Ident results:\n");
      PRINTF("  res   = %.4f\n", b16tof(final.res));
      PRINTF("  ind   = %.8f\n", b16tof(final.ind));

      if (final.res < FOC_FLOAT_IDENT_RES_MIN ||
          final.res > FOC_FLOAT_IDENT_RES_MAX)
        {
          PRINTF("ERROR: Motor resistance out of valid range res=%.4f!\n",
                 b16tof(final.res));

          ret = -EINVAL;
          goto errout;
        }

      if (final.ind < FOC_FLOAT_IDENT_IND_MIN ||
          final.ind > FOC_FLOAT_IDENT_IND_MAX)
        {
          PRINTF("ERROR: Motor inductance out of valid range ind=%.8f!\n",
                 b16tof(final.ind));

          ret = -EINVAL;
          goto errout;
        }

      /* Store results */

      motor->phy_ident.res = final.res;
      motor->phy_ident.ind = final.ind;

      *done = true;
    }

  /* Copy output */

  motor->dq_ref.d   = out.dq_ref.d;
  motor->dq_ref.q   = out.dq_ref.q;
  motor->vdq_comp.d = out.vdq_comp.d;
  motor->vdq_comp.q = out.vdq_comp.q;
  motor->angle_now  = out.angle;
  motor->foc_mode   = out.foc_mode;

errout:
  return ret;
}
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_RUN
/****************************************************************************
 * Name: foc_runmode_init
 ****************************************************************************/

static int foc_runmode_init(FAR struct foc_motor_b16_s *motor)
{
  int ret = OK;

  switch (motor->envp->cfg->fmode)
    {
      case FOC_FMODE_IDLE:
        {
          motor->foc_mode_run = FOC_HANDLER_MODE_IDLE;
          break;
        }

      case FOC_FMODE_VOLTAGE:
        {
          motor->foc_mode_run = FOC_HANDLER_MODE_VOLTAGE;
          break;
        }

      case FOC_FMODE_CURRENT:
        {
          motor->foc_mode_run = FOC_HANDLER_MODE_CURRENT;
          break;
        }

      default:
        {
          PRINTF("ERROR: unsupported op mode %d\n", motor->envp->cfg->fmode);
          ret = -EINVAL;
          goto errout;
        }
    }

  /* Force open-loop if sensorless */

#ifdef CONFIG_EXAMPLES_FOC_SENSORLESS
#  ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  motor->openloop_now = FOC_OPENLOOP_ENABLED;
#  else
#    error
#  endif
#endif

errout:
  return ret;
}
#endif

/****************************************************************************
 * Name: foc_motor_configure
 ****************************************************************************/

static int foc_motor_configure(FAR struct foc_motor_b16_s *motor)
{
  FAR struct foc_control_ops_b16_s    *foc_ctrl = NULL;
  FAR struct foc_modulation_ops_b16_s *foc_mod  = NULL;
#ifdef CONFIG_EXAMPLES_FOC_CONTROL_PI
  struct foc_initdata_b16_s ctrl_cfg;
#endif
#ifdef CONFIG_EXAMPLES_FOC_MODULATION_SVM3
  struct foc_mod_cfg_b16_s mod_cfg;
#endif
#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  struct foc_model_pmsm_cfg_b16_s pmsm_cfg;
#endif
  int              ret  = OK;

  DEBUGASSERT(motor);

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  /* Initialize velocity ramp */

  ret = foc_ramp_init_b16(&motor->ramp,
                          motor->per,
                          ftob16(RAMP_CFG_THR),
                          ftob16((motor->envp->cfg->acc / 1.0f)),
                          ftob16((motor->envp->cfg->dec / 1.0f)));
  if (ret < 0)
    {
      PRINTF("ERROR: foc_ramp_init failed %d\n", ret);
      goto errout;
    }
#endif

  /* Get FOC controller */

#ifdef CONFIG_EXAMPLES_FOC_CONTROL_PI
  foc_ctrl = &g_foc_control_pi_b16;
#else
#  error FOC controller not selected
#endif

  /* Get FOC modulation */

#ifdef CONFIG_EXAMPLES_FOC_MODULATION_SVM3
  foc_mod = &g_foc_mod_svm3_b16;
#else
#  error FOC modulation not selected
#endif

  /* Initialize FOC handler */

  DEBUGASSERT(foc_ctrl != NULL);
  DEBUGASSERT(foc_mod != NULL);

  /* Initialize FOC handler */

  ret = foc_handler_init_b16(&motor->handler,
                             foc_ctrl,
                             foc_mod);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_handler_init failed %d\n", ret);
      goto errout;
    }

#ifdef CONFIG_EXAMPLES_FOC_CONTROL_PI
  /* Get PI controller configuration */

  ctrl_cfg.id_kp = ftob16(motor->envp->cfg->foc_pi_kp / 1000.0f);
  ctrl_cfg.id_ki = ftob16(motor->envp->cfg->foc_pi_ki / 1000.0f);
  ctrl_cfg.iq_kp = ftob16(motor->envp->cfg->foc_pi_kp / 1000.0f);
  ctrl_cfg.iq_ki = ftob16(motor->envp->cfg->foc_pi_ki / 1000.0f);
#endif

#ifdef CONFIG_EXAMPLES_FOC_MODULATION_SVM3
  /* Get SVM3 modulation configuration */

  mod_cfg.pwm_duty_max = motor->pwm_duty_max;
#endif

  /* Configure FOC handler */

  foc_handler_cfg_b16(&motor->handler, &ctrl_cfg, &mod_cfg);

  /* Configure motor phy */

  motor_phy_params_init_b16(&motor->phy,
                   CONFIG_EXAMPLES_FOC_MOTOR_POLES,
                   ftob16(CONFIG_EXAMPLES_FOC_MOTOR_RES / 1000000.0f),
                   ftob16(CONFIG_EXAMPLES_FOC_MOTOR_IND / 1000000.0f),
                   ftob16(CONFIG_EXAMPLES_FOC_MOTOR_FLUXLINK / 1000000.0f));

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
 * Name: foc_motor_vbus
 ****************************************************************************/

static int foc_motor_vbus(FAR struct foc_motor_b16_s *motor, uint32_t vbus)
{
  DEBUGASSERT(motor);

  /* Update motor VBUS */

  motor->vbus = b16muli(vbus, ftob16(VBUS_ADC_SCALE));

  return OK;
}

#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
/****************************************************************************
 * Name: foc_motor_torq
 ****************************************************************************/

static int foc_motor_torq(FAR struct foc_motor_b16_s *motor, uint32_t torq)
{
  b16_t tmp1 = 0;
  b16_t tmp2 = 0;

  DEBUGASSERT(motor);

  /* Update motor torqocity destination
   * NOTE: torqmax may not fit in b16_t so we can't use b16idiv()
   */

  tmp1 = itob16(motor->envp->cfg->torqmax / 1000);
  tmp2 = b16mulb16(ftob16(SETPOINT_INTF_SCALE), tmp1);

  motor->torq.des = b16mulb16(motor->dir, b16muli(tmp2, torq));

  return OK;
}
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
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

  tmp1 = itob16(motor->envp->cfg->velmax / 1000);
  tmp2 = b16mulb16(ftob16(SETPOINT_INTF_SCALE), tmp1);

  motor->vel.des = b16mulb16(motor->dir, b16muli(tmp2, vel));

  return OK;
}
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
/****************************************************************************
 * Name: foc_motor_pos
 ****************************************************************************/

static int foc_motor_pos(FAR struct foc_motor_b16_s *motor, uint32_t pos)
{
  b16_t tmp1 = 0;
  b16_t tmp2 = 0;

  DEBUGASSERT(motor);

  /* Update motor posocity destination
   * NOTE: posmax may not fit in b16_t so we can't use b16idiv()
   */

  tmp1 = itob16(motor->envp->cfg->posmax / 1000);
  tmp2 = b16mulb16(ftob16(SETPOINT_INTF_SCALE), tmp1);

  motor->pos.des = b16mulb16(motor->dir, b16muli(tmp2, pos));

  return OK;
}
#endif

/****************************************************************************
 * Name: foc_motor_setpoint
 ****************************************************************************/

static int foc_motor_setpoint(FAR struct foc_motor_b16_s *motor, uint32_t sp)
{
  int ret = OK;

  switch (motor->envp->cfg->mmode)
    {
#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
      case FOC_MMODE_TORQ:
        {
          /* Update motor torque destination */

          ret = foc_motor_torq(motor, sp);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_motor_torq failed %d!\n", ret);
              goto errout;
            }

          break;
        }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
      case FOC_MMODE_VEL:
        {
          /* Update motor velocity destination */

          ret = foc_motor_vel(motor, sp);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_motor_vel failed %d!\n", ret);
              goto errout;
            }

          break;
        }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
      case FOC_MMODE_POS:
        {
          /* Update motor position destination */

          ret = foc_motor_pos(motor, sp);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_motor_pos failed %d!\n", ret);
              goto errout;
            }

          break;
        }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
      case FOC_MMODE_ALIGN_ONLY:
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
      case FOC_MMODE_IDENT_ONLY:
#endif
        {
          /* Do nothing */

          break;
        }

      default:
        {
          PRINTF("ERROR: unsupported ctrl mode %d\n",
                 motor->envp->cfg->mmode);
          ret = -EINVAL;
          goto errout;
        }
    }

errout:
  return ret;
}

#if defined(CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP) || \
    defined(CONFIG_EXAMPLES_FOC_VELOBS)
/****************************************************************************
 * Name: foc_motor_vel_reset
 ****************************************************************************/

static int foc_motor_vel_reset(FAR struct foc_motor_b16_s *motor)
{
  int ret = OK;

  /* Reset velocity observer */

#ifdef CONFIG_EXAMPLES_FOC_VELOBS_DIV
  ret = foc_velocity_zero_b16(&motor->vel_div);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_velocity_zero failed %d\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_VELOBS_PLL
  ret = foc_velocity_zero_b16(&motor->vel_pll);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_velocity_zero failed %d\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_VELOBS
  errout:
#endif
  return ret;
}
#endif

/****************************************************************************
 * Name: foc_motor_state
 ****************************************************************************/

static int foc_motor_state(FAR struct foc_motor_b16_s *motor, int state)
{
  int ret = OK;

  DEBUGASSERT(motor);

  /* Update motor state - this function is called every controller cycle */

  switch (state)
    {
      case FOC_EXAMPLE_STATE_FREE:
        {
          motor->dir = DIR_NONE_B16;

          /* Force DQ vector to zero */

          motor->dq_ref.q = 0;
          motor->dq_ref.d = 0;

          break;
        }

      case FOC_EXAMPLE_STATE_STOP:
        {
#ifdef CONFIG_EXAMPLES_FOC_SENSORLESS
          /* For sensorless we can just set Q reference to lock the motor */

          motor->dir = DIR_NONE_B16;

          /* DQ vector not zero - active brake */

          motor->dq_ref.q = ftob16(CONFIG_EXAMPLES_FOC_STOP_CURRENT /
                                   1000.0f);
          motor->dq_ref.d = 0;
#else
          /* For sensored mode we set requested velocity to 0 */

#  ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
          motor->vel.des = 0;
#  else
#    error STOP state for sensored mode requires velocity support
#  endif
#endif

          break;
        }

      case FOC_EXAMPLE_STATE_CW:
        {
          motor->dir = DIR_CW_B16;

          break;
        }

      case FOC_EXAMPLE_STATE_CCW:
        {
          motor->dir = DIR_CCW_B16;

          break;
        }

      default:
        {
          ret = -EINVAL;
          goto errout;
        }
    }

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  /* Re-align motor if we change mode from FREE/STOP to CW/CCW otherwise,
   * the open-loop may fail because the rotor position at the start is
   * random.
   */

  if ((motor->mq.app_state == FOC_EXAMPLE_STATE_FREE ||
       motor->mq.app_state == FOC_EXAMPLE_STATE_STOP) &&
      (state == FOC_EXAMPLE_STATE_CW ||
       state == FOC_EXAMPLE_STATE_CCW))
    {
      motor->ctrl_state = FOC_CTRL_STATE_ALIGN;
      motor->align_done = false;
      motor->angle_now  = 0;

      /* Reset velocity observer */

      foc_motor_vel_reset(motor);
    }
#endif

  /* Reset current setpoint */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
  motor->torq.set = 0;
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  motor->vel.set = 0;
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
  motor->pos.set = 0;
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

#ifdef CONFIG_EXAMPLES_FOC_HAVE_RUN

/****************************************************************************
 * Name: foc_motor_run_init
 ****************************************************************************/

static int foc_motor_run_init(FAR struct foc_motor_b16_s *motor)
{
  int ret = OK;

#ifdef CONFIG_EXAMPLES_FOC_VELOBS
  ret = foc_motor_vel_reset(motor);
#endif

  return ret;
}

#ifdef CONFIG_EXAMPLES_FOC_ANGOBS
/****************************************************************************
 * Name: foc_motor_openloop_trans
 ****************************************************************************/

static void foc_motor_openloop_trans(FAR struct foc_motor_b16_s *motor)
{
#ifdef CONFIG_EXAMPLES_FOC_VELCTRL_PI
  /* Set the intergral part of the velocity PI controller equal to the
   * open-loop Q current value.
   *
   * REVISIT: this may casue a velocity overshoot when going from open-loop
   *          to closed-loop. We can either use part of the open-loop Q
   *          current here or gradually reduce the Q current during
   *          transition.
   */

  motor->vel_pi.part[1] = b16mulb16(motor->dir, motor->openloop_q);
  motor->vel_pi.part[0] = 0;
#endif
}

/****************************************************************************
 * Name: foc_motor_openloop_angobs
 ****************************************************************************/

static void foc_motor_openloop_angobs(FAR struct foc_motor_b16_s *motor)
{
  b16_t vel_abs = 0;

  vel_abs = b16abs(motor->vel_el);

  /* Disable open-loop if velocity above threshold */

  if (motor->openloop_now == FOC_OPENLOOP_ENABLED)
    {
      if (vel_abs >= motor->ol_thr)
        {
          /* Store angle error between the forced open-loop angle and
           * observer output. The error will be gradually eliminated over
           * the next controller cycles.
           */

#ifdef ANGLE_MERGE_FACTOR
          motor->angle_step = b16mulb16(motor->angle_err,
                                        ftob16(ANGLE_MERGE_FACTOR));
          motor->angle_err = motor->angle_ol - motor->angle_obs;
#endif

          motor->openloop_now = FOC_OPENLOOP_TRANSITION;
        }
    }

  /* Handle transition end */

  else if (motor->openloop_now == FOC_OPENLOOP_TRANSITION)
    {
      if (motor->angle_err == 0)
        {
          /* Call open-open loop transition handler */

          foc_motor_openloop_trans(motor);

          motor->openloop_now = FOC_OPENLOOP_DISABLED;
        }
    }

  /* Enable open-loop if velocity below threshold with hysteresis  */

  else if (motor->openloop_now == FOC_OPENLOOP_DISABLED)
    {
      /* For better stability we add hysteresis from transition
       * from closed-loop to open-loop.
       */

      if (vel_abs < (motor->ol_thr - motor->ol_hys))
        {
          motor->openloop_now = FOC_OPENLOOP_ENABLED;
        }
    }
}
#endif

/****************************************************************************
 * Name: foc_motor_run
 ****************************************************************************/

static int foc_motor_run(FAR struct foc_motor_b16_s *motor)
{
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  b16_t vel_err = 0.0f;
#endif
  b16_t q_ref = 0;
  b16_t d_ref = 0;
  int   ret   = OK;

  DEBUGASSERT(motor);

#ifdef CONFIG_EXAMPLES_FOC_ANGOBS
  if (motor->envp->cfg->ol_force == false)
    {
      /* Handle open-loop to observer transition */

      foc_motor_openloop_angobs(motor);
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  /* Open-loop works only in velocity control mode */

  if (motor->openloop_now == true)
    {
#  ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
      if (motor->envp->cfg->mmode != FOC_MMODE_VEL)
#endif
        {
          PRINTF("ERROR: open-loop only with FOC_MMODE_VEL\n");
          ret = -EINVAL;
          goto errout;
        }
    }
#endif

  /* Get previous DQ */

  q_ref = motor->dq_ref.q;
  d_ref = motor->dq_ref.d;

  /* Ignore controller if motor is free (sensorless and sensored mode)
   * or stopped (only sensorless mode)
   */

  if (motor->mq.app_state == FOC_EXAMPLE_STATE_FREE
#ifdef CONFIG_EXAMPLES_FOC_SENSORLESS
      || motor->mq.app_state == FOC_EXAMPLE_STATE_STOP
#endif
    )
    {
      goto no_controller;
    }

  /* Controller.
   *
   * The FOC motor controller is a cascade controller:
   *
   *   1. Position controller sets requested velocity,
   *   2. Velocity controller sets requested torque,
   *   3. Torque controller sets requested motor phase voltages.
   *
   *      NOTE: the motor torque is directly proportional to the motor
   *            current which is proportional to the motor set voltage
   */

  switch (motor->envp->cfg->mmode)
    {
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
      case FOC_MMODE_VEL:
        {
          /* Saturate velocity */

          f_saturate_b16(&motor->vel.des, -motor->vel_sat,
                         motor->vel_sat);

          /* Velocity controller */

          if (motor->time % VEL_CONTROL_PRESCALER == 0)
            {
              /* Run velocity ramp controller */

              ret = foc_ramp_run_b16(&motor->ramp,
                                     motor->vel.des,
                                     motor->vel.now,
                                     &motor->vel.set);
              if (ret < 0)
                {
                  PRINTF("ERROR: foc_ramp_run failed %d\n", ret);
                  goto errout;
                }

              /* Run velocity controller if no in open-loop */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
              if (motor->openloop_now == false)
#endif
                {
                  /* Get velocity error */

                  vel_err = motor->vel.set - motor->vel.now;

#ifdef CONFIG_EXAMPLES_FOC_VELCTRL_PI
                  /* PI velocit controller */

                  motor->torq.des = pi_controller_b16(&motor->vel_pi,
                                                      vel_err);
#else
#  error Missing velocity controller
#endif
                }
            }

          /* Don't break here! pass to torque controller */
        }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
      case FOC_MMODE_TORQ:
        {
          /* Saturate torque */

          f_saturate_b16(&motor->torq.des, -motor->torq_sat,
                         motor->torq_sat);

          /* Torque setpoint */

          motor->torq.set = motor->torq.des;
          motor->torq.now = motor->foc_state.idq.q;

          break;
        }
#endif

      default:
        {
          ret = -EINVAL;
          goto errout;
        }
    }

  /* Get dq ref */

  q_ref = motor->torq.set;
  d_ref = 0;

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  /* Force open-loop current */

  if (motor->openloop_now == FOC_OPENLOOP_ENABLED ||
      motor->openloop_now == FOC_OPENLOOP_TRANSITION)
    {
      /* Get open-loop currents. Positive for CW direction, negative for
       * CCW direction. Id always set to 0.
       */

      q_ref = b16mulb16(motor->dir, motor->openloop_q);
      d_ref = 0;
    }
#endif

no_controller:

  /* Set DQ reference frame */

  motor->dq_ref.q = q_ref;
  motor->dq_ref.d = d_ref;

  /* DQ compensation */

#ifdef CONFIG_EXAMPLES_FOC_FEEDFORWARD
  foc_feedforward_pmsm_b16(&motor->phy, &motor->foc_state.idq,
                           motor->vel.now, &motor->vdq_comp);
#else
  motor->vdq_comp.q = 0;
  motor->vdq_comp.d = 0;
#endif

errout:
  return ret;
}
#endif

/****************************************************************************
 * Name: foc_motor_ang_get
 ****************************************************************************/

static int foc_motor_ang_get(FAR struct foc_motor_b16_s *motor)
{
  struct foc_angle_in_b16_s  ain;
  struct foc_angle_out_b16_s aout;
  int                        ret = OK;

  DEBUGASSERT(motor);

  /* Update open-loop angle handler */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  ain.vel   = motor->vel.set;
#endif
  ain.state = &motor->foc_state;
  ain.angle = motor->angle_now;
  ain.dir   = motor->dir;

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  if (motor->openloop_now != FOC_OPENLOOP_DISABLED)
    {
      foc_angle_run_b16(&motor->openloop, &ain, &aout);

      /* Store open-loop angle */

      motor->angle_ol = aout.angle;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_QENCO
  ret = foc_angle_run_b16(&motor->qenco, &ain, &aout);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_angle_run failed %d\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_HALL
  ret = foc_angle_run_b16(&motor->hall, &ain, &aout);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_angle_run failed %d\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_ANGOBS_SMO
  ret = foc_angle_run_b16(&motor->ang_smo, &ain, &aout);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_angle_run failed %d\n", ret);
      goto errout;
    }

  motor->angle_obs = aout.angle;
#endif

#ifdef CONFIG_EXAMPLES_FOC_ANGOBS_NFO
  ret = foc_angle_run_b16(&motor->ang_nfo, &ain, &aout);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_angle_run failed %d\n", ret);
      goto errout;
    }

  motor->angle_obs = aout.angle;
#endif

#ifndef CONFIG_EXAMPLES_FOC_HAVE_RUN
  /* Dummy value when motor controller disabled */

  UNUSED(ain);
  aout.type  = FOC_ANGLE_TYPE_ELE;
  aout.angle = 0;
#endif

  /* Store electrical angle from sensor or observer */

  if (aout.type == FOC_ANGLE_TYPE_ELE)
    {
      /* Store electrical angle */

      motor->angle_el = aout.angle;
    }

  else if (aout.type == FOC_ANGLE_TYPE_MECH)
    {
      /* Store mechanical angle */

      motor->angle_m = aout.angle;

      /* Convert mechanical angle to electrical angle */

      motor->angle_el = (b16muli(motor->angle_m,
                                 motor->phy.p) %
                         MOTOR_ANGLE_E_MAX_B16);
    }

  else
    {
      ASSERT(0);
    }

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  /* Get open-loop phase angle */

  if (motor->openloop_now == FOC_OPENLOOP_ENABLED)
    {
      motor->angle_now = motor->angle_ol;
      motor->angle_el = motor->angle_ol;
    }
  else
#endif
#ifdef CONFIG_EXAMPLES_FOC_SENSORLESS
  /* Handle smooth open-loop to closed-loop transition */

  if (motor->openloop_now == FOC_OPENLOOP_TRANSITION)
    {
#ifdef ANGLE_MERGE_FACTOR
      if (b16abs(motor->angle_err) > b16abs(motor->angle_step))
        {
          motor->angle_now = motor->angle_obs + motor->angle_err;

          /* Update error */

          motor->angle_err -= motor->angle_step;
        }
      else
#endif
        {
          motor->angle_now = motor->angle_obs;
          motor->angle_err = 0;
        }
    }

  /* Get angle from observer if closed-loop now */

  else
    {
      motor->angle_now = motor->angle_obs;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_SENSORED
  /* Get phase angle from sensor */

  motor->angle_now = motor->angle_el;
#endif

#if defined(CONFIG_EXAMPLES_FOC_SENSORED) || defined(CONFIG_EXAMPLES_FOC_ANGOBS)
  errout:
#endif
  return ret;
}

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
/****************************************************************************
 * Name: foc_motor_vel_get
 ****************************************************************************/

static int foc_motor_vel_get(FAR struct foc_motor_b16_s *motor)
{
  struct foc_velocity_in_b16_s  vin;
  struct foc_velocity_out_b16_s vout;
  int                           ret = OK;

  DEBUGASSERT(motor);

  /* Update velocity handler input data */

  vin.state = &motor->foc_state;
  vin.angle = motor->angle_now;
  vin.vel   = motor->vel.now;
  vin.dir   = motor->dir;

  /* Get velocity from observer */

#ifdef CONFIG_EXAMPLES_FOC_VELOBS_DIV
  ret = foc_velocity_run_b16(&motor->vel_div, &vin, &vout);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_velocity_run failed %d\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_VELOBS_PLL
  ret = foc_velocity_run_b16(&motor->vel_pll, &vin, &vout);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_velocity_run failed %d\n", ret);
      goto errout;
    }
#endif

  /* Get motor electrical velocity now */

#if defined(CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP) && \
    !defined(CONFIG_EXAMPLES_FOC_VELOBS) ||       \
    !defined(CONFIG_EXAMPLES_FOC_HAVE_RUN)
  /* No velocity feedback - assume that electical velocity is velocity set
   * in a open-loop contorller.
   */

  UNUSED(vin);
  UNUSED(vout);

  motor->vel_el = motor->vel.set;
#elif defined(CONFIG_EXAMPLES_FOC_VELOBS) && defined(CONFIG_EXAMPLES_FOC_SENSORLESS)
  if (motor->openloop_now == FOC_OPENLOOP_DISABLED)
    {
      /* Get electrical velocity from observer if we are in closed-loop */

      motor->vel_el = motor->vel_obs;
    }
  else
    {
      /* Otherwise use open-loop velocity */

      motor->vel_el = motor->vel.set;
    }
#elif defined(CONFIG_EXAMPLES_FOC_VELOBS) && defined(CONFIG_EXAMPLES_FOC_SENSORED)
  /* Get electrical velocity from observer in sensored mode */

  motor->vel_el = motor->vel_obs;
#else
  /* Need electrical velocity source here - raise assertion */

  ASSERT(0);
#endif

  LP_FILTER_B16(motor->vel.now, motor->vel_el, motor->vel_filter);

  /* Get mechanical velocity (rad/s) */

  motor->vel_mech = b16mulb16(motor->vel_el, motor->phy.one_by_p);

#ifdef CONFIG_EXAMPLES_FOC_VELOBS
errout:
#endif
  return ret;
}
#endif  /* CONFIG_EXAMPLES_FOC_HAVE_VEL */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_motor_init
 ****************************************************************************/

int foc_motor_init(FAR struct foc_motor_b16_s *motor,
                   FAR struct foc_ctrl_env_s *envp)
{
#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  struct foc_openloop_cfg_b16_s      ol_cfg;
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_QENCO
  struct foc_qenco_cfg_b16_s         qe_cfg;
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_HALL
  struct foc_hall_cfg_b16_s          hl_cfg;
#endif
#ifdef CONFIG_EXAMPLES_FOC_ANGOBS_SMO
  struct foc_angle_osmo_cfg_b16_s    smo_cfg;
#endif
#ifdef CONFIG_EXAMPLES_FOC_ANGOBS_NFO
  struct foc_angle_onfo_cfg_b16_s    nfo_cfg;
#endif
#ifdef CONFIG_EXAMPLES_FOC_VELOBS_DIV
  struct foc_vel_div_b16_cfg_s       odiv_cfg;
#endif
#ifdef CONFIG_EXAMPLES_FOC_VELOBS_PLL
  struct foc_vel_pll_b16_cfg_s       opll_cfg;
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
  struct foc_routine_align_cfg_b16_s align_cfg;
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
  struct foc_routine_ident_cfg_b16_s ident_cfg;
#endif
  int                                ret = OK;

  DEBUGASSERT(motor);
  DEBUGASSERT(envp);

  /* Reset data */

  memset(motor, 0, sizeof(struct foc_motor_b16_s));

  /* Connect envp with motor handler */

  motor->envp = envp;

  /* Initialize motor data */

  motor->per        = b16divi(b16ONE, CONFIG_EXAMPLES_FOC_NOTIFIER_FREQ);
#ifdef CONFIG_EXAMPLES_FOC_ANGOBS
  motor->ol_thr     = ftob16(motor->envp->cfg->ol_thr / 1.0f);
  motor->ol_hys     = ftob16(motor->envp->cfg->ol_hys / 1.0f);
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
  motor->torq_sat   = ftob16(CONFIG_EXAMPLES_FOC_TORQ_MAX / 1000.0f);
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  motor->vel_sat    = ftob16(CONFIG_EXAMPLES_FOC_VEL_MAX / 1.0f);
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_RUN
  /* Initialize controller run mode */

  ret = foc_runmode_init(motor);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_runmode_init failed %d!\n", ret);
      goto errout;
    }
#endif

  /* Start with FOC IDLE mode */

  motor->foc_mode = FOC_HANDLER_MODE_IDLE;

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  /* Initialize open-loop angle handler */

  foc_angle_init_b16(&motor->openloop,
                     &g_foc_angle_ol_b16);

  /* Configure open-loop angle handler */

  ol_cfg.per = motor->per;
  foc_angle_cfg_b16(&motor->openloop, &ol_cfg);

  /* Store open-loop Q-param */

  motor->openloop_q = ftob16(motor->envp->cfg->qparam / 1000.0f);
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_QENCO
  /* Initialize qenco angle handler */

  ret = foc_angle_init_b16(&motor->qenco,
                           &g_foc_angle_qe_b16);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_init_b16 failed %d!\n", ret);
      goto errout;
    }

  /* Get qenco devpath */

  snprintf(motor->qedpath, sizeof(motor->qedpath),
           "%s%d",
           CONFIG_EXAMPLES_FOC_QENCO_DEVPATH,
           motor->envp->id);

  /* Configure qenco angle handler */

  qe_cfg.posmax  = CONFIG_EXAMPLES_FOC_QENCO_POSMAX;
  qe_cfg.devpath = motor->qedpath;

  ret = foc_angle_cfg_b16(&motor->qenco, &qe_cfg);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_cfg_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_HALL
  /* Initialize hall angle handler */

  ret = foc_angle_init_b16(&motor->hall,
                           &g_foc_angle_hl_b16);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_init_b16 failed %d!\n", ret);
      goto errout;
    }

  /* Get hall devpath */

  snprintf(motor->hldpath, sizeof(motor->hldpath),
           "%s%d",
           CONFIG_EXAMPLES_FOC_HALL_DEVPATH,
           motor->envp->id);

  /* Configure hall angle handler */

  hl_cfg.devpath = motor->hldpath;
  hl_cfg.per     = motor->per;

  ret = foc_angle_cfg_b16(&motor->hall, &hl_cfg);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_cfg_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_ANGOBS_SMO
  /* Initialzie SMO angle observer handler */

  ret = foc_angle_init_b16(&motor->ang_smo,
                           &g_foc_angle_osmo_b16);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_init_b16 failed %d!\n", ret);
      goto errout;
    }

  /* Configure SMO angle handler */

  smo_cfg.per     = motor->per;
  smo_cfg.k_slide = ftob16(CONFIG_EXAMPLES_FOC_ANGOBS_SMO_KSLIDE / 1000.0f);
  smo_cfg.err_max = ftob16(CONFIG_EXAMPLES_FOC_ANGOBS_SMO_ERRMAX / 1000.0f);
  memcpy(&smo_cfg.phy, &motor->phy, sizeof(struct motor_phy_params_b16_s));

  ret = foc_angle_cfg_b16(&motor->ang_smo, &smo_cfg);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_cfg_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_ANGOBS_NFO
  /* Initialzie NFO angle observer handler */

  ret = foc_angle_init_b16(&motor->ang_nfo,
                           &g_foc_angle_onfo_b16);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_init_b16 failed %d!\n", ret);
      goto errout;
    }

  /* Configure NFO angle handler */

  nfo_cfg.per       = motor->per;
  nfo_cfg.gain      = ftob16(motor->envp->cfg->ang_nfo_gain / 1.0f);
  nfo_cfg.gain_slow = ftob16(motor->envp->cfg->ang_nfo_slow / 1.0f);
  memcpy(&nfo_cfg.phy, &motor->phy, sizeof(struct motor_phy_params_b16_s));

  ret = foc_angle_cfg_b16(&motor->ang_nfo, &nfo_cfg);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_cfg_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_VELOBS_DIV
  /* Initialize velocity observer */

  ret = foc_velocity_init_b16(&motor->vel_div,
                              &g_foc_velocity_odiv_b16);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_init_b16 failed %d!\n", ret);
      goto errout;
    }

  /* Configure velocity observer */

  odiv_cfg.samples = (motor->envp->cfg->vel_div_samples);
  odiv_cfg.filter  = ftob16(motor->envp->cfg->vel_div_samples / 1000.0f);
  odiv_cfg.per     = motor->per;

  ret = foc_velocity_cfg_b16(&motor->vel_div, &odiv_cfg);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_velocity_cfg_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_VELOBS_PLL
  /* Initialize velocity observer */

  ret = foc_velocity_init_b16(&motor->vel_pll,
                              &g_foc_velocity_opll_b16);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_init_b16 failed %d!\n", ret);
      goto errout;
    }

  /* Configure velocity observer */

  opll_cfg.kp  = ftob16(motor->envp->cfg->vel_pll_kp / 1.0f);
  opll_cfg.ki  = ftob16(motor->envp->cfg->vel_pll_ki / 1.0f);
  opll_cfg.per = motor->per;

  ret = foc_velocity_cfg_b16(&motor->vel_pll, &opll_cfg);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_velocity_cfg_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_VELCTRL_PI
  /* Initialize velocity controller */

  pi_controller_init_b16(&motor->vel_pi,
                   ftob16(motor->envp->cfg->vel_pi_kp / 1000000.0f),
                   ftob16(motor->envp->cfg->vel_pi_ki / 1000000.0f));

  pi_saturation_set_b16(&motor->vel_pi,  -motor->torq_sat, motor->torq_sat);

  pi_antiwindup_enable_b16(&motor->vel_pi, ftob16(0.99f), true);
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
  /* Initialize motor alignment routine */

  ret = foc_routine_init_b16(&motor->align, &g_foc_routine_align_b16);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_routine_init_b16 failed %d!\n", ret);
      goto errout;
    }

  /* Initialize motor alignment data */

  align_cfg.volt         = ftob16(CONFIG_EXAMPLES_FOC_ALIGN_VOLT / 1000.0f);
  align_cfg.offset_steps = (CONFIG_EXAMPLES_FOC_NOTIFIER_FREQ * \
                            CONFIG_EXAMPLES_FOC_ALIGN_SEC / 1000);

  /* Connect align callbacks */

  align_cfg.cb.zero = foc_align_zero_cb;
  align_cfg.cb.dir  = foc_align_dir_cb;

  /* Connect align callbacks private data */

#  ifdef CONFIG_EXAMPLES_FOC_SENSORLESS
  align_cfg.cb.priv = &motor->openloop;
#  endif
#  ifdef CONFIG_EXAMPLES_FOC_HAVE_QENCO
  align_cfg.cb.priv = &motor->qenco;
#  endif
#  ifdef CONFIG_EXAMPLES_FOC_HAVE_HALL
  align_cfg.cb.priv = &motor->hall;
#  endif

  ret = foc_routine_cfg_b16(&motor->align, &align_cfg);
  if (ret < 0)
    {
#  ifndef CONFIG_EXAMPLES_FOC_RUN_DISABLE
      PRINTFV("ERROR: foc_routine_cfg_b16 failed %d!\n", ret);
      goto errout;
#  else
      /* When motor controller is disabled, most likely we don't care about
       * align routine failure
       */

      PRINTFV("ignore align routine failure\n", ret);
      ret = OK;
#  endif
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT

  /* Initialize motor identifiaction routine */

  ret = foc_routine_init_b16(&motor->ident, &g_foc_routine_ident_b16);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_routine_init_b16 failed %d!\n", ret);
      goto errout;
    }

  /* Initialize motor identification data */

  ident_cfg.per         = motor->per;
  ident_cfg.res_current = ftob16(motor->envp->cfg->ident_res_curr /
                                 1000.0f);
  ident_cfg.res_ki      = ftob16(motor->envp->cfg->ident_res_ki /
                                 1000.0f);
  ident_cfg.ind_volt    = ftob16(motor->envp->cfg->ident_ind_volt /
                                 1000.0f);
  ident_cfg.res_steps   = (CONFIG_EXAMPLES_FOC_NOTIFIER_FREQ *
                           motor->envp->cfg->ident_res_sec / 1000);
  ident_cfg.ind_steps   = (CONFIG_EXAMPLES_FOC_NOTIFIER_FREQ *
                           motor->envp->cfg->ident_ind_sec / 1000);
  ident_cfg.idle_steps  = CONFIG_EXAMPLES_FOC_IDENT_IDLE;

  ret = foc_routine_cfg_b16(&motor->ident, &ident_cfg);
  if (ret < 0)
    {
#  ifndef CONFIG_EXAMPLES_FOC_RUN_DISABLE
      PRINTFV("ERROR: foc_ident_cfg_b16 failed %d!\n", ret);
      goto errout;
#  else
      /* When motor controller is disabled, most likely we don't care about
       * ident routine failure
       */

      PRINTFV("ident align routine failure\n", ret);
      ret = OK;
#  endif
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  /* Store velocity filter value */

  motor->vel_filter = ftob16(motor->envp->cfg->vel_filter / 1000.0f);
#endif

  /* Initialize controller state */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
  if (motor->envp->cfg->mmode == FOC_MMODE_ALIGN_ONLY)
    {
      motor->ctrl_state = FOC_CTRL_STATE_ALIGN;
    }
  else
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
  if (motor->envp->cfg->mmode == FOC_MMODE_IDENT_ONLY)
    {
      motor->ctrl_state = FOC_CTRL_STATE_IDENT;
    }
  else
#endif
    {
      motor->ctrl_state = FOC_CTRL_STATE_INIT;
    }

#if defined(CONFIG_EXAMPLES_FOC_SENSORED)   || \
    defined(CONFIG_EXAMPLES_FOC_HAVE_RUN)   || \
    defined(CONFIG_EXAMPLES_FOC_HAVE_ALIGN) || \
    defined(CONFIG_EXAMPLES_FOC_HAVE_IDENT)
errout:
#endif
  return ret;
}

/****************************************************************************
 * Name: foc_motor_deinit
 ****************************************************************************/

int foc_motor_deinit(FAR struct foc_motor_b16_s *motor)
{
  int ret = OK;

  DEBUGASSERT(motor);

  #ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  /* Deinitialzie open-loop handler */

  ret = foc_angle_deinit_b16(&motor->openloop);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_deinit_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_QENCO
  /* Deinitialzie qenco handler */

  ret = foc_angle_deinit_b16(&motor->qenco);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_deinit_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_HALL
  /* Deinitialzie hall handler */

  ret = foc_angle_deinit_b16(&motor->hall);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_deinit_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_ANGOBS_SMO
  /* Deinitialize SMO observer handler */

  ret = foc_angle_deinit_b16(&motor->ang_smo);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_deinit_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_ANGOBS_NFO
  /* Deinitialize NFO observer handler */

  ret = foc_angle_deinit_b16(&motor->ang_nfo);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_angle_deinit_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_VELOBS_DIV
  /* Deinitialize DIV observer handler */

  ret = foc_velocity_deinit_b16(&motor->vel_div);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_velocity_deinit_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_VELOBS_PLL
  /* Deinitialize PLL observer handler */

  ret = foc_velocity_deinit_b16(&motor->vel_pll);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_velocity_deinit_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
  /* Deinitialize motor alignment routine */

  ret = foc_routine_deinit_b16(&motor->align);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_routine_deinit_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
  /* Deinitialize motor identification routine */

  ret = foc_routine_deinit_b16(&motor->ident);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_routine_deinit_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

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
 * Name: foc_motor_get
 ****************************************************************************/

int foc_motor_get(FAR struct foc_motor_b16_s *motor)
{
  int ret = OK;

  DEBUGASSERT(motor);

  /* Get motor angle */

  ret = foc_motor_ang_get(motor);
  if (ret < 0)
    {
      goto errout;
    }

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  /* Get motor velocity */

  ret = foc_motor_vel_get(motor);
  if (ret < 0)
    {
      goto errout;
    }
#endif

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_motor_control
 ****************************************************************************/

int foc_motor_control(FAR struct foc_motor_b16_s *motor)
{
  int ret = OK;

  DEBUGASSERT(motor);

  /* Controller state machine */

  switch (motor->ctrl_state)
    {
      case FOC_CTRL_STATE_INIT:
        {
          /* Next state */

          motor->ctrl_state += 1;
          motor->foc_mode = FOC_HANDLER_MODE_IDLE;

          break;
        }

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
      case FOC_CTRL_STATE_ALIGN:
        {
          /* Run motor align procedure */

          ret = foc_motor_align(motor, &motor->align_done);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_motor_align failed %d!\n", ret);
              goto errout;
            }

          if (motor->align_done == true)
            {
              /* Next state */

              motor->ctrl_state += 1;
              motor->foc_mode = FOC_HANDLER_MODE_IDLE;

              if (motor->envp->cfg->mmode == FOC_MMODE_ALIGN_ONLY)
                {
                  motor->ctrl_state = FOC_CTRL_STATE_TERMINATE;
                }
            }

          break;
        }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
      case FOC_CTRL_STATE_IDENT:
        {
          /* Run motor identification procedure */

          ret = foc_motor_ident(motor, &motor->ident_done);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_motor_ident failed %d!\n", ret);
              goto errout;
            }

          if (motor->ident_done == true)
            {
              /* Next state */

              motor->ctrl_state += 1;
              motor->foc_mode = FOC_HANDLER_MODE_IDLE;

              if (motor->envp->cfg->mmode == FOC_MMODE_IDENT_ONLY)
                {
                  motor->ctrl_state = FOC_CTRL_STATE_TERMINATE;
                }
            }

          break;
        }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_RUN
      case FOC_CTRL_STATE_RUN_INIT:
        {
          /* Initialize controller run mode */

          ret = foc_motor_run_init(motor);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_motor_run_init failed %d!\n", ret);
              goto errout;
            }

          /* Next state */

          motor->ctrl_state += 1;
          motor->foc_mode = FOC_HANDLER_MODE_IDLE;

          break;
        }

      case FOC_CTRL_STATE_RUN:
        {
          /* Get FOC run mode */

          motor->foc_mode = motor->foc_mode_run;

          /* Run motor */

          ret = foc_motor_run(motor);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_motor_run failed %d!\n", ret);
              goto errout;
            }

          break;
        }
#endif

      case FOC_CTRL_STATE_IDLE:
        {
          motor->foc_mode = FOC_HANDLER_MODE_IDLE;

          break;
        }

      case FOC_CTRL_STATE_TERMINATE:
        {
          /* Do nothing */

          break;
        }

      default:
        {
          PRINTF("ERROR: invalid ctrl_state=%d\n", motor->ctrl_state);
          ret = -EINVAL;
          goto errout;
        }
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_motor_handle
 ****************************************************************************/

int foc_motor_handle(FAR struct foc_motor_b16_s *motor,
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

      /* Update motor setpoint */

      ret = foc_motor_setpoint(motor, handle->setpoint);
      if (ret < 0)
        {
          PRINTF("ERROR: foc_motor_setpoint failed %d!\n", ret);
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
