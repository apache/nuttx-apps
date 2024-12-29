/****************************************************************************
 * apps/examples/foc/foc_motor_b16.h
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

#ifndef __APPS_EXAMPLES_FOC_FOC_MOTOR_B16_H
#define __APPS_EXAMPLES_FOC_FOC_MOTOR_B16_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "foc_mq.h"
#include "foc_thr.h"

#include "industry/foc/fixed16/foc_handler.h"
#include "industry/foc/fixed16/foc_ramp.h"
#include "industry/foc/fixed16/foc_angle.h"
#include "industry/foc/fixed16/foc_velocity.h"

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
#  include "industry/foc/fixed16/foc_align.h"
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
#  include "industry/foc/fixed16/foc_ident.h"
#endif
#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
#  include "industry/foc/fixed16/foc_model.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* FOC setpoint (fixed16) */

struct foc_setpoint_b16_s
{
  b16_t set;
  b16_t now;
  b16_t des;
};

/* FOC motor data (fixed16) */

struct foc_motor_b16_s
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

  struct foc_state_b16_s        foc_state;    /* FOC controller sate */
#ifdef CONFIG_EXAMPLES_FOC_MODULATION_SVM3
  struct svm3_state_b16_s       mod_state;    /* Modulation state */
#endif
  foc_handler_b16_t             handler;      /* FOC controller */
  dq_frame_b16_t                dq_ref;       /* DQ reference */
  dq_frame_b16_t                vdq_comp;     /* DQ voltage compensation */
  int                           foc_mode;     /* FOC mode */
  int                           time;         /* Helper counter */
  b16_t                         vbus;         /* Power bus voltage */
  b16_t                         per;          /* Controller period in seconds */
#ifdef CONFIG_EXAMPLES_FOC_ANGOBS
  b16_t                         ol_thr;       /* Angle observer threshold velocity */
  b16_t                         ol_hys;       /* Angle observer hysteresis */
  b16_t                         angle_step;   /* Open-loop transition step */
#endif

  /* Data from FOC device ***************************************************/

  b16_t                         iphase_adc;   /* Iphase ADC scaling factor */
  b16_t                         pwm_duty_max; /* PWM duty max */

  /* Velocity controller data ***********************************************/

  struct foc_ramp_b16_s         ramp;         /* Velocity ramp data */
#ifdef CONFIG_EXAMPLES_FOC_VELCTRL_PI
  pid_controller_b16_t          vel_pi;       /* Velocity controller */
#endif

  /* Angle state ************************************************************/

  b16_t                         angle_now;    /* Phase angle now */
  b16_t                         angle_m;      /* Motor mechanical angle */
  b16_t                         angle_el;     /* Motor electrical angle */
#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  b16_t                         angle_ol;     /* Phase angle open-loop */
#endif
#ifdef CONFIG_EXAMPLES_FOC_ANGOBS
  b16_t                         angle_obs;    /* Angle observer output */
  b16_t                         angle_err;    /* Open-loop to observer error */
#endif

  /* Velocity state *********************************************************/

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  b16_t                         vel_el;       /* Velocity - electrical */
  b16_t                         vel_mech;     /* Velocity - mechanical */
  b16_t                         vel_filter;   /* Velocity low-pass filter */
#endif
#ifdef CONFIG_EXAMPLES_FOC_VELOBS
  b16_t                         vel_obs;      /* Velocity observer output */
#endif

  /* Motor setpoints ********************************************************/

#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
  struct foc_setpoint_b16_s     torq;         /* Torque setpoint */
  b16_t                         torq_sat;     /* Torque saturation */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  struct foc_setpoint_b16_s     vel;          /* Velocity setpoint */
  b16_t                         vel_sat;      /* Velocity saturation */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
  struct foc_setpoint_b16_s     pos;          /* Position setpoint */
#endif
  b16_t                         dir;          /* Motor's direction */

  /* Motor routines *********************************************************/

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
  struct foc_routine_b16_s      align;        /* Alignment routine */
  bool                          align_done;   /* Alignment done */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
  struct foc_routine_b16_s      ident;        /* Motor ident routine */
  struct motor_phy_params_b16_s phy_ident;    /* Motor phy from ident */
  bool                          ident_done;   /* Motor ident done */
#endif

  /* Motor data *************************************************************/

#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM
  struct foc_model_b16_s        model;        /* Model handler */
  struct foc_model_state_b16_s  model_state;  /* PMSM model state */
#endif
  struct motor_phy_params_b16_s phy;          /* Motor phy */

  /* Motor velocity and angle handlers **************************************/

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  foc_angle_b16_t               openloop;     /* Open-loop angle handler */
  uint8_t                       openloop_now; /* Open-loop now */
  b16_t                         openloop_q;   /* Open-loop Q parameter */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_HALL
  foc_angle_b16_t               hall;         /* Hall angle handler */
  char                          hldpath[32];  /* Hall devpath */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_QENCO
  foc_angle_b16_t               qenco;        /* Qenco angle handler */
  char                          qedpath[32];  /* Qenco devpath */
#endif
#ifdef CONFIG_EXAMPLES_FOC_VELOBS_DIV
  foc_velocity_b16_t            vel_div;       /* DIV velocity observer */
#endif
#ifdef CONFIG_EXAMPLES_FOC_VELOBS_PLL
  foc_velocity_b16_t            vel_pll;       /* PLL velocity observer */
#endif
#ifdef CONFIG_EXAMPLES_FOC_ANGOBS_SMO
  foc_angle_b16_t               ang_smo;      /* SMO angle observer */
#endif
#ifdef CONFIG_EXAMPLES_FOC_ANGOBS_NFO
  foc_angle_b16_t               ang_nfo;      /* NFO angle observer */
#endif
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int foc_motor_init(FAR struct foc_motor_b16_s *motor,
                   FAR struct foc_ctrl_env_s *envp);
int foc_motor_deinit(FAR struct foc_motor_b16_s *motor);
int foc_motor_get(FAR struct foc_motor_b16_s *motor);
int foc_motor_control(FAR struct foc_motor_b16_s *motor);
int foc_motor_handle(FAR struct foc_motor_b16_s *motor,
                     FAR struct foc_mq_s *handle);

#endif /* __APPS_EXAMPLES_FOC_FOC_MOTOR_B16_H */
