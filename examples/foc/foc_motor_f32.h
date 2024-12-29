/****************************************************************************
 * apps/examples/foc/foc_motor_f32.h
 *
 * SPDX-License-Identifier: Apache-2.0
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
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
#  include "industry/foc/float/foc_ident.h"
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
  /* App data ***************************************************************/

  FAR struct foc_ctrl_env_s    *envp;         /* Thread env */
  struct foc_mq_s               mq;           /* MQ data */
  bool                          fault;        /* Fault flag */
  bool                          startstop;    /* Start/stop request */
  int                           ctrl_state;   /* Controller state */
#ifdef CONFIG_EXAMPLES_FOC_HAVE_RUN
  int                           foc_mode_run; /* FOC mode for run state */
#endif

  /* FOC data ***************************************************************/

  struct foc_state_f32_s        foc_state;    /* FOC controller sate */
#ifdef CONFIG_EXAMPLES_FOC_MODULATION_SVM3
  struct svm3_state_f32_s       mod_state;    /* Modulation state */
#endif
  foc_handler_f32_t             handler;      /* FOC controller */
  dq_frame_f32_t                dq_ref;       /* DQ reference */
  dq_frame_f32_t                vdq_comp;     /* DQ voltage compensation */
  int                           foc_mode;     /* FOC mode */
  int                           time;         /* Helper counter */
  float                         vbus;         /* Power bus voltage */
  float                         per;          /* Controller period in seconds */
#ifdef CONFIG_EXAMPLES_FOC_ANGOBS
  float                         ol_thr;       /* Angle observer threshold velocity */
  float                         ol_hys;       /* Angle observer hysteresis */
#endif

  /* Data from FOC device ***************************************************/

  float                         iphase_adc;   /* Iphase ADC scaling factor */
  float                         pwm_duty_max; /* PWM duty max */

  /* Velocity controller data ***********************************************/

  struct foc_ramp_f32_s         ramp;         /* Velocity ramp data */
#ifdef CONFIG_EXAMPLES_FOC_VELCTRL_PI
  pid_controller_f32_t          vel_pi;       /* Velocity controller */
#endif

  /* Angle state ************************************************************/

  float                         angle_now;    /* Phase angle now */
  float                         angle_m;      /* Motor mechanical angle */
  float                         angle_el;     /* Motor electrical angle */
#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  float                         angle_ol;     /* Phase angle open-loop */
#endif
#ifdef CONFIG_EXAMPLES_FOC_ANGOBS
  float                         angle_obs;    /* Angle observer output */
  float                         angle_err;    /* Open-loop to observer error */
  float                         angle_step;   /* Open-loop transition step */
#endif

  /* Velocity state *********************************************************/

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  float                         vel_el;       /* Velocity - electrical */
  float                         vel_mech;     /* Velocity - mechanical */
  float                         vel_filter;   /* Velocity low-pass filter */
#endif
#ifdef CONFIG_EXAMPLES_FOC_VELOBS
  float                         vel_obs;      /* Velocity observer output */
#endif

  /* Motor setpoints ********************************************************/

#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
  struct foc_setpoint_f32_s     torq;         /* Torque setpoint */
  float                         torq_sat;     /* Torque saturation */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  struct foc_setpoint_f32_s     vel;          /* Velocity setpoint */
  float                         vel_sat;      /* Velocity saturation */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
  struct foc_setpoint_f32_s     pos;          /* Position setpoint */
#endif
  float                         dir;          /* Motor's direction */

  /* Motor routines *********************************************************/

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
  struct foc_routine_f32_s      align;        /* Alignment routine */
  bool                          align_done;   /* Motor alignment done */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
  struct foc_routine_f32_s      ident;        /* Motor ident routine */
  struct motor_phy_params_f32_s phy_ident;    /* Motor phy from ident */
  bool                          ident_done;   /* Motor ident done */
#endif

  /* Motor data *************************************************************/

#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  struct foc_model_f32_s        model;        /* Model handler */
  struct foc_model_state_f32_s  model_state;  /* PMSM model state */
#endif
  struct motor_phy_params_f32_s phy;          /* Motor phy */

  /* Motor velocity and angle handlers **************************************/

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  foc_angle_f32_t               openloop;     /* Open-loop angle handler */
  uint8_t                       openloop_now; /* Open-loop now */
  float                         openloop_q;   /* Open-loop Q parameter */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_HALL
  foc_angle_f32_t               hall;         /* Hall angle handler */
  char                          hldpath[32];  /* Hall devpath */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_QENCO
  foc_angle_f32_t               qenco;        /* Qenco angle handler */
  char                          qedpath[32];  /* Qenco devpath */
#endif
#ifdef CONFIG_EXAMPLES_FOC_VELOBS_DIV
  foc_velocity_f32_t            vel_div;       /* DIV velocity observer */
#endif
#ifdef CONFIG_EXAMPLES_FOC_VELOBS_PLL
  foc_velocity_f32_t            vel_pll;       /* PLL velocity observer */
#endif
#ifdef CONFIG_EXAMPLES_FOC_ANGOBS_SMO
  foc_angle_f32_t               ang_smo;      /* SMO angle observer */
#endif
#ifdef CONFIG_EXAMPLES_FOC_ANGOBS_NFO
  foc_angle_f32_t               ang_nfo;      /* NFO angle observer */
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
