/****************************************************************************
 * apps/examples/foc/foc_parseargs.h
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

#ifndef __EXAMPLES_FOC_FOC_PARSEARGS_H
#define __EXAMPLES_FOC_FOC_PARSEARGS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "foc_device.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* Application arguments */

struct args_s
{
  int      time;                /* Run time limit in sec, -1 if forever */
  int      mode;                /* Operation mode */
  int      qparam;              /* Open-loop Q setting (x1000) */
  uint32_t pi_kp;               /* PI Kp (x1000) */
  uint32_t pi_ki;               /* PI Ki (x1000) */
  uint32_t velmax;              /* Velocity max (x1000) */
  int      state;               /* Example state (FREE, CW, CCW, STOP) */
  int8_t   en;                  /* Enabled instances (bit-encoded) */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

void parse_args(FAR struct args_s *args, int argc, FAR char **argv);

#endif /* __EXAMPLES_FOC_FOC_THR_H */
