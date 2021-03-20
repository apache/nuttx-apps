/****************************************************************************
 * apps/include/industry/foc/fixed16/foc_ramp.h
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

#ifndef __INDUSTRY_FOC_FIXED16_FOC_RAMP_H
#define __INDUSTRY_FOC_FIXED16_FOC_RAMP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <dspb16.h>

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* Ramp data */

struct foc_ramp_b16_s
{
  uint8_t ramp_mode;            /* Ramp mode */
  b16_t   per;                  /* Controller period */
  b16_t   ramp_thr;             /* Ramp threshold */
  b16_t   ramp_acc;             /* Ramp acceleration */
  b16_t   ramp_dec;             /* Ramp deceleration */
  b16_t   ramp_dec_per;         /* dec * per */
  b16_t   ramp_acc_per;         /* acc * per */
  b16_t   diff;                 /* Difference now */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_ramp_init_b16
 ****************************************************************************/

int foc_ramp_init_b16(FAR struct foc_ramp_b16_s *ramp, b16_t per,
                      b16_t thr, b16_t acc, b16_t dec);

/****************************************************************************
 * Name: foc_ramp_run_b16
 ****************************************************************************/

int foc_ramp_run_b16(FAR struct foc_ramp_b16_s *ramp, b16_t des,
                     b16_t now, FAR b16_t *set);

#endif /* __INDUSTRY_FOC_FIXED16_FOC_RAMP_H */
