/****************************************************************************
 * apps/industry/foc/fixed16/foc_model.c
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

#include "industry/foc/foc_log.h"
#include "industry/foc/foc_common.h"
#include "industry/foc/fixed16/foc_model.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_model_init_b16
 *
 * Description:
 *   Initialize the FOC model (fixed16)
 *
 * Input Parameter:
 *   h    - pointer to FOC model handler
 *   ops  - pointer to FOC model operations
 *
 ****************************************************************************/

int foc_model_init_b16(FAR foc_model_b16_t *h,
                       FAR struct foc_model_ops_b16_s *ops)
{
  int ret = OK;

  DEBUGASSERT(h);
  DEBUGASSERT(ops);

  /* Model ops */

  DEBUGASSERT(ops->init);
  DEBUGASSERT(ops->deinit);
  DEBUGASSERT(ops->cfg);
  DEBUGASSERT(ops->ele_run);
  DEBUGASSERT(ops->mech_run);
  DEBUGASSERT(ops->state);

  /* Reset handler */

  memset(h, 0, sizeof(foc_model_b16_t));

  /* Connect ops */

  h->ops = ops;

  /* Initialize model */

  ret = h->ops->init(h);
  if (ret < 0)
    {
      FOCLIBERR("ERROR: ops->init failed %d\n", ret);
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_model_deinit_b16
 *
 * Description:
 *   De-initialize the FOC model (fixed16)
 *
 * Input Parameter:
 *   h    - pointer to FOC model handler
 *
 ****************************************************************************/

int foc_model_deinit_b16(FAR foc_model_b16_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Deinitialize model */

  h->ops->deinit(h);

  /* Reset data */

  memset(h, 0, sizeof(foc_model_b16_t));

  return ret;
}

/****************************************************************************
 * Name: foc_model_cfg_b16
 *
 * Description:
 *   Configure the FOC model (fixed16)
 *
 * Input Parameter:
 *   h    - pointer to FOC model handler
 *   cfg  - pointer to FOC model configuration data
 *
 ****************************************************************************/

int foc_model_cfg_b16(FAR foc_model_b16_t *h, FAR void *cfg)
{
  DEBUGASSERT(h);
  DEBUGASSERT(cfg);

  return h->ops->cfg(h, cfg);
}

/****************************************************************************
 * Name: foc_model_run_b16
 *
 * Description:
 *   Run the FOC model (fixed16)
 *
 * Input Parameter:
 *   h    - pointer to FOC model handler
 *   load - applied load
 *   v_ab - applied voltage in alpha-beta frame
 *
 * REVISIT:
 *   It would be better if we feed model with duty cycle and VBUS, but it
 *   complicates the calculations, so we leave it like that for now
 *
 ****************************************************************************/

void foc_model_run_b16(FAR foc_model_b16_t *h,
                       b16_t load,
                       FAR ab_frame_b16_t *v_ab)
{
  DEBUGASSERT(h);
  DEBUGASSERT(v_ab);

  /* Run electrical model */

  h->ops->ele_run(h, v_ab);

  /* Run mechanical model */

  h->ops->mech_run(h, load);
}

/****************************************************************************
 * Name: foc_model_state_b16
 *
 * Description:
 *   Get model state (fixed16)
 *
 * Input Parameter:
 *   h     - pointer to FOC model handler
 *   state - pointer to FOC model state
 *
 ****************************************************************************/

void foc_model_state_b16(FAR foc_model_b16_t *h,
                         FAR struct foc_model_state_b16_s *state)
{
  DEBUGASSERT(h);
  DEBUGASSERT(state);

  h->ops->state(h, state);
}
