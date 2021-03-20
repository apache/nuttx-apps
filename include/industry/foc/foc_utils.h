/****************************************************************************
 * apps/include/industry/foc/foc_utils.h
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
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_INDUSTRY_FOC_FOC_UTILS_H
#define __APPS_INCLUDE_INDUSTRY_FOC_FOC_UTILS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/motor/foc/foc.h>
#include <nuttx/motor/motor_ioctl.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_dev_stop
 ****************************************************************************/

int foc_dev_stop(int fd);

/****************************************************************************
 * Name: foc_dev_start
 ****************************************************************************/

int foc_dev_start(int fd);

/****************************************************************************
 * Name: foc_dev_clearfault
 ****************************************************************************/

int foc_dev_clearfault(int fd);

/****************************************************************************
 * Name: foc_dev_getstate
 ****************************************************************************/

int foc_dev_getstate(int fd, FAR struct foc_state_s *state);

/****************************************************************************
 * Name: foc_dev_setparams
 ****************************************************************************/

int foc_dev_setparams(int fd, FAR struct foc_params_s *params);

/****************************************************************************
 * Name: foc_dev_setcfg
 ****************************************************************************/

int foc_dev_setcfg(int fd, FAR struct foc_cfg_s *cfg);

/****************************************************************************
 * Name: foc_dev_getcfg
 ****************************************************************************/

int foc_dev_getcfg(int fd, FAR struct foc_cfg_s *cfg);

/****************************************************************************
 * Name: foc_dev_getinfo
 ****************************************************************************/

int foc_dev_getinfo(int fd, FAR struct foc_info_s *info);

/****************************************************************************
 * Name: foc_cfg_print
 ****************************************************************************/

void foc_cfg_print(FAR struct foc_cfg_s *cfg);

#endif /* __APPS_INCLUDE_INDUSTRY_FOC_FOC_UTILS_H */
