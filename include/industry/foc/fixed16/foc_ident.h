/****************************************************************************
 * apps/include/industry/foc/fixed16/foc_ident.h
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

#ifndef __INDUSTRY_FOC_FIXED16_FOC_IDENT_H
#define __INDUSTRY_FOC_FIXED16_FOC_IDENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <dspb16.h>

#include "industry/foc/fixed16/foc_routine.h"

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* Identification routine configuration */

struct foc_routine_ident_cfg_b16_s
{
  b16_t per;                    /* Routine period in sec */
  b16_t res_current;            /* Resistance measurement current */
  b16_t ind_volt;               /* Inductance measurement current */
  int   res_steps;              /* Resistance measurement steps */
  int   ind_steps;              /* Inductance measurement steps */
  int   idle_steps;             /* IDLE steps */
};

/* Identification routine final data */

struct foc_routine_ident_final_b16_s
{
  bool    ready;                 /* Result ready */
  b16_t   res;                   /* Phase resistance */
  b16_t   ind;                   /* Phase inductance */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern struct foc_routine_ops_b16_s g_foc_routine_ident_b16;

#endif /* __INDUSTRY_FOC_FIXED16_FOC_IDENT_H */
