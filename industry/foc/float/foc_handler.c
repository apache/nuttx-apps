/****************************************************************************
 * apps/industry/foc/float/foc_handler.c
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
#include <errno.h>
#include <fcntl.h>

#include "industry/foc/foc_log.h"
#include "industry/foc/foc_common.h"
#include "industry/foc/float/foc_handler.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_handler_init_f32
 *
 * Description:
 *   Initialize the FOC handler (float32)
 *
 * Input Parameter:
 *   h    - pointer to FOC handler
 *   ctrl - pointer to controller operations
 *   mod  - pointer to modulation operations
 *
 ****************************************************************************/

int foc_handler_init_f32(FAR foc_handler_f32_t *h,
                         FAR struct foc_control_ops_f32_s *ctrl,
                         FAR struct foc_modulation_ops_f32_s *mod)
{
  int ret = OK;

  DEBUGASSERT(h);
  DEBUGASSERT(ctrl);
  DEBUGASSERT(mod);

  /* Controller ops */

  DEBUGASSERT(ctrl->init);
  DEBUGASSERT(ctrl->deinit);
  DEBUGASSERT(ctrl->cfg);
  DEBUGASSERT(ctrl->input_set);
  DEBUGASSERT(ctrl->voltage_run);
  DEBUGASSERT(ctrl->current_run);
  DEBUGASSERT(ctrl->state_get);

  /* Modulation ops */

  DEBUGASSERT(mod->init);
  DEBUGASSERT(mod->deinit);
  DEBUGASSERT(mod->cfg);
  DEBUGASSERT(mod->current);
  DEBUGASSERT(mod->vbase_get);
  DEBUGASSERT(mod->run);

  /* Reset handler */

  memset(h, 0, sizeof(foc_handler_f32_t));

  /* Connect ops */

  h->ops.ctrl = ctrl;
  h->ops.mod  = mod;

  /* Initialize control handler */

  ret = h->ops.mod->init(h);
  if (ret < 0)
    {
      FOCLIBERR("ERROR: mod->init failed %d\n", ret);
      goto errout;
    }

  /* Initialize modulation handler */

  ret = h->ops.ctrl->init(h);
  if (ret < 0)
    {
      FOCLIBERR("ERROR: ctrl->init failed %d\n", ret);
      goto errout;
    }

#ifdef CONFIG_INDUSTRY_FOC_CORDIC
  /* Open CORDIC device */

  h->fd = open(CONFIG_INDUSTRY_FOC_CORDIC_DEVPATH, O_RDWR);
  if (h->fd < 0)
    {
      FOCLIBERR("ERROR: failed to open CORDIC device %s %d\n",
                CONFIG_INDUSTRY_FOC_CORDIC_DEVPATH, errno);
      goto errout;
    }
#endif

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_handler_deinit_f32
 *
 * Description:
 *   De-initialize the FOC handler (float32)
 *
 * Input Parameter:
 *   h - pointer to FOC handler
 *
 ****************************************************************************/

int foc_handler_deinit_f32(FAR foc_handler_f32_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Deinitialize modulation handler */

  h->ops.mod->deinit(h);

  /* Deinitialize control handler */

  h->ops.ctrl->deinit(h);

#ifdef CONFIG_INDUSTRY_FOC_CORDIC
  /* Close CORDIC device */

  close(h->fd);
#endif

  /* Reset data */

  memset(h, 0, sizeof(foc_handler_f32_t));

  return ret;
}

/****************************************************************************
 * Name: foc_handler_cfg_f32
 *
 * Description:
 *   Configure the FOC handler (float32)
 *
 * Input Parameter:
 *   h        - pointer to FOC handler
 *   ctrl_cfg - pointer to controller configuration data
 *   mod_cfg  - pointer to modulation configuration data
 *
 ****************************************************************************/

void foc_handler_cfg_f32(FAR foc_handler_f32_t *h,
                         FAR void *ctrl_cfg,
                         FAR void *mod_cfg)
{
  DEBUGASSERT(h);
  DEBUGASSERT(ctrl_cfg);
  DEBUGASSERT(mod_cfg);

  /* Configure controller */

  h->ops.ctrl->cfg(h, ctrl_cfg);

  /* Configure modulation */

  h->ops.mod->cfg(h, mod_cfg);
}

/****************************************************************************
 * Name: foc_handler_run_f32
 *
 * Description:
 *   Run the FOC handler and process the given input data (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC handler
 *   in  - pointer to FOC handler input data
 *   out - pointer to FOC handler output data
 *
 ****************************************************************************/

int foc_handler_run_f32(FAR foc_handler_f32_t *h,
                        FAR struct foc_handler_input_f32_s *in,
                        FAR struct foc_handler_output_f32_s *out)
{
  ab_frame_f32_t v_ab_mod;
  float          vbase = 0;
  int            ret   = OK;

  DEBUGASSERT(h);
  DEBUGASSERT(in);
  DEBUGASSERT(out);

  /* Do nothing if control mode not specified yet.
   * This also protects against initial state when the controller is
   * started but input data has not yet been provided.
   */

  if (in->mode <= FOC_HANDLER_MODE_INIT)
    {
      ret = -EINVAL;
      goto errout;
    }

  /* Correct current samples according to modulation state */

  h->ops.mod->current(h, in->current);

  /* Get VBASE */

  h->ops.mod->vbase_get(h, in->vbus, &vbase);

  /* Feed controller with phase currents */

  h->ops.ctrl->input_set(h, in->current, vbase, in->angle);

  /* Call controller */

  switch (in->mode)
    {
      /* IDLE */

      case FOC_HANDLER_MODE_IDLE:
        {
          /* Do nothing and set duty to zeros */

          ret = OK;
          goto errout;
        }

      /* FOC current mode - control DQ-current */

      case FOC_HANDLER_MODE_CURRENT:
        {
          /* Current controller */

          h->ops.ctrl->current_run(h,
                                   in->dq_ref,
                                   in->vdq_comp,
                                   &v_ab_mod);

          break;
        }

      /* FOC voltage mode - control DQ-voltage */

      case FOC_HANDLER_MODE_VOLTAGE:
        {
          /* Voltage controller */

          h->ops.ctrl->voltage_run(h,
                                   in->dq_ref,
                                   &v_ab_mod);

          break;
        }

      /* Otherwise - we should not be here */

      default:
        {
          ret = -EINVAL;
          goto errout;
        }
    }

  /* Duty cycle modulation */

  h->ops.mod->run(h, &v_ab_mod, out->duty);

  return ret;

errout:

  /* Set duty to zeros */

  memset(out->duty, 0, sizeof(float) * CONFIG_MOTOR_FOC_PHASES);

  return ret;
}

/****************************************************************************
 * Name: foc_handler_state_f32
 *
 * Description:
 *   Get FOC handler state (float32)
 *
 * Input Parameter:
 *   h     - pointer to FOC handler
 *   state - pointer to FOC state data
 *
 ****************************************************************************/

void foc_handler_state_f32(FAR foc_handler_f32_t *h,
                           FAR struct foc_state_f32_s *state)
{
  DEBUGASSERT(h);
  DEBUGASSERT(state);

  h->ops.ctrl->state_get(h, state);
}

#ifdef CONFIG_INDUSTRY_FOC_HANDLER_PRINT
/****************************************************************************
 * Name: foc_handler_state_print_f32
 *
 * Description:
 *   Print FOC handler state (float32)
 *
 * Input Parameter:
 *   state - pointer to FOC state data
 *
 ****************************************************************************/

void foc_handler_state_print_f32(FAR struct foc_state_f32_s *state)
{
  DEBUGASSERT(state);

#if CONFIG_MOTOR_FOC_PHASES == 3
  FOCLIBLOG("curr = [%.2f %.2f %.2f]\n",
            state->curr[0],
            state->curr[1],
            state->curr[2]);
  FOCLIBLOG("volt = [%.2f %.2f %.2f]\n",
            state->volt[0],
            state->volt[1],
            state->volt[2]);
#else
#  error TODO
#endif

  FOCLIBLOG("iab = [%.2f %.2f]\n", state->iab.a,
            state->iab.b);
  FOCLIBLOG("vab = [%.2f %.2f]\n", state->vab.a,
            state->vab.b);
  FOCLIBLOG("idq = [%.2f %.2f]\n", state->idq.d,
            state->idq.q);
  FOCLIBLOG("vdq = [%.2f %.2f]\n", state->vdq.d,
            state->vdq.q);
}
#endif
