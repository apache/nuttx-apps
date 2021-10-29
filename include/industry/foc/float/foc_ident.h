/****************************************************************************
 * apps/include/industry/foc/float/foc_ident.h
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

#ifndef __INDUSTRY_FOC_FLOAT_FOC_IDENT_H
#define __INDUSTRY_FOC_FLOAT_FOC_IDENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <dsp.h>

#include "industry/foc/float/foc_routine.h"

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* Identification routine configuration */

struct foc_routine_ident_cfg_f32_s
{
  float per;                    /* Routine period in sec */
  float res_current;            /* Resistance measurement current */
  float ind_volt;               /* Inductance measurement current */
  int   res_steps;              /* Resistance measurement steps */
  int   ind_steps;              /* Inductance measurement steps */
  int   idle_steps;             /* IDLE steps */
};

/* Identification routine final data */

struct foc_routine_ident_final_f32_s
{
  bool    ready;                 /* Result ready */
  float   res;                   /* Phase resistance */
  float   ind;                   /* Phase inductance */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern struct foc_routine_ops_f32_s g_foc_routine_ident_f32;

#endif /* __INDUSTRY_FOC_FLOAT_FOC_IDENT_H */
