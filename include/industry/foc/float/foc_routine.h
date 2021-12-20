/****************************************************************************
 * apps/include/industry/foc/float/foc_routine.h
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

#ifndef __INDUSTRY_FOC_FLOAT_FOC_ROUTINE_H
#define __INDUSTRY_FOC_FLOAT_FOC_ROUTINE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "industry/foc/float/foc_handler.h"

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* Routine run return */

enum foc_routine_run_e
{
  FOC_ROUTINE_RUN_NOTDONE = 0,
  FOC_ROUTINE_RUN_DONE    = 1
};

/* FOC routine input */

struct foc_routine_in_f32_s
{
  FAR struct foc_state_f32_s *foc_state; /* FOC controller state */
  float                       angle;     /* Angle electrical now */
  float                       angle_m;   /* Angle mechanical now */
  float                       vel;       /* Velocity now */
  float                       vbus;      /* VBUS now */
};

/* FOC routine output */

struct foc_routine_out_f32_s
{
  dq_frame_f32_t dq_ref;        /* Output DQ reference */
  dq_frame_f32_t vdq_comp;      /* Output DQ voltage compensation */
  float          angle;         /* Output phase angle */
  int            foc_mode;      /* Output FOC mode */
};

/* Forward declaration */

typedef struct foc_routine_f32_s foc_routine_f32_t;

/* FOC routine operations */

struct foc_routine_ops_f32_s
{
  /* Initialize */

  CODE int (*init)(FAR foc_routine_f32_t *h);

  /* Deinitialize */

  CODE void (*deinit)(FAR foc_routine_f32_t *h);

  /* Configure */

  CODE int (*cfg)(FAR foc_routine_f32_t *h, FAR void *cfg);

  /* Run routine */

  CODE int (*run)(FAR foc_routine_f32_t *h,
                  FAR struct foc_routine_in_f32_s *in,
                  FAR struct foc_routine_out_f32_s *out);

  /* Run routine */

  CODE int (*final)(FAR foc_routine_f32_t *h, FAR void *data);
};

/* FOC routine data */

struct foc_routine_f32_s
{
  FAR struct foc_routine_ops_f32_s *ops;
  FAR void                         *data;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_routine_init_f32
 ****************************************************************************/

int foc_routine_init_f32(FAR foc_routine_f32_t *r,
                         FAR struct foc_routine_ops_f32_s *ops);

/****************************************************************************
 * Name: foc_routine_deinit_f32
 ****************************************************************************/

int foc_routine_deinit_f32(FAR foc_routine_f32_t *r);

/****************************************************************************
 * Name: foc_routine_cfg_f32
 ****************************************************************************/

int foc_routine_cfg_f32(FAR foc_routine_f32_t *r, FAR void *cfg);

/****************************************************************************
 * Name: foc_routine_run_f32
 ****************************************************************************/

int foc_routine_run_f32(FAR foc_routine_f32_t *r,
                        FAR struct foc_routine_in_f32_s *in,
                        FAR struct foc_routine_out_f32_s *out);

/****************************************************************************
 * Name: foc_routine_final_f32
 ****************************************************************************/

int foc_routine_final_f32(FAR foc_routine_f32_t *r, FAR void *data);

#endif /* __INDUSTRY_FOC_FLOAT_FOC_ROUTINE_H */
