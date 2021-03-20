/****************************************************************************
 * apps/include/industry/foc/float/foc_model.h
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

#ifndef __INDUSTRY_FOC_FLOAT_FOC_MODEL_H
#define __INDUSTRY_FOC_FLOAT_FOC_MODEL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <dsp.h>

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_MODEL_PMSM
/* PMSM model configuration */

struct foc_model_pmsm_cfg_f32_s
{
  uint8_t poles;
  float   res;
  float   ind;
  float   iner;
  float   flux_link;
  float   ind_d;
  float   ind_q;
  float   per;
  float   iphase_adc;
};
#endif  /* CONFIG_INDUSTRY_FOC_MODEL_PMSM */

/* FOC model state */

struct foc_model_state_f32_s
{
  int32_t        curr_raw[CONFIG_MOTOR_FOC_PHASES];
  float          curr[CONFIG_MOTOR_FOC_PHASES];
  float          volt[CONFIG_MOTOR_FOC_PHASES];
  ab_frame_f32_t iab;
  ab_frame_f32_t vab;
  dq_frame_f32_t vdq;
  dq_frame_f32_t idq;
  float          omega_e;
  float          omega_m;
};

/* Forward declaration */

typedef struct foc_model_f32_s foc_model_f32_t;

/* FOC model operations */

struct foc_model_ops_f32_s
{
  /* Initialize */

  CODE int (*init)(FAR foc_model_f32_t *h);

  /* Deinitialize */

  CODE void (*deinit)(FAR foc_model_f32_t *h);

  /* Configure model */

  CODE int (*cfg)(FAR foc_model_f32_t *h, FAR void *cfg);

  /* Run electrical model */

  CODE void (*ele_run)(FAR foc_model_f32_t *h,
                       FAR ab_frame_f32_t *v_ab);

  /* Run mechanical model */

  CODE void (*mech_run)(FAR foc_model_f32_t *h,
                       float load);

  /* Get model state */

  CODE void (*state)(FAR foc_model_f32_t *h,
                     FAR struct foc_model_state_f32_s *state);
};

/* FOC model handler */

struct foc_model_f32_s
{
  FAR struct foc_model_ops_f32_s *ops;
  FAR void                       *model;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_MODEL_PMSM
/* PMSM model operations (float) */

extern struct foc_model_ops_f32_s g_foc_model_pmsm_ops_f32;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_model_init_f32
 ****************************************************************************/

int foc_model_init_f32(FAR foc_model_f32_t *h,
                       FAR struct foc_model_ops_f32_s *ops);

/****************************************************************************
 * Name: foc_model_deinit_f32
 ****************************************************************************/

int foc_model_deinit_f32(FAR foc_model_f32_t *h);

/****************************************************************************
 * Name: foc_model_cfg_f32
 ****************************************************************************/

int foc_model_cfg_f32(FAR foc_model_f32_t *h, FAR void *cfg);

/****************************************************************************
 * Name: foc_model_run_f32
 ****************************************************************************/

void foc_model_run_f32(FAR foc_model_f32_t *h,
                       float load,
                       FAR ab_frame_f32_t *v_ab);

/****************************************************************************
 * Name: foc_model_state_f32
 ****************************************************************************/

void foc_model_state_f32(FAR foc_model_f32_t *h,
                         FAR struct foc_model_state_f32_s *state);

#endif /* __INDUSTRY_FOC_FLOAT_FOC_MODEL_H */
