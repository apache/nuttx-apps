/****************************************************************************
 * apps/industry/foc/fixed16/foc_routine.c
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
#include "industry/foc/fixed16/foc_routine.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_routine_init_b16
 *
 * Description:
 *   Initialize the FOC routine (fixed16)
 *
 * Input Parameter:
 *   r   - pointer to FOC routine
 *   ops - pointer to FOC routine operations
 *
 ****************************************************************************/

int foc_routine_init_b16(FAR foc_routine_b16_t *r,
                         FAR struct foc_routine_ops_b16_s *ops)
{
  int ret = OK;

  DEBUGASSERT(r);
  DEBUGASSERT(ops);

  /* Routine ops */

  DEBUGASSERT(ops->init);
  DEBUGASSERT(ops->deinit);
  DEBUGASSERT(ops->cfg);
  DEBUGASSERT(ops->run);

  /* Reset routine */

  memset(r, 0, sizeof(foc_routine_b16_t));

  /* Connect ops */

  r->ops = ops;

  /* Initialize routine */

  ret = r->ops->init(r);
  if (ret < 0)
    {
      FOCLIBERR("ERROR: ops->init failed %d\n", ret);
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_routine_deinit_b16
 *
 * Description:
 *   De-initialize the FOC routine (fixed16)
 *
 * Input Parameter:
 *   r - pointer to FOC routine
 *
 ****************************************************************************/

int foc_routine_deinit_b16(FAR foc_routine_b16_t *r)
{
  int ret = OK;

  DEBUGASSERT(r);

  /* Deinitialize routine */

  r->ops->deinit(r);

  /* Reset data */

  memset(r, 0, sizeof(foc_routine_b16_t));

  return ret;
}

/****************************************************************************
 * Name: foc_routine_cfg_b16
 *
 * Description:
 *   Configure the FOC routine (fixed16)
 *
 * Input Parameter:
 *   r   - pointer to FOC routine
 *   cfg - pointer to routine configuration data
 *
 ****************************************************************************/

int foc_routine_cfg_b16(FAR foc_routine_b16_t *r, FAR void *cfg)
{
  DEBUGASSERT(r);
  DEBUGASSERT(cfg);

  return r->ops->cfg(r, cfg);
}

/****************************************************************************
 * Name: foc_routine_run_b16
 *
 * Description:
 *   Run the FOC routine (fixed16)
 *
 * Input Parameter:
 *   r   - pointer to FOC routine
 *   in  - pointer to FOC routine input data
 *   out - pointer to FOC routine output data
 *
 ****************************************************************************/

int foc_routine_run_b16(FAR foc_routine_b16_t *r,
                        FAR struct foc_routine_in_b16_s *in,
                        FAR struct foc_routine_out_b16_s *out)
{
  DEBUGASSERT(r);
  DEBUGASSERT(in);
  DEBUGASSERT(out);

  /* Run routine handler */

  return r->ops->run(r, in, out);
}

/****************************************************************************
 * Name: foc_routine_final_b16
 *
 * Description:
 *   Finalize the FOC routine data (fixed16)
 *
 * Input Parameter:
 *   r    - pointer to FOC routine
 *   data - pointer to FOC routine final data
 *
 ****************************************************************************/

int foc_routine_final_b16(FAR foc_routine_b16_t *r, FAR void *data)
{
  DEBUGASSERT(r);
  DEBUGASSERT(data);

  /* Finalize routine */

  return r->ops->final(r, data);
}
