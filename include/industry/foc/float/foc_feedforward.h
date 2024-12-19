/****************************************************************************
 * apps/include/industry/foc/float/foc_feedforward.h
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

#ifndef __INDUSTRY_FOC_FLOAT_FOC_FEEDFORWARD_H
#define __INDUSTRY_FOC_FLOAT_FOC_FEEDFORWARD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <dsp.h>

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

int foc_feedforward_pmsm_f32(FAR struct motor_phy_params_f32_s *phy,
                             FAR dq_frame_f32_t *idq,
                             float vel_now,
                             FAR dq_frame_f32_t *vdq_comp);

#endif /* __INDUSTRY_FOC_FLOAT_FOC_FEEDFORWARD_H */
