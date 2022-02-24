/****************************************************************************
 * apps/examples/foc/foc_motor_b16.c
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

#include "foc_motor_b16.h"

#include "foc_cfg.h"
#include "foc_debug.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Definition
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

      PRINTF("Aling results:\n");
      PRINTF("  dir    = %.2f\n", b16tof(final.dir));
      PRINTF("  offset = %.2f\n", b16tof(final.offset));

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

  switch (motor->envp->fmode)
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
          PRINTF("ERROR: unsupported op mode %d\n", motor->envp->fmode);
          ret = -EINVAL;
          goto errout;
        }
    }

  /* Force open-loop if sensorless */

#ifdef CONFIG_EXAMPLES_FOC_SENSORLESS
#  ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  motor->openloop_now = true;
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
#ifdef CONFIG_INDUSTRY_FOC_MODULATION_SVM3
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
                          ftob16(RAMP_CFG_ACC),
                          ftob16(RAMP_CFG_ACC));
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

#ifdef CONFIG_INDUSTRY_FOC_MODULATION_SVM3
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

  ctrl_cfg.id_kp = ftob16(motor->envp->foc_pi_kp / 1000.0f);
  ctrl_cfg.id_ki = ftob16(motor->envp->foc_pi_ki / 1000.0f);
  ctrl_cfg.iq_kp = ftob16(motor->envp->foc_pi_kp / 1000.0f);
  ctrl_cfg.iq_ki = ftob16(motor->envp->foc_pi_ki / 1000.0f);
#endif

#ifdef CONFIG_INDUSTRY_FOC_MODULATION_SVM3
  /* Get SVM3 modulation configuration */

  mod_cfg.pwm_duty_max = motor->pwm_duty_max;
#endif

  /* Configure FOC handler */

  foc_handler_cfg_b16(&motor->handler, &ctrl_cfg, &mod_cfg);

#ifdef CONFIG_EXAMPLES_FOC_MOTOR_POLES
  /* Configure motor poles */

  motor->poles = CONFIG_EXAMPLES_FOC_MOTOR_POLES;
#endif

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

  tmp1 = itob16(motor->envp->torqmax / 1000);
  tmp2 = b16mulb16(ftob16(SETPOINT_ADC_SCALE), tmp1);

  motor->torq.des = b16muli(tmp2, torq);

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

  tmp1 = itob16(motor->envp->velmax / 1000);
  tmp2 = b16mulb16(ftob16(SETPOINT_ADC_SCALE), tmp1);

  motor->vel.des = b16muli(tmp2, vel);

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

  tmp1 = itob16(motor->envp->posmax / 1000);
  tmp2 = b16mulb16(ftob16(SETPOINT_ADC_SCALE), tmp1);

  motor->pos.des = b16muli(tmp2, pos);

  return OK;
}
#endif

/****************************************************************************
 * Name: foc_motor_setpoint
 ****************************************************************************/

static int foc_motor_setpoint(FAR struct foc_motor_b16_s *motor, uint32_t sp)
{
  int ret = OK;

  switch (motor->envp->mmode)
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

      default:
        {
          PRINTF("ERROR: unsupported ctrl mode %d\n", motor->envp->mmode);
          ret = -EINVAL;
          goto errout;
        }
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_motor_state
 ****************************************************************************/

static int foc_motor_state(FAR struct foc_motor_b16_s *motor, int state)
{
  int ret = OK;

  DEBUGASSERT(motor);

  /* Update motor state */

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
          motor->dir = DIR_NONE_B16;

          /* DQ vector not zero - active brake */

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
  /* Empty for now */

  return OK;
}

/****************************************************************************
 * Name: foc_motor_run
 ****************************************************************************/

static int foc_motor_run(FAR struct foc_motor_b16_s *motor)
{
  b16_t q_ref = 0;
  b16_t d_ref = 0;
  int   ret   = OK;

  DEBUGASSERT(motor);

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  /* Open-loop works only in velocity control mode */

  if (motor->openloop_now == true)
    {
#  ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
      if (motor->envp->mmode != FOC_MMODE_VEL)
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

  /* Controller */

  switch (motor->envp->mmode)
    {
#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
      case FOC_MMODE_TORQ:
        {
          motor->torq.set = b16mulb16(motor->dir, motor->torq.des);

          q_ref = motor->torq.set;
          d_ref = 0;

          break;
        }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
      case FOC_MMODE_VEL:
        {
          /* Run velocity ramp controller */

          ret = foc_ramp_run_b16(&motor->ramp, motor->vel.des,
                                 motor->vel.now, &motor->vel.set);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_ramp_run failed %d\n", ret);
              goto errout;
            }

          break;
        }
#endif

      default:
        {
          ret = -EINVAL;
          goto errout;
        }
    }

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  /* Force open-loop current */

  if (motor->openloop_now == true)
    {
      /* Get open-loop currents
       * NOTE: Id always set to 0
       */

      motor->dq_ref.q = b16idiv(motor->envp->qparam, 1000);
      motor->dq_ref.d = 0;
    }
#endif

  /* Set DQ reference frame */

  motor->dq_ref.q = q_ref;
  motor->dq_ref.d = d_ref;

  /* DQ compensation */

  motor->vdq_comp.q = 0;
  motor->vdq_comp.d = 0;

errout:
  return ret;
}
#endif

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
#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
  struct foc_routine_align_cfg_b16_s align_cfg;
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
  motor->iphase_adc = ftob16((CONFIG_EXAMPLES_FOC_IPHASE_ADC) / 100000.0f);

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

  sprintf(motor->qedpath,
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

  sprintf(motor->hldpath,
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
  align_cfg.offset_steps = (CONFIG_EXAMPLES_FOC_NOTIFIER_FREQ *     \
                            CONFIG_EXAMPLES_FOC_ALIGN_SEC / 1000);

  /* Connect align callbacks */

  align_cfg.cb.zero = foc_align_zero_cb;
  align_cfg.cb.dir  = foc_align_dir_cb;

  /* Connect align callbacks private data */

#  ifdef CONFIG_EXAMPLES_FOC_HAVE_QENCO
  align_cfg.cb.priv = &motor->qenco;
#  endif
#  ifdef CONFIG_EXAMPLES_FOC_HAVE_HALL
  align_cfg.cb.priv = &motor->hall;
#  endif

  ret = foc_routine_cfg_b16(&motor->align, &align_cfg);
  if (ret < 0)
    {
      PRINTFV("ERROR: foc_routine_cfg_b16 failed %d!\n", ret);
      goto errout;
    }
#endif

  /* Initialize controller state */

  motor->ctrl_state = FOC_CTRL_STATE_INIT;

#if defined(CONFIG_EXAMPLES_FOC_SENSORED) || defined(CONFIG_EXAMPLES_FOC_HAVE_RUN)
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
  struct foc_angle_in_b16_s  ain;
  struct foc_angle_out_b16_s aout;
  int                        ret = OK;

  DEBUGASSERT(motor);

  /* Update open-loop angle handler */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  ain.vel   = motor->vel.set;
#endif
  ain.angle = motor->angle_now;
  ain.dir   = motor->dir;

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  foc_angle_run_b16(&motor->openloop, &ain, &aout);

  /* Store open-loop angle */

  motor->angle_ol = aout.angle;
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

#ifdef CONFIG_EXAMPLES_FOC_SENSORED
  /* Handle angle from sensor */

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
                                 motor->poles) % MOTOR_ANGLE_E_MAX_B16);
    }

  else
    {
      ASSERT(0);
    }
#endif  /* CONFIG_EXAMPLES_FOC_SENSORED */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  /* Get phase angle now */

  if (motor->openloop_now == true)
    {
      motor->angle_now = motor->angle_ol;
    }
  else
#endif
    {
      /* Get phase angle from observer or sensor */

      motor->angle_now = motor->angle_el;
    }

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  if (motor->openloop_now == true)
    {
      /* No velocity feedback - assume that velocity now is velocity set */

      motor->vel.now = motor->vel.set;
    }
  else
#endif
    {
      /* TODO: velocity observer or sensor */
    }

#ifdef CONFIG_EXAMPLES_FOC_SENSORED
errout:
#endif
  return ret;
}

/****************************************************************************
 * Name: foc_motor_control
 ****************************************************************************/

int foc_motor_control(FAR struct foc_motor_b16_s *motor)
{
  int ret = OK;
#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
  bool align_done = false;
#endif

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

          ret = foc_motor_align(motor, &align_done);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_motor_align failed %d!\n", ret);
              goto errout;
            }

          if (align_done == true)
            {
              /* Next state */

              motor->ctrl_state += 1;
              motor->foc_mode = FOC_HANDLER_MODE_IDLE;
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
