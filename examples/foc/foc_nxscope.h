/****************************************************************************
 * apps/examples/foc/foc_nxscope.h
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

#ifndef __APPS_EXAMPLES_FOC_FOC_NXSCOPE_H
#define __APPS_EXAMPLES_FOC_FOC_NXSCOPE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "logging/nxscope/nxscope.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Definitions for CONFIG_EXAMPLES_FOC_NXSCOPE_CFG bit-mask */

#define FOC_NXSCOPE_IABC       (1 << 0)   /* Phases current */
#define FOC_NXSCOPE_IDQ        (1 << 1)   /* Current dq */
#define FOC_NXSCOPE_IAB        (1 << 2)   /* Current alpha-beta */
#define FOC_NXSCOPE_VABC       (1 << 3)   /* Phases voltage */
#define FOC_NXSCOPE_VDQ        (1 << 4)   /* Voltage dq */
#define FOC_NXSCOPE_VAB        (1 << 5)   /* Voltage alpha-beta */
#define FOC_NXSCOPE_AEL        (1 << 6)   /* Electrical angle */
#define FOC_NXSCOPE_AM         (1 << 7)   /* Mechanical angle */
#define FOC_NXSCOPE_VEL        (1 << 8)   /* Electrical velocity */
#define FOC_NXSCOPE_VM         (1 << 9)   /* Mechanical velocity */
#define FOC_NXSCOPE_VBUS       (1 << 10)  /* VBUS */
#define FOC_NXSCOPE_SPTORQ     (1 << 11)  /* Torque setpoint */
#define FOC_NXSCOPE_SPVEL      (1 << 12)  /* Velolcity setpoint */
#define FOC_NXSCOPE_SPPOS      (1 << 13)  /* Position setpoint */
#define FOC_NXSCOPE_DQREF      (1 << 14)  /* DQ reference */
#define FOC_NXSCOPE_VDQCOMP    (1 << 15)  /* VDQ compensation */
#define FOC_NXSCOPE_SVM3       (1 << 16)  /* Space-vector modulation sector */
#define FOC_NXSCOPE_VOBS       (1 << 17)  /* Output from velocity observer */
#define FOC_NXSCOPE_AOBS       (1 << 18)  /* Output from angle observer */
                                          /* Max 32-bit */

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

struct foc_nxscope_s
{
  struct nxscope_s           nxs;
  struct nxscope_intf_s      intf;
  struct nxscope_proto_s     proto;
  struct nxscope_ser_cfg_s   ser_cfg;
  struct nxscope_callbacks_s cb;
  uint8_t                    ch_per_inst;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_nxscope_init
 ****************************************************************************/

int foc_nxscope_init(FAR struct foc_nxscope_s *nxs);

/****************************************************************************
 * Name: foc_nxscope_deinit
 ****************************************************************************/

void foc_nxscope_deinit(FAR struct foc_nxscope_s *nxs);

/****************************************************************************
 * Name: foc_nxscope_work
 ****************************************************************************/

void foc_nxscope_work(FAR struct foc_nxscope_s *nxs);

#endif /* __APPS_EXAMPLES_FOC_FOC_NXSCOPE_H */
