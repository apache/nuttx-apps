/****************************************************************************
 * apps/examples/foc/foc_intf.h
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

#ifndef __APPS_EXAMPLES_FOC_FOC_INTF_H
#define __APPS_EXAMPLES_FOC_FOC_INTF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* FOC control interface data */

struct foc_intf_data_s
{
  uint32_t state;
  uint32_t vbus_raw;
  int32_t  sp_raw;
  bool     vbus_update;
  bool     state_update;
  bool     sp_update;
  bool     terminate;
  bool     started;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int foc_intf_init(void);
int foc_intf_deinit(void);
int foc_intf_update(FAR struct foc_intf_data_s *data);

#endif /* __APPS_EXAMPLES_FOC_FOC_INTF_H */
