/****************************************************************************
 * apps/examples/foc/foc_device.h
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

#ifndef __APPS_EXAMPLES_FOC_FOC_DEVICE_H
#define __APPS_EXAMPLES_FOC_FOC_DEVICE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "industry/foc/foc_utils.h"

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* FOC device data */

struct foc_device_s
{
  int                 fd;      /* FOC device */
  struct foc_info_s   info;    /* FOC dev info */
  struct foc_state_s  state;   /* FOC dev state */
  struct foc_params_s params;  /* FOC dev params */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int foc_device_init(FAR struct foc_device_s *dev, int id);
int foc_device_deinit(FAR struct foc_device_s *dev);
int foc_device_start(FAR struct foc_device_s *dev, bool state);
int foc_dev_state_get(FAR struct foc_device_s *dev);
int foc_dev_params_set(FAR struct foc_device_s *dev);
int foc_dev_state_handle(FAR struct foc_device_s *dev, FAR bool *flag);

#endif /* __APPS_EXAMPLES_FOC_FOC_DEVICE_H */
