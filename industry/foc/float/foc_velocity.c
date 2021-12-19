/****************************************************************************
 * apps/industry/foc/float/foc_velocity.c
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
#include "industry/foc/float/foc_velocity.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_velocity_init_f32
 *
 * Description:
 *   Initialize the FOC velocity handler (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC velocity handler
 *   ops - pointer to FOC velocity handler operations
 *
 ****************************************************************************/

int foc_velocity_init_f32(FAR foc_velocity_f32_t *h,
                          FAR struct foc_velocity_ops_f32_s *ops)
{
  int ret = OK;

  DEBUGASSERT(h);
  DEBUGASSERT(ops);

  /* Velocity ops */

  DEBUGASSERT(ops->init);
  DEBUGASSERT(ops->deinit);
  DEBUGASSERT(ops->cfg);
  DEBUGASSERT(ops->run);

  /* Reset handler */

  memset(h, 0, sizeof(foc_velocity_f32_t));

  /* Connect ops */

  h->ops = ops;

  /* Initialize velocity */

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
 * Name: foc_velocity_deinit_f32
 *
 * Description:
 *   De-initialize the FOC velocity handler (float32)
 *
 * Input Parameter:
 *   h - pointer to FOC velocity handler
 *
 ****************************************************************************/

int foc_velocity_deinit_f32(FAR foc_velocity_f32_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Deinitialize velocity */

  h->ops->deinit(h);

  /* Reset data */

  memset(h, 0, sizeof(foc_velocity_f32_t));

  return ret;
}

/****************************************************************************
 * Name: foc_velocity_cfg_f32
 *
 * Description:
 *   Configure the FOC velocity handler (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC velocity handler
 *   cfg - pointer to velocity handler configuration data
 *
 ****************************************************************************/

int foc_velocity_cfg_f32(FAR foc_velocity_f32_t *h, FAR void *cfg)
{
  DEBUGASSERT(h);
  DEBUGASSERT(cfg);

  return h->ops->cfg(h, cfg);
}

/****************************************************************************
 * Name: foc_velocity_zero_f32
 *
 * Description:
 *   Zero the FOC velocity handler (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC velocity handler
 *
 ****************************************************************************/

int foc_velocity_zero_f32(FAR foc_velocity_f32_t *h)
{
  DEBUGASSERT(h);

  return h->ops->zero(h);
}

/****************************************************************************
 * Name: foc_velocity_dir_f32
 *
 * Description:
 *   Set the FOC velocity handler direction (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC velocity handler
 *   dir - sensor direction (1 if normal -1 if inverted)
 *
 ****************************************************************************/

int foc_velocity_dir_f32(FAR foc_velocity_f32_t *h, float dir)
{
  DEBUGASSERT(h);

  return h->ops->dir(h, dir);
}

/****************************************************************************
 * Name: foc_velocity_run_f32
 *
 * Description:
 *   Process the FOC velocity handler data (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC velocity handler
 *   in  - pointer to FOC velocity handler input data
 *   out - pointer to FOC velocity handler output data
 *
 ****************************************************************************/

int foc_velocity_run_f32(FAR foc_velocity_f32_t *h,
                         FAR struct foc_velocity_in_f32_s *in,
                         FAR struct foc_velocity_out_f32_s *out)
{
  DEBUGASSERT(h);
  DEBUGASSERT(in);
  DEBUGASSERT(out);

  /* Run velocity handler */

  return h->ops->run(h, in, out);
}
