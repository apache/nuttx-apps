/****************************************************************************
 * apps/industry/foc/float/foc_cordic.c
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

#include "industry/foc/float/foc_cordic.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_cordic_dqsat_f32
 *
 * Description:
 *   CORDIC DQ-frame saturation (float32)
 *
 * Input Parameter:
 *   fd     - the file descriptor for CORDIC device
 *   dq_ref - DQ vector
 *   mag_max - vector magnitude max
 *
 ****************************************************************************/

int foc_cordic_dqsat_f32(int fd, FAR dq_frame_f32_t *dq, float mag_max)
{
  DEBUGASSERT(dq);

  /* TODO: */

  ASSERT(0);

  return OK;
}

/****************************************************************************
 * Name: foc_cordic_angle_f32
 *
 * Description:
 *   CORDIC angle update (float32)
 *
 * Input Parameter:
 *   fd    - the file descriptor for CORDIC device
 *   angle - phase angle data
 *   a     - phase angle in rad
 *
 ****************************************************************************/

int foc_cordic_angle_f32(int fd, FAR phase_angle_f32_t *angle, float a)
{
  DEBUGASSERT(angle);

  /* TODO: */

  ASSERT(0);

  return OK;
}
