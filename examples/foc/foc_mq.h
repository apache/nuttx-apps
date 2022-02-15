/****************************************************************************
 * apps/examples/foc/foc_mq.h
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

#ifndef __APPS_EXAMPLES_FOC_FOC_MQ_H
#define __APPS_EXAMPLES_FOC_FOC_MQ_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <mqueue.h>

#include "foc_device.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONTROL_MQ_MAXMSG  (10)
#define CONTROL_MQ_MSGSIZE (5)
#define CONTROL_MQ_MQNAME  "mqf"

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* Thread message type */

enum foc_thr_msg_e
{
  CONTROL_MQ_MSG_INVALID  = 0,
  CONTROL_MQ_MSG_VBUS     = 1,
  CONTROL_MQ_MSG_APPSTATE = 2,
  CONTROL_MQ_MSG_SETPOINT = 3,
  CONTROL_MQ_MSG_START    = 4,
  CONTROL_MQ_MSG_KILL     = 5
};

/* FOC thread handler */

struct foc_mq_s
{
  bool     quit;
  bool     start;
  int      app_state;
  uint32_t vbus;
  uint32_t setpoint;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_mq_handle
 ****************************************************************************/

int foc_mq_handle(mqd_t mq, FAR struct foc_mq_s *h);

#endif /* __APPS_EXAMPLES_FOC_FOC_THR_H */
