/****************************************************************************
 * apps/industry/foc/fixed16/foc_feedforward.c
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

#include "industry/foc/fixed16/foc_feedforward.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_feedforward_pmsm_b16
 *
 * Description:
 *   Feed forward compensation for PMSM (fixed16)
 *
 * Input Parameter:
 *   phy      - motor physical parameters
 *   idq      - iqd frame
 *   vel_now  - electrical velocity
 *   vdq_comp - compensation vdq frame
 *
 ****************************************************************************/

int foc_feedforward_pmsm_b16(FAR struct motor_phy_params_b16_s *phy,
                             FAR dq_frame_b16_t *idq,
                             b16_t vel_now,
                             FAR dq_frame_b16_t *vdq_comp)
{
  DEBUGASSERT(phy);
  DEBUGASSERT(idq);
  DEBUGASSERT(vdq_comp);

  /* NOTE: vdq_comp is substracted from vdq_ref in foc_current_control()
   * so vq compensation must be negative here.
   */

  vdq_comp->q = -b16mulb16(vel_now,
                           (phy->flux_link + b16mulb16(phy->ind,
                                                       idq->d)));
  vdq_comp->d = b16mulb16(b16mulb16(vel_now, phy->ind), idq->q);

  return OK;
}
