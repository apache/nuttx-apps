/****************************************************************************
 * apps/include/industry/foc/fixed16/foc_model.h
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

#ifndef __INDUSTRY_FOC_FIXED16_FOC_MODEL_H
#define __INDUSTRY_FOC_FIXED16_FOC_MODEL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <dspb16.h>

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_MODEL_PMSM
/* PMSM model configuration */

struct foc_model_pmsm_cfg_b16_s
{
  uint8_t poles;
  b16_t   res;
  b16_t   ind;
  b16_t   iner;
  b16_t   flux_link;
  b16_t   ind_d;
  b16_t   ind_q;
  b16_t   per;
  b16_t   iphase_adc;
};
#endif  /* CONFIG_INDUSTRY_FOC_MODEL_PMSM */

/* FOC model state */

struct foc_model_state_b16_s
{
  int32_t        curr_raw[CONFIG_MOTOR_FOC_PHASES];
  b16_t          curr[CONFIG_MOTOR_FOC_PHASES];
  b16_t          volt[CONFIG_MOTOR_FOC_PHASES];
  ab_frame_b16_t iab;
  ab_frame_b16_t vab;
  dq_frame_b16_t vdq;
  dq_frame_b16_t idq;
  b16_t          omega_e;
  b16_t          omega_m;
};

/* Forward declaration */

typedef struct foc_model_b16_s foc_model_b16_t;

/* FOC model operations */

struct foc_model_ops_b16_s
{
  /* Initialize */

  CODE int (*init)(FAR foc_model_b16_t *h);

  /* Deinitialize */

  CODE void (*deinit)(FAR foc_model_b16_t *h);

  /* Configure model */

  CODE int (*cfg)(FAR foc_model_b16_t *h, FAR void *cfg);

  /* Run electrical model */

  CODE void (*ele_run)(FAR foc_model_b16_t *h,
                       FAR ab_frame_b16_t *v_ab);

  /* Run mechanical model */

  CODE void (*mech_run)(FAR foc_model_b16_t *h,
                        b16_t load);

  /* Get model state */

  CODE void (*state)(FAR foc_model_b16_t *h,
                     FAR struct foc_model_state_b16_s *state);
};

/* FOC model handler */

struct foc_model_b16_s
{
  FAR struct foc_model_ops_b16_s *ops;
  FAR void                       *model;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_MODEL_PMSM
/* PMSM model operations (fixed16) */

extern struct foc_model_ops_b16_s g_foc_model_pmsm_ops_b16;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_model_init_b16
 ****************************************************************************/

int foc_model_init_b16(FAR foc_model_b16_t *h,
                       FAR struct foc_model_ops_b16_s *ops);

/****************************************************************************
 * Name: foc_model_deinit_b16
 ****************************************************************************/

int foc_model_deinit_b16(FAR foc_model_b16_t *h);

/****************************************************************************
 * Name: foc_model_cfg_b16
 ****************************************************************************/

int foc_model_cfg_b16(FAR foc_model_b16_t *h, FAR void *cfg);

/****************************************************************************
 * Name: foc_model_run_b16
 ****************************************************************************/

void foc_model_run_b16(FAR foc_model_b16_t *h,
                       b16_t load,
                       FAR ab_frame_b16_t *v_ab);

/****************************************************************************
 * Name: foc_model_state_b16
 ****************************************************************************/

void foc_model_state_b16(FAR foc_model_b16_t *h,
                         FAR struct foc_model_state_b16_s *state);

#endif /* __INDUSTRY_FOC_FIXED16_FOC_MODEL_H */
