/****************************************************************************
 * apps/include/industry/foc/fixed16/foc_cordic.h
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

#ifndef __INDUSTRY_FOC_FIXED16_FOC_CORDIC_H
#define __INDUSTRY_FOC_FIXED16_FOC_CORDIC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <dspb16.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_CORDIC_DQSAT
/****************************************************************************
 * Name: foc_cordic_dqsat_b16
 ****************************************************************************/

int foc_cordic_dqsat_b16(int fd, FAR dq_frame_b16_t *dq_ref, b16_t mag_max);
#endif

#ifdef CONFIG_INDUSTRY_FOC_CORDIC_ANGLE
/****************************************************************************
 * Name: foc_cordic_angle_b16
 ****************************************************************************/

int foc_cordic_angle_b16(int fd, FAR phase_angle_b16_t *angle, b16_t a);
#endif

#endif /* __INDUSTRY_FOC_FIXED16_FOC_CORDIC_H */
