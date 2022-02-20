/****************************************************************************
 * apps/include/industry/foc/fixed16/foc_handler.h
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

#ifndef __INDUSTRY_FOC_FIXED16_FOC_HANDLER_H
#define __INDUSTRY_FOC_FIXED16_FOC_HANDLER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <dspb16.h>

#ifdef CONFIG_INDUSTRY_FOC_CORDIC
#  include "industry/foc/fixed16/foc_cordic.h"
#endif

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* Input to FOC controller */

struct foc_handler_input_b16_s
{
  FAR b16_t          *current;  /* Phase current samples */
  FAR dq_frame_b16_t *dq_ref;   /* DQ reference frame */
  FAR dq_frame_b16_t *vdq_comp; /* DQ voltage compensation */
  b16_t               angle;    /* Phase angle */
  b16_t               vbus;     /* Bus voltage */
  int                 mode;     /* Controller mode (enum foc_handler_mode_e) */
};

/* Output from FOC controller */

struct foc_handler_output_b16_s
{
  b16_t duty[CONFIG_MOTOR_FOC_PHASES];  /* New duty cycle for PWM */
};

/* Controller state */

struct foc_state_b16_s
{
  b16_t           curr[CONFIG_MOTOR_FOC_PHASES];
  b16_t           volt[CONFIG_MOTOR_FOC_PHASES];
  ab_frame_b16_t  iab;
  ab_frame_b16_t  vab;
  dq_frame_b16_t  vdq;
  dq_frame_b16_t  idq;
  b16_t           mod_scale;
};

/* Forward declaration */

typedef struct foc_handler_b16_s foc_handler_b16_t;

/* Modulation operations */

struct foc_modulation_ops_b16_s
{
  /* Initialize */

  CODE int (*init)(FAR foc_handler_b16_t *h);

  /* Deinitialzie */

  CODE void (*deinit)(FAR foc_handler_b16_t *h);

  /* Configure modulation */

  CODE void (*cfg)(FAR foc_handler_b16_t *h, FAR void *cfg);

  /* Modulation specific current correction */

  CODE void (*current)(FAR foc_handler_b16_t *h, FAR b16_t *curr);

  /* Get the base voltage for a given modulation scheme */

  CODE void (*vbase_get)(FAR foc_handler_b16_t *h,
                         b16_t vbus,
                         FAR b16_t *vbase);

  /* Run modulation */

  CODE void (*run)(FAR foc_handler_b16_t *h,
                   FAR ab_frame_b16_t *v_ab_mod,
                   FAR b16_t *duty);
};

/* Current/voltage controller operations */

struct foc_control_ops_b16_s
{
  /* Initialize */

  CODE int (*init)(FAR foc_handler_b16_t *h);

  /* Deinitialize */

  CODE void (*deinit)(FAR foc_handler_b16_t *h);

  /* Configure controller */

  CODE void (*cfg)(FAR foc_handler_b16_t *h, FAR void *cfg);

  /* Feed controller with input data */

  CODE void (*input_set)(FAR foc_handler_b16_t *h,
                         FAR b16_t *current,
                         b16_t vbase,
                         b16_t angle);

  /* Run voltage controller */

  CODE void (*voltage_run)(FAR foc_handler_b16_t *h,
                           FAR dq_frame_b16_t *dq_ref,
                           FAR ab_frame_b16_t *v_ab_mod);

  /* Run current controller */

  CODE void (*current_run)(FAR foc_handler_b16_t *h,
                           FAR dq_frame_b16_t *dq_ref,
                           FAR dq_frame_b16_t *vdq_comp,
                           FAR ab_frame_b16_t *v_ab_mod);

  /* Get controller state data */

  CODE void (*state_get)(FAR foc_handler_b16_t *h,
                         FAR struct foc_state_b16_s *state);
};

/* FOC handler operations */

struct foc_handler_ops_b16_s
{
  /* Current/voltage controller interface */

  struct foc_control_ops_b16_s *ctrl;

  /* Modulation interface */

  struct foc_modulation_ops_b16_s *mod;
};

/* FOC handler data */

struct foc_handler_b16_s
{
#ifdef CONFIG_INDUSTRY_FOC_CORDIC
  int                           fd;            /* CORDIC device */
#endif
  struct foc_handler_ops_b16_s  ops;           /* Handler operations */
  FAR void                     *modulation;    /* Modulation data */
  FAR void                     *control;       /* Controller data */
};

/* Modulation configuration */

struct foc_mod_cfg_b16_s
{
  b16_t pwm_duty_max;  /* Maximum allowed PWM duty cycle */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_CONTROL_PI
/* FOC PI controller (fixed16) */

extern struct foc_control_ops_b16_s g_foc_control_pi_b16;
#endif

#ifdef CONFIG_INDUSTRY_FOC_MODULATION_SVM3
/* 3-phase space vector modulation Modulation handler (fixed16) */

extern struct foc_modulation_ops_b16_s g_foc_mod_svm3_b16;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_handler_init_b16
 ****************************************************************************/

int foc_handler_init_b16(FAR foc_handler_b16_t *h,
                         FAR struct foc_control_ops_b16_s *ctrl,
                         FAR struct foc_modulation_ops_b16_s *mod);

/****************************************************************************
 * Name: foc_handler_deinit_b16
 ****************************************************************************/

int foc_handler_deinit_b16(FAR foc_handler_b16_t *h);

/****************************************************************************
 * Name: foc_handler_run_b16
 ****************************************************************************/

int foc_handler_run_b16(FAR foc_handler_b16_t *h,
                        FAR struct foc_handler_input_b16_s *in,
                        FAR struct foc_handler_output_b16_s *out);

/****************************************************************************
 * Name: foc_handler_cfg_b16
 ****************************************************************************/

void foc_handler_cfg_b16(FAR foc_handler_b16_t *h,
                         FAR void *ctrl_cfg,
                         FAR void *mod_cfg);

/****************************************************************************
 * Name: foc_handler_state_b16
 ****************************************************************************/

void foc_handler_state_b16(FAR foc_handler_b16_t *h,
                           FAR struct foc_state_b16_s *state);

#ifdef CONFIG_INDUSTRY_FOC_HANDLER_PRINT
/****************************************************************************
 * Name: foc_handler_state_print_b16
 ****************************************************************************/

void foc_handler_state_print_b16(FAR struct foc_state_b16_s *state);
#endif

#endif /* __INDUSTRY_FOC_FIXED16_FOC_HANDLER_H */
