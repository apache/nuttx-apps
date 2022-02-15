/****************************************************************************
 * apps/industry/foc/fixed16/foc_angle.c
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
#include "industry/foc/fixed16/foc_angle.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_angle_init_b16
 *
 * Description:
 *   Initialize the FOC angle handler (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   ops - pointer to FOC angle handler operations
 *
 ****************************************************************************/

int foc_angle_init_b16(FAR foc_angle_b16_t *h,
                       FAR struct foc_angle_ops_b16_s *ops)
{
  int ret = OK;

  DEBUGASSERT(h);
  DEBUGASSERT(ops);

  /* Angle ops */

  DEBUGASSERT(ops->init);
  DEBUGASSERT(ops->deinit);
  DEBUGASSERT(ops->cfg);
  DEBUGASSERT(ops->run);

  /* Reset handler */

  memset(h, 0, sizeof(foc_angle_b16_t));

  /* Connect ops */

  h->ops = ops;

  /* Initialize angle */

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
 * Name: foc_angle_deinit_b16
 *
 * Description:
 *   De-initialize the FOC angle handler (fixed16)
 *
 * Input Parameter:
 *   h - pointer to FOC angle handler
 *
 ****************************************************************************/

int foc_angle_deinit_b16(FAR foc_angle_b16_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Deinitialize angle */

  h->ops->deinit(h);

  /* Reset data */

  memset(h, 0, sizeof(foc_angle_b16_t));

  return ret;
}

/****************************************************************************
 * Name: foc_angle_cfg_b16
 *
 * Description:
 *   Configure the FOC angle handler (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   cfg - pointer to angle handler configuration data
 *
 ****************************************************************************/

int foc_angle_cfg_b16(FAR foc_angle_b16_t *h, FAR void *cfg)
{
  DEBUGASSERT(h);
  DEBUGASSERT(cfg);

  return h->ops->cfg(h, cfg);
}

/****************************************************************************
 * Name: foc_angle_zero_b16
 *
 * Description:
 *   Reset the FOC angle handler (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *
 ****************************************************************************/

int foc_angle_zero_b16(FAR foc_angle_b16_t *h)
{
  DEBUGASSERT(h);

  return h->ops->zero(h);
}

/****************************************************************************
 * Name: foc_angle_dir_b16
 *
 * Description:
 *   Set the FOC angle handler direction (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *
 ****************************************************************************/

int foc_angle_dir_b16(FAR foc_angle_b16_t *h, b16_t dir)
{
  DEBUGASSERT(h);

  return h->ops->dir(h, dir);
}

/****************************************************************************
 * Name: foc_angle_run_b16
 *
 * Description:
 *   Process the FOC angle handler data (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   in  - pointer to FOC angle handler input data
 *   out - pointer to FOC angle handler output data
 *
 ****************************************************************************/

int foc_angle_run_b16(FAR foc_angle_b16_t *h,
                       FAR struct foc_angle_in_b16_s *in,
                       FAR struct foc_angle_out_b16_s *out)
{
  DEBUGASSERT(h);
  DEBUGASSERT(in);
  DEBUGASSERT(out);

  /* Run angle handler */

  return h->ops->run(h, in, out);
}
