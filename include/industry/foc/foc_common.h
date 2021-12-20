/****************************************************************************
 * apps/include/industry/foc/foc_common.h
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
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_INDUSTRY_FOC_FOC_COMMON_H
#define __APPS_INCLUDE_INDUSTRY_FOC_FOC_COMMON_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* FOC controller mode */

enum foc_handler_mode_e
{
  FOC_HANDLER_MODE_INIT = 0, /* Initial state */
  FOC_HANDLER_MODE_IDLE,     /* Idle */
  FOC_HANDLER_MODE_VOLTAGE,  /* Voltage mode - control DQ voltage */
  FOC_HANDLER_MODE_CURRENT   /* Current mode - control DQ current */
};

/* FOC number type identifiers */

enum foc_number_type_e
{
  FOC_NUMBER_TYPE_INVALID = 0,
#ifdef CONFIG_INDUSTRY_FOC_FLOAT
  FOC_NUMBER_TYPE_FLOAT   = 1,   /* float */
#endif
#ifdef CONFIG_INDUSTRY_FOC_FIXED16
  FOC_NUMBER_TYPE_FIXED16 = 2,   /* b16_t */
#endif
};

/* Speed ramp mode */

enum foc_ramp_mode_e
{
  RAMP_MODE_INVALID   = 0,      /* Reserved */
  RAMP_MODE_SOFTSTART = 1,      /* Soft start */
  RAMP_MODE_SOFTSTOP  = 2,      /* Soft stop */
  RAMP_MODE_NORMAL    = 3,      /* Normal operation */
};

/* Angle handler type */

enum foc_angle_type_e
{
  FOC_ANGLE_TYPE_INVALID = 0,  /* Reserved */
  FOC_ANGLE_TYPE_ELE     = 1,  /* Electrical angle */
  FOC_ANGLE_TYPE_MECH    = 2,  /* Mechanical angle */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* __APPS_INCLUDE_INDUSTRY_FOC_FOC_COMMON_H */
