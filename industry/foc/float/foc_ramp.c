/****************************************************************************
 * apps/industry/foc/float/foc_ramp.c
 * This file implements ramp for float32
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

#include "industry/foc/foc_common.h"
#include "industry/foc/float/foc_ramp.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_ramp_init_f32
 *
 * Description:
 *   Initialize ramp (float32)
 *
 * Input Parameter:
 *   ramp - pointer to ramp handler
 *   per  - ramp period
 *   thr  - ramp threshold
 *   acc  - ramp acceleration
 *   dec  - ramp decceleration
 *
 ****************************************************************************/

int foc_ramp_init_f32(FAR struct foc_ramp_f32_s *ramp, float per,
                      float thr, float acc, float dec)
{
  DEBUGASSERT(ramp);
  DEBUGASSERT(per > 0);
  DEBUGASSERT(thr > 0);
  DEBUGASSERT(acc > 0);
  DEBUGASSERT(dec > 0);

  ramp->per          = per;
  ramp->ramp_thr     = thr;
  ramp->ramp_acc     = acc;
  ramp->ramp_dec     = dec;
  ramp->ramp_acc_per = (ramp->ramp_acc * ramp->per);
  ramp->ramp_dec_per = (ramp->ramp_dec * ramp->per);

  return OK;
}

/****************************************************************************
 * Name: foc_ramp_run_f32
 *
 * Description:
 *   Handle ramp (float32)
 *
 * Input Parameter:
 *   ramp - pointer to ramp handler
 *   des  - ramp parameter destination
 *   now  - ramp parameter now
 *   set  - (out) ramp parameter set
 *
 ****************************************************************************/

int foc_ramp_run_f32(FAR struct foc_ramp_f32_s *ramp, float des,
                     float now, FAR float *set)
{
  DEBUGASSERT(ramp);

  /* Check if we require soft start/stop operation.
   * Only if the user set point differs from the driver set point
   */

  if (des != *set)
    {
      ramp->diff = des - *set;

      if (ramp->diff >= ramp->ramp_thr)
        {
          ramp->ramp_mode = RAMP_MODE_SOFTSTART;
        }
      else if (ramp->diff <= -ramp->ramp_thr)
        {
          ramp->ramp_mode = RAMP_MODE_SOFTSTOP;
        }
      else
        {
          /* Just set new setpoint */

          *set            = des;
          ramp->ramp_mode = RAMP_MODE_NORMAL;
        }
    }
  else
    {
      ramp->ramp_mode = RAMP_MODE_NORMAL;
    }

  /* Handle according to current motor state */

  switch (ramp->ramp_mode)
    {
      case RAMP_MODE_NORMAL:
        {
          /* Nothing to do here ? */

          break;
        }

      case RAMP_MODE_SOFTSTART:
        {
          if (des - *set >= ramp->ramp_thr)
            {
              /* Increase setpoin with ramp */

              *set = now + ramp->ramp_acc_per;
            }
          else
            {
              /* Set final setpoint and exit soft start */

              *set            = des;
              ramp->ramp_mode = RAMP_MODE_NORMAL;
            }

          break;
        }

      case RAMP_MODE_SOFTSTOP:
        {
          if (des - *set <= -ramp->ramp_thr)
            {
              /* Stop motor with ramp */

              *set = now - ramp->ramp_dec_per;
            }
          else
            {
              /* Set final setpint and exit soft stop */

              *set            = des;
              ramp->ramp_mode = RAMP_MODE_NORMAL;
            }

          break;
        }

      default:
        {
          /* We should not be here */

          DEBUGASSERT(0);
          break;
        }
    }

  return OK;
}
