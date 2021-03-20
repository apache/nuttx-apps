/****************************************************************************
 * apps/include/industry/foc/float/foc_ramp.h
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

#ifndef __INDUSTRY_FOC_FLOAT_FOC_RAMP_H
#define __INDUSTRY_FOC_FLOAT_FOC_RAMP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <dsp.h>

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* Ramp data */

struct foc_ramp_f32_s
{
  uint8_t ramp_mode;            /* Ramp mode */
  float   per;                  /* Controller period */
  float   ramp_thr;             /* Ramp threshold */
  float   ramp_acc;             /* Ramp acceleration */
  float   ramp_dec;             /* Ramp deceleration */
  float   ramp_dec_per;         /* dec * per */
  float   ramp_acc_per;         /* acc * per */
  float   diff;                 /* Difference now */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_ramp_init_f32
 ****************************************************************************/

int foc_ramp_init_f32(FAR struct foc_ramp_f32_s *ramp, float per,
                      float thr, float acc, float dec);

/****************************************************************************
 * Name: foc_ramp_run_f32
 ****************************************************************************/

int foc_ramp_run_f32(FAR struct foc_ramp_f32_s *ramp, float des,
                     float now, FAR float *set);

#endif /* __INDUSTRY_FOC_FLOAT_FOC_RAMP_H */
