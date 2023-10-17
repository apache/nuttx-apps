/****************************************************************************
 * apps/industry/foc/fixed16/foc_ramp.c
 * This file implements ramp for fixed16
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
#include "industry/foc/fixed16/foc_ramp.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_ramp_init_b16
 *
 * Description:
 *   Initialize ramp (fixed16)
 *
 * Input Parameter:
 *   ramp - pointer to ramp handler
 *   per  - ramp period
 *   thr  - ramp threshold
 *   acc  - ramp acceleration
 *   dec  - ramp decceleration
 *
 ****************************************************************************/

int foc_ramp_init_b16(FAR struct foc_ramp_b16_s *ramp, b16_t per,
                      b16_t thr, b16_t acc, b16_t dec)
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
  ramp->ramp_acc_per = b16mulb16(ramp->ramp_acc, ramp->per);
  ramp->ramp_dec_per = b16mulb16(ramp->ramp_dec, ramp->per);

  return OK;
}

/****************************************************************************
 * Name: foc_ramp_run_b16
 *
 * Description:
 *   Handle ramp (fixed16)
 *
 * Input Parameter:
 *   ramp - pointer to ramp handler
 *   des  - ramp parameter destination
 *   now  - ramp parameter now
 *   set  - (out) ramp parameter set
 *
 ****************************************************************************/

int foc_ramp_run_b16(FAR struct foc_ramp_b16_s *ramp, b16_t des,
                     b16_t now, FAR b16_t *set)
{
  b16_t sign = b16ONE;

  DEBUGASSERT(ramp);

  /* Check if we require soft start/stop operation.
   * Only if the user set point differs from the driver set point
   */

  if (des != *set)
    {
      ramp->diff = des - *set;

      /* If we change direction then the first vel target is 0 */

      if (des * (*set) < 0)
        {
          ramp->diff = -now;
          des        = 0;
        }

      if (now >= 0 && ramp->diff >= ramp->ramp_thr)
        {
          /* Soft start in CW direction */

          sign            = b16ONE;
          ramp->ramp_mode = RAMP_MODE_SOFTSTART;
        }
      else if (now >= 0 && ramp->diff <= -ramp->ramp_thr)
        {
          /* Soft stop in CW direction */

          sign            = -b16ONE;
          ramp->ramp_mode = RAMP_MODE_SOFTSTOP;
        }
      else if (now < 0 && ramp->diff >= ramp->ramp_thr)
        {
          /* Soft stop in CCW direction */

          sign            = b16ONE;
          ramp->ramp_mode = RAMP_MODE_SOFTSTOP;
        }
      else if (now < 0 && ramp->diff <= -ramp->ramp_thr)
        {
          /* Soft start in CCW direction */

          sign            = -b16ONE;
          ramp->ramp_mode = RAMP_MODE_SOFTSTART;
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
          /* Increase setpoint with ramp */

          *set = now + b16mulb16(sign, ramp->ramp_acc_per);

          break;
        }

      case RAMP_MODE_SOFTSTOP:
        {
          /* Stop motor with ramp */

          *set = now + b16mulb16(sign, ramp->ramp_dec_per);

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
