/****************************************************************************
 * apps/include/industry/foc/fixed16/foc_routine.h
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

#ifndef __INDUSTRY_FOC_FIXED16_FOC_ROUTINE_H
#define __INDUSTRY_FOC_FIXED16_FOC_ROUTINE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "industry/foc/fixed16/foc_handler.h"

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

struct foc_routine_in_b16_s
{
  FAR struct foc_state_b16_s *foc_state; /* FOC controller state */
  b16_t                       angle;     /* Angle electrical now */
  b16_t                       angle_m;   /* Angle mechanical now */
  b16_t                       vel;       /* Velocity now */
  b16_t                       vbus;      /* VBUS now */
};

/* FOC routine output */

struct foc_routine_out_b16_s
{
  dq_frame_b16_t dq_ref;        /* Output DQ reference */
  dq_frame_b16_t vdq_comp;      /* Output DQ voltage compensation */
  b16_t          angle;         /* Output phase angle */
  int            foc_mode;      /* Output FOC mode */
};

/* Forward declaration */

typedef struct foc_routine_b16_s foc_routine_b16_t;

/* FOC routine operations */

struct foc_routine_ops_b16_s
{
  /* Initialize */

  CODE int (*init)(FAR foc_routine_b16_t *h);

  /* Deinitialize */

  CODE void (*deinit)(FAR foc_routine_b16_t *h);

  /* Configure */

  CODE int (*cfg)(FAR foc_routine_b16_t *h, FAR void *cfg);

  /* Run routine */

  CODE int (*run)(FAR foc_routine_b16_t *h,
                  FAR struct foc_routine_in_b16_s *in,
                  FAR struct foc_routine_out_b16_s *out);

  /* Run routine */

  CODE int (*final)(FAR foc_routine_b16_t *h, FAR void *data);
};

/* FOC routine data */

struct foc_routine_b16_s
{
  FAR struct foc_routine_ops_b16_s *ops;
  FAR void                         *data;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_routine_init_b16
 ****************************************************************************/

int foc_routine_init_b16(FAR foc_routine_b16_t *r,
                         FAR struct foc_routine_ops_b16_s *ops);

/****************************************************************************
 * Name: foc_routine_deinit_b16
 ****************************************************************************/

int foc_routine_deinit_b16(FAR foc_routine_b16_t *r);

/****************************************************************************
 * Name: foc_routine_cfg_b16
 ****************************************************************************/

int foc_routine_cfg_b16(FAR foc_routine_b16_t *r, FAR void *cfg);

/****************************************************************************
 * Name: foc_routine_run_b16
 ****************************************************************************/

int foc_routine_run_b16(FAR foc_routine_b16_t *r,
                        FAR struct foc_routine_in_b16_s *in,
                        FAR struct foc_routine_out_b16_s *out);

/****************************************************************************
 * Name: foc_routine_final_b16
 ****************************************************************************/

int foc_routine_final_b16(FAR foc_routine_b16_t *r, FAR void *data);

#endif /* __INDUSTRY_FOC_FIXED16_FOC_ROUTINE_H */
