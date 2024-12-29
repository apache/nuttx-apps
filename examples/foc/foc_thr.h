/****************************************************************************
 * apps/examples/foc/foc_thr.h
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

#ifndef __APPS_EXAMPLES_FOC_FOC_THR_H
#define __APPS_EXAMPLES_FOC_FOC_THR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <pthread.h>
#include <mqueue.h>

#include <nuttx/motor/foc/foc.h>

#include "foc_device.h"

#ifdef CONFIG_EXAMPLES_FOC_NXSCOPE
#  include "foc_nxscope.h"
#endif

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* FOC example state */

enum foc_example_state_e
{
  FOC_EXAMPLE_STATE_INVALID = 0, /* Reserved */
  FOC_EXAMPLE_STATE_FREE    = 1, /* No current */
  FOC_EXAMPLE_STATE_STOP    = 2, /* Active break */
  FOC_EXAMPLE_STATE_CW      = 3, /* CW direction */
  FOC_EXAMPLE_STATE_CCW     = 4, /* CCW direction */
};

/* FOC control mode */

enum foc_foc_mode_e
{
  FOC_FMODE_INVALID  = 0, /* Reserved */
  FOC_FMODE_IDLE     = 1, /* IDLE */
  FOC_FMODE_VOLTAGE  = 2, /* Voltage mode */
  FOC_FMODE_CURRENT  = 3, /* Current mode */
};

/* Motor control mode */

enum foc_motor_mode_e
{
  FOC_MMODE_INVALID    = 0,  /* Reserved */
#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
  FOC_MMODE_TORQ       = 1,  /* Torque control */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  FOC_MMODE_VEL        = 2,  /* Velocity control */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
  FOC_MMODE_POS        = 3,  /* Position control */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
  FOC_MMODE_ALIGN_ONLY = 4,  /* Sensor alignment only */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
  FOC_MMODE_IDENT_ONLY = 5,  /* Motor identification only */
#endif
  FOC_MMODE_IDLE       = 6,  /* IDLE state */
};

/* Controller state */

enum foc_controller_state_e
{
  FOC_CTRL_STATE_INVALID = 0,
  FOC_CTRL_STATE_INIT,
#ifdef CONFIG_EXAMPLES_FOC_HAVE_ALIGN
  FOC_CTRL_STATE_ALIGN,
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
  FOC_CTRL_STATE_IDENT,
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_RUN
  FOC_CTRL_STATE_RUN_INIT,
  FOC_CTRL_STATE_RUN,
#endif
  FOC_CTRL_STATE_IDLE,
  FOC_CTRL_STATE_TERMINATE
};

/* FOC thread data */

struct foc_ctrl_env_s
{
  mqd_t                     mqd;   /* Control msg queue */
  uint8_t                   id;    /* FOC device id */
  int                       inst;  /* Type specific instance counter */
  int                       type;  /* Controller type */
  FAR struct foc_thr_cfg_s *cfg;   /* Control thread configuration */
#ifdef CONFIG_EXAMPLES_FOC_NXSCOPE
  FAR struct foc_nxscope_s *nxs;   /* nxscope handler */
#endif
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int foc_threads_init(void);
void foc_threads_deinit(void);
bool foc_threads_terminated(void);
uint32_t foc_threads_get(void);
int foc_thread_type(int id);
int foc_ctrlthr_init(FAR struct foc_ctrl_env_s *foc, int i, FAR mqd_t *mqd,
                     FAR pthread_t *thread);

#endif /* __APPS_EXAMPLES_FOC_FOC_THR_H */
