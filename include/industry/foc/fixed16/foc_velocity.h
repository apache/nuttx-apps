/****************************************************************************
 * apps/include/industry/foc/fixed16/foc_velocity.h
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

#ifndef __INDUSTRY_FOC_FIXED16_FOC_VELOCITY_H
#define __INDUSTRY_FOC_FIXED16_FOC_VELOCITY_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <dspb16.h>

#include "industry/foc/fixed16/foc_handler.h"

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* Input to velocity handler */

struct foc_velocity_in_b16_s
{
  FAR struct foc_state_b16_s *state; /* FOC state */
  b16_t                       angle; /* Last angle */
  b16_t                       vel;   /* Last velocity */
  b16_t                       dir;   /* Movement direction */
};

/* Output from velocity handler */

struct foc_velocity_out_b16_s
{
  b16_t velocity;
};

/* Forward declaration */

typedef struct foc_velocity_b16_s foc_velocity_b16_t;

/* Velocity operations */

struct foc_velocity_ops_b16_s
{
  /* Initialize */

  CODE int (*init)(FAR foc_velocity_b16_t *h);

  /* Deinitialize */

  CODE void (*deinit)(FAR foc_velocity_b16_t *h);

  /* Configure */

  CODE int (*cfg)(FAR foc_velocity_b16_t *h, FAR void *cfg);

  /* Zero */

  CODE int (*zero)(FAR foc_velocity_b16_t *h);

  /* Direction */

  CODE int (*dir)(FAR foc_velocity_b16_t *h, b16_t dir);

  /* Run */

  CODE int (*run)(FAR foc_velocity_b16_t *h,
                  FAR struct foc_velocity_in_b16_s *in,
                  FAR struct foc_velocity_out_b16_s *out);
};

/* Velocity handler - sensor or sensorless */

struct foc_velocity_b16_s
{
  FAR struct foc_velocity_ops_b16_s *ops;
  FAR void                          *data;
};

#ifdef CONFIG_INDUSTRY_FOC_VELOCITY_ODIV
/* Velocity DIV observer */

struct foc_vel_div_b16_cfg_s
{
  uint8_t samples;
  b16_t   filter;
  b16_t   per;
};
#endif

#ifdef CONFIG_INDUSTRY_FOC_VELOCITY_OPLL
/* Velocity PLL observer */

struct foc_vel_pll_b16_cfg_s
{
  b16_t kp;
  b16_t ki;
  b16_t per;
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_VELOCITY_ODIV
/* Velocity DIV observer (fixed16) */

extern struct foc_velocity_ops_b16_s g_foc_velocity_odiv_b16;
#endif

#ifdef CONFIG_INDUSTRY_FOC_VELOCITY_OPLL
/* Velocity PLL observer (fixed16) */

extern struct foc_velocity_ops_b16_s g_foc_velocity_opll_b16;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_velocity_init_b16
 ****************************************************************************/

int foc_velocity_init_b16(FAR foc_velocity_b16_t *h,
                          FAR struct foc_velocity_ops_b16_s *ops);

/****************************************************************************
 * Name: foc_velocity_deinit_b16
 ****************************************************************************/

int foc_velocity_deinit_b16(FAR foc_velocity_b16_t *h);

/****************************************************************************
 * Name: foc_velocity_cfg_b16
 ****************************************************************************/

int foc_velocity_cfg_b16(FAR foc_velocity_b16_t *h, FAR void *cfg);

/****************************************************************************
 * Name: foc_velocity_zero_b16
 ****************************************************************************/

int foc_velocity_zero_b16(FAR foc_velocity_b16_t *h);

/****************************************************************************
 * Name: foc_velocity_dir_b16
 ****************************************************************************/

int foc_velocity_dir_b16(FAR foc_velocity_b16_t *h, b16_t dir);

/****************************************************************************
 * Name: foc_velocity_run_b16
 ****************************************************************************/

int foc_velocity_run_b16(FAR foc_velocity_b16_t *h,
                         FAR struct foc_velocity_in_b16_s *in,
                         FAR struct foc_velocity_out_b16_s *out);

#endif /* __INDUSTRY_FOC_FIXED16_FOC_VELOCITY_H */
