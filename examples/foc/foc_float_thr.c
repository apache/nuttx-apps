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

#include "foc_cfg.h"
#include "foc_debug.h"
#include "foc_motor_f32.h"

#include "industry/foc/foc_utils.h"
#include "industry/foc/foc_common.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_INDUSTRY_FOC_FLOAT
#  error
#endif

/****************************************************************************
 * Private Type Definition
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_handler_run
 ****************************************************************************/

static int foc_handler_run(FAR struct foc_motor_f32_s *motor,
                           FAR struct foc_device_s *dev)
{
  struct foc_handler_input_f32_s  input;
  struct foc_handler_output_f32_s output;
  float                           current[CONFIG_MOTOR_FOC_PHASES];
  int                             ret   = OK;
  int                             i     = 0;

  DEBUGASSERT(motor);
  DEBUGASSERT(dev);

  /* FOC device fault */

  if (motor->fault == true)
    {
      /* Stop motor */

      motor->dq_ref.q   = 0.0f;
      motor->dq_ref.d   = 0.0f;
      motor->angle_now  = 0.0f;
      motor->vbus       = 0.0f;

      /* Force velocity to zero */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
      motor->torq.des = 0.0f;
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
      motor->vel.des = 0.0f;
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
      motor->pos.des = 0.0f;
#endif
    }

  /* Get real currents */

  for (i = 0; i < CONFIG_MOTOR_FOC_PHASES; i += 1)
    {
      current[i] = (motor->iphase_adc * dev->state.curr[i]);
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
      dev->params.duty[i] = FOCDUTY_FROM_FLOAT(output.duty[i]);
    }

  /* Get FOC handler state */

  foc_handler_state_f32(&motor->handler, &motor->foc_state);

  return ret;
}

#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
/****************************************************************************
 * Name: foc_model_state_get
 ****************************************************************************/

static int foc_model_state_get(FAR struct foc_motor_f32_s *motor,
                               FAR struct foc_device_s *dev)
{
  int i = 0;

  DEBUGASSERT(motor);
  DEBUGASSERT(dev);

  /* Get model state */

  foc_model_state_f32(&motor->model, &motor->model_state);

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

static int foc_state_print(FAR struct foc_motor_f32_s *motor)
{
  DEBUGASSERT(motor);

  PRINTF("f32 inst %d:\n", motor->envp->inst);

  foc_handler_state_print_f32(&motor->foc_state);

  return OK;
}
#endif

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
  struct foc_device_s     dev;
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

  /* Initialize FOC device as blocking */

  ret = foc_device_init(&dev, envp->id);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_device_init failed %d!\n", ret);
      goto errout;
    }

  /* Get PWM max duty */

  motor.pwm_duty_max = FOCDUTY_TO_FLOAT(dev.info.hw_cfg.pwm_max);

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

          foc_model_run_f32(&motor.model,
                            FOC_MODEL_LOAD,
                            &motor.foc_state.vab);
#endif

          /* Set FOC device parameters */

          ret = foc_dev_params_set(&dev);
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

  /* De-initialize FOC device */

  ret = foc_device_deinit(&dev);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_device_deinit %d failed %d\n", envp->id, ret);
    }

  PRINTF("foc_float_thr %d exit\n", envp->id);

  return ret;
}
