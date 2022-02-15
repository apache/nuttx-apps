/****************************************************************************
 * apps/include/industry/foc/float/foc_align.h
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

#ifndef __INDUSTRY_FOC_FLOAT_FOC_ALIGN_H
#define __INDUSTRY_FOC_FLOAT_FOC_ALIGN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <dsp.h>

#include "industry/foc/float/foc_routine.h"
#include "industry/foc/float/foc_angle.h"

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* Align routine callbacks */

struct foc_routine_align_cb_f32_s
{
  /* Private data for callbacks */

  FAR void *priv;

  /* Align angle zero callback */

  CODE int (*zero)(FAR void *priv, float offset);

  /* Align angle direction callback */

  CODE int (*dir)(FAR void *priv, float dir);
};

/* Align routine configuration */

struct foc_routine_align_cfg_f32_s
{
  struct foc_routine_align_cb_f32_s cb;           /* Align routine callbacks */
  float                             volt;         /* Align voltage */
  int                               offset_steps; /* Offset alignment steps */
};

/* Align routine final data */

struct foc_routine_aling_final_f32_s
{
  float dir;                    /* Angle sensor direction */
  float offset;                 /* Angle sensor offset */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern struct foc_routine_ops_f32_s g_foc_routine_align_f32;

#endif /* __INDUSTRY_FOC_FLOAT_FOC_ALIGN_H */
