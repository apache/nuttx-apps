/****************************************************************************
 * apps/examples/foc/foc_thr.h
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

#ifndef __EXAMPLES_FOC_FOC_THR_H
#define __EXAMPLES_FOC_FOC_THR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/motor/foc/foc.h>

#include <mqueue.h>

#include "foc_device.h"

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* FOC example state */

enum foc_example_state_e
{
  FOC_EXAMPLE_STATE_INVALID = 0, /* Reserved */
  FOC_EXAMPLE_STATE_FREE    = 1, /* No current */
  FOC_EXAMPLE_STATE_STOP    = 2, /* Active break */
  FOC_EXAMPLE_STATE_CW      = 3, /* CW direction */
  FOC_EXAMPLE_STATE_CCW     = 4, /* CCW direction */
};

/* Operation modes */

enum foc_operation_mode_e
{
  FOC_OPMODE_INVALID  = 0, /* Reserved */
  FOC_OPMODE_IDLE     = 1, /* IDLE */
  FOC_OPMODE_OL_V_VEL = 2, /* Voltage open-loop velocity controller */
  FOC_OPMODE_OL_C_VEL = 3, /* Current open-loop velocity controller */

  /* Not supported yet */

#if 0
  FOC_OPMODE_CL_C_TRQ = 3, /* Current closed-loop torque controller */
  FOC_OPMODE_CL_C_VEL = 4, /* Current closed-loop velocity controller */
  FOC_OPMODE_CL_C_POS = 5  /* Current closed-loop position controller */
#endif
};

/* FOC thread data */

struct foc_ctrl_env_s
{
  struct foc_device_s dev;      /* FOC device */
  mqd_t               mqd;      /* Control msg queue */
  int                 id;       /* FOC device id */
  int                 inst;     /* Type specific instance counter */
  int                 type;     /* Controller type */
  int                 qparam;   /* Open-loop Q setting (x1000) */
  int                 mode;     /* Operation mode */
  uint32_t            pi_kp;    /* FOC PI Kp (x1000) */
  uint32_t            pi_ki;    /* FOC PI Ki (x1000) */
  uint32_t            velmax;   /* Velocity max (x1000) */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_FLOAT
int foc_float_thr(FAR struct foc_ctrl_env_s *envp);
#endif

#ifdef CONFIG_INDUSTRY_FOC_FIXED16
int foc_fixed16_thr(FAR struct foc_ctrl_env_s *envp);
#endif

#endif /* __EXAMPLES_FOC_FOC_THR_H */
