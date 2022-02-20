/****************************************************************************
 * apps/include/industry/foc/float/foc_velocity.h
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

#ifndef __INDUSTRY_FOC_FLOAT_FOC_VELOCITY_H
#define __INDUSTRY_FOC_FLOAT_FOC_VELOCITY_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <dsp.h>

#include "industry/foc/float/foc_handler.h"

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* Input to velocity handler */

struct foc_velocity_in_f32_s
{
  FAR struct foc_state_f32_s *state; /* FOC state */
  float                       angle; /* Last angle */
  float                       vel;   /* Last velocity */
  float                       dir;   /* Movement direction */
};

/* Output from velocity handler */

struct foc_velocity_out_f32_s
{
  float velocity;
};

/* Forward declaration */

typedef struct foc_velocity_f32_s foc_velocity_f32_t;

/* Velocity operations */

struct foc_velocity_ops_f32_s
{
  /* Initialize */

  CODE int (*init)(FAR foc_velocity_f32_t *h);

  /* Deinitialize */

  CODE void (*deinit)(FAR foc_velocity_f32_t *h);

  /* Configure */

  CODE int (*cfg)(FAR foc_velocity_f32_t *h, FAR void *cfg);

  /* Zero */

  CODE int (*zero)(FAR foc_velocity_f32_t *h);

  /* Direction */

  CODE int (*dir)(FAR foc_velocity_f32_t *h, float dir);

  /* Run velocity handler */

  CODE int (*run)(FAR foc_velocity_f32_t *h,
                  FAR struct foc_velocity_in_f32_s *in,
                  FAR struct foc_velocity_out_f32_s *out);
};

/* Velocity handler - sensor or sensorless */

struct foc_velocity_f32_s
{
  FAR struct foc_velocity_ops_f32_s *ops;
  FAR void                          *data;
};

#ifdef CONFIG_INDUSTRY_FOC_VELOCITY_ODIV
/* Velocity DIV observer */

struct foc_vel_div_f32_cfg_s
{
  uint8_t samples;
  float   filter;
  float   per;
};
#endif

#ifdef CONFIG_INDUSTRY_FOC_VELOCITY_OPLL
/* Velocity PLL observer */

struct foc_vel_pll_f32_cfg_s
{
  float kp;
  float ki;
  float per;
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_VELOCITY_ODIV
/* Velocity DIV observer (float) */

extern struct foc_velocity_ops_f32_s g_foc_velocity_odiv_f32;
#endif

#ifdef CONFIG_INDUSTRY_FOC_VELOCITY_OPLL
/* Velocity PLL observer (float) */

extern struct foc_velocity_ops_f32_s g_foc_velocity_opll_f32;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_velocity_init_f32
 ****************************************************************************/

int foc_velocity_init_f32(FAR foc_velocity_f32_t *h,
                          FAR struct foc_velocity_ops_f32_s *ops);

/****************************************************************************
 * Name: foc_velocity_deinit_f32
 ****************************************************************************/

int foc_velocity_deinit_f32(FAR foc_velocity_f32_t *h);

/****************************************************************************
 * Name: foc_velocity_cfg_f32
 ****************************************************************************/

int foc_velocity_cfg_f32(FAR foc_velocity_f32_t *h, FAR void *cfg);

/****************************************************************************
 * Name: foc_velocity_zero_f32
 ****************************************************************************/

int foc_velocity_zero_f32(FAR foc_velocity_f32_t *h);

/****************************************************************************
 * Name: foc_velocity_dir_f32
 ****************************************************************************/

int foc_velocity_dir_f32(FAR foc_velocity_f32_t *h, float dir);

/****************************************************************************
 * Name: foc_velocity_run_f32
 ****************************************************************************/

int foc_velocity_run_f32(FAR foc_velocity_f32_t *h,
                         FAR struct foc_velocity_in_f32_s *in,
                         FAR struct foc_velocity_out_f32_s *out);

#endif /* __INDUSTRY_FOC_FLOAT_FOC_VELOCITY_H */
