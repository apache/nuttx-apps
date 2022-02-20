/****************************************************************************
 * apps/examples/foc/foc_motor_f32.h
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

#ifndef __APPS_EXAMPLES_FOC_FOC_MOTOR_F32_H
#define __APPS_EXAMPLES_FOC_FOC_MOTOR_F32_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "foc_mq.h"
#include "foc_thr.h"

#include "industry/foc/float/foc_handler.h"
#include "industry/foc/float/foc_ramp.h"
#include "industry/foc/float/foc_angle.h"
#include "industry/foc/float/foc_velocity.h"

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
#  include "industry/foc/float/foc_align.h"
#endif
#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
#  include "industry/foc/float/foc_model.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* FOC setpoint (float32) */

struct foc_setpoint_f32_s
{
  float set;
  float now;
  float des;
};

/* FOC motor data (float32) */

struct foc_motor_f32_s
{
  FAR struct foc_ctrl_env_s    *envp;         /* Thread env */
  bool                          fault;        /* Fault flag */
  bool                          startstop;    /* Start/stop request */
#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  bool                          openloop_now; /* Open-loop now */
  float                         angle_ol;     /* Phase angle open-loop */
  foc_angle_f32_t               openloop;     /* Open-loop angle handler */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_HALL
  char                          hldpath[32];  /* Hall devpath */
  foc_angle_f32_t               hall;         /* Hall angle handler */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_QENCO
  char                          qedpath[32];  /* Qenco devpath */
  foc_angle_f32_t               qenco;        /* Qenco angle handler */
#endif
  int                           foc_mode;     /* FOC mode */
#ifdef CONFIG_EXAMPLES_FOC_HAVE_RUN
  int                           foc_mode_run; /* FOC mode for run state */
#endif
  int                           ctrl_state;   /* Controller state */
  float                         vbus;         /* Power bus voltage */
  float                         angle_now;    /* Phase angle now */
  float                         angle_m;      /* Motor mechanical angle */
  float                         angle_el;     /* Motor electrical angle */
#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
  struct foc_setpoint_f32_s     torq;         /* Torque setpoint */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  struct foc_setpoint_f32_s     vel;          /* Velocity setpoint */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
  struct foc_setpoint_f32_s     pos;          /* Position setpoint */
#endif
  float                         dir;          /* Motor's direction */
  float                         per;          /* Controller period in seconds */
  float                         iphase_adc;   /* Iphase ADC scaling factor */
  float                         pwm_duty_max; /* PWM duty max */
  uint8_t                       poles;        /* Motor poles */
  dq_frame_f32_t                dq_ref;       /* DQ reference */
  dq_frame_f32_t                vdq_comp;     /* DQ voltage compensation */
  foc_handler_f32_t             handler;      /* FOC controller */
  struct foc_mq_s               mq;           /* MQ data */
  struct foc_state_f32_s        foc_state;    /* FOC controller sate */
  struct foc_ramp_f32_s         ramp;         /* Velocity ramp data */
#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
  struct foc_routine_f32_s      align;        /* Alignment routine */
#endif
#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  struct foc_model_f32_s        model;         /* Model handler */
  struct foc_model_state_f32_s  model_state;   /* PMSM model state */
#endif
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int foc_motor_init(FAR struct foc_motor_f32_s *motor,
                   FAR struct foc_ctrl_env_s *envp);
int foc_motor_deinit(FAR struct foc_motor_f32_s *motor);
int foc_motor_get(FAR struct foc_motor_f32_s *motor);
int foc_motor_control(FAR struct foc_motor_f32_s *motor);
int foc_motor_handle(FAR struct foc_motor_f32_s *motor,
                     FAR struct foc_mq_s *handle);

#endif /* __APPS_EXAMPLES_FOC_FOC_MOTOR_F32_H */
