/****************************************************************************
 * apps/industry/foc/float/foc_feedforward.c
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

#include "industry/foc/float/foc_feedforward.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_feedforward_pmsm_f32
 *
 * Description:
 *   Feed forward compensation for PMSM (float32)
 *
 * Input Parameter:
 *   phy      - motor physical parameters
 *   idq      - iqd frame
 *   vel_now  - electrical velocity
 *   vdq_comp - compensation vdq frame
 *
 ****************************************************************************/

int foc_feedforward_pmsm_f32(FAR struct motor_phy_params_f32_s *phy,
                             FAR dq_frame_f32_t *idq,
                             float vel_now,
                             FAR dq_frame_f32_t *vdq_comp)
{
  DEBUGASSERT(phy);
  DEBUGASSERT(idq);
  DEBUGASSERT(vdq_comp);

  /* NOTE: vdq_comp is substracted from vdq_ref in foc_current_control()
   * so vq compensation must be negative here.
   */

  vdq_comp->q = -vel_now * (phy->flux_link + phy->ind * idq->d);
  vdq_comp->d = vel_now * phy->ind * idq->q;

  return OK;
}
