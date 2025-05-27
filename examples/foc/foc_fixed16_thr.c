/****************************************************************************
 * apps/examples/foc/foc_fixed16_thr.c
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
#include <dspb16.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "foc_cfg.h"
#include "foc_debug.h"
#include "foc_motor_b16.h"

#include "industry/foc/foc_utils.h"
#include "industry/foc/foc_common.h"

#ifdef CONFIG_EXAMPLES_FOC_NXSCOPE
#  include "logging/nxscope/nxscope.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_INDUSTRY_FOC_FIXED16
#  error
#endif

/* Critical section */

#ifdef CONFIG_EXAMPLES_FOC_CONTROL_CRITSEC
#  define foc_enter_critical() irqstate_t intflags = enter_critical_section()
#  define foc_leave_critical() leave_critical_section(intflags)
#else
#  define foc_enter_critical()
#  define foc_leave_critical()
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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

      /* Force setpoints to zero */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
      motor->torq.des = 0;
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
      motor->vel.des = 0;
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
      motor->pos.des = 0;
#endif
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

  foc_handler_state_b16(&motor->handler,
                        &motor->foc_state,
                        &motor->mod_state);

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

#ifdef CONFIG_EXAMPLES_FOC_NXSCOPE
/****************************************************************************
 * Name: foc_fixed16_nxscope
 ****************************************************************************/

static void foc_fixed16_nxscope(FAR struct foc_nxscope_s *nxs,
                                FAR struct foc_motor_b16_s *motor,
                                FAR struct foc_device_s *dev)
{
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG != 0)
  FAR b16_t *ptr = NULL;
  int        i = nxs->ch_per_inst * motor->envp->id;
#endif

  nxscope_lock(&nxs->nxs);

#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_IABC)
  ptr = (FAR b16_t *)&motor->foc_state.curr;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, CONFIG_MOTOR_FOC_PHASES);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_IDQ)
  ptr = (FAR b16_t *)&motor->foc_state.idq;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 2);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_IAB)
  ptr = (FAR b16_t *)&motor->foc_state.iab;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 2);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VABC)
  ptr = (FAR b16_t *)&motor->foc_state.volt;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, CONFIG_MOTOR_FOC_PHASES);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VDQ)
  ptr = (FAR b16_t *)&motor->foc_state.vdq;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 2);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VAB)
  ptr = (FAR b16_t *)&motor->foc_state.vab;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 2);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_AEL)
  ptr = (FAR b16_t *)&motor->angle_el;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 1);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_AM)
  ptr = (FAR b16_t *)&motor->angle_m;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 1);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VEL)
  ptr = (FAR b16_t *)&motor->vel_el;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 1);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VM)
  ptr = (FAR b16_t *)&motor->vel_mech;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 1);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VBUS)
  ptr = (FAR b16_t *)&motor->vbus;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 1);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_SPTORQ)
  ptr = (FAR b16_t *)&motor->torq;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 3);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_SPVEL)
  ptr = (FAR b16_t *)&motor->vel;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 3);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_SPPOS)
  ptr = (FAR b16_t *)&motor->pos;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 3);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_DQREF)
  ptr = (FAR b16_t *)&motor->dq_ref;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 2);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VDQCOMP)
  ptr = (FAR b16_t *)&motor->vdq_comp;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 2);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_SVM3)
  b16_t svm3_tmp[4];

  /* Convert sector to b16_t.
   * Normally, a sector value is an integer in the range 1-6 but we convert
   * it to b16_t and range to 0.1-0.6. This is to send the entire SVM3 state
   * as b16_t array and scale the sector value closer to PWM duty values
   * (range 0.0 to 0.5) which makes it easier to visualize the data later.
   */

  svm3_tmp[0] = b16mulb16(itob16(motor->mod_state.sector), ftob16(0.1f));
  svm3_tmp[1] = motor->mod_state.d_u;
  svm3_tmp[2] = motor->mod_state.d_v;
  svm3_tmp[3] = motor->mod_state.d_w;

  ptr = svm3_tmp;
  nxscope_put_vb16(&nxs->nxs, i++, ptr, 4);
#endif

  nxscope_unlock(&nxs->nxs);
}
#endif

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
  int                     ret  = OK;

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

  /* Store data from device */

  motor.pwm_duty_max = FOCDUTY_TO_FIXED16(dev.info.hw_cfg.pwm_max);
  motor.iphase_adc = b16idiv(dev.info.hw_cfg.iphase_scale, 100000);

  /* Start with motor free */

  handle.app_state = FOC_EXAMPLE_STATE_FREE;

  /* Wait some time */

  usleep(1000);

  /* Control loop */

  while (motor.mq.quit == false)
    {
      foc_enter_critical();

      if (motor.mq.start == true)
        {
          /* Get FOC device state */

          ret = foc_dev_state_get(&dev);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_dev_state_get failed %d!\n", ret);
              goto errout;
            }
        }

      PRINTFV("foc_fixed16_thr %d %d\n", envp->id, motor.time);

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

          /* Start from the beginning of the control loop */

          foc_leave_critical();
          continue;
        }

      /* Ignore control logic if controller not started yet */

      if (motor.mq.start == false)
        {
          foc_leave_critical();
          usleep(1000);
          continue;
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

      if (motor.time % FOC_STATE_PRINT_PRE == 0)
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

#ifdef CONFIG_EXAMPLES_FOC_NXSCOPE
      /* Capture nxscope samples */

      if (motor.time % CONFIG_EXAMPLES_FOC_NXSCOPE_PRESCALER == 0)
        {
          foc_fixed16_nxscope(envp->nxs, &motor, &dev);
        }
#endif

#ifdef CONFIG_EXAMPLES_FOC_NXSCOPE_CONTROL
      /* Handle nxscope work */

      if (motor.time % CONFIG_EXAMPLES_FOC_NXSCOPE_WORK_PRESCALER == 0)
        {
          foc_nxscope_work(envp->nxs);
        }
#endif

      /* Terminate control thread */

      if (motor.ctrl_state == FOC_CTRL_STATE_TERMINATE)
        {
          PRINTF("TERMINATE CTRL THREAD\n");
          goto errout;
        }

      /* Increase counter */

      motor.time += 1;

      foc_leave_critical();
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

#ifdef CONFIG_EXAMPLES_FOC_PERF_EXIT
  /* Print final perf stats */

  foc_perf_exit(&dev.perf);
#endif

  PRINTF("foc_fixed16_thr %d exit\n", envp->id);

  return ret;
}
