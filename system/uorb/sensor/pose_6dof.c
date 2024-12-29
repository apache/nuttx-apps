/****************************************************************************
 * apps/system/uorb/sensor/pose_6dof.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sensor/pose_6dof.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_UORB
static const char sensor_pose_6dof_format[] =
  "timestamp:%" PRIu64 ",x:%hf,y:%hf,z:%hf,w:%hf,tx:%hf,ty:%hf,tz:%hf,"
  "dx:%hf,dy:%hf,dz:%hf,dw:%hf,dtx:%hf,dty:%hf,dtz:%hf,number:%" PRIu64 "";
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

ORB_DEFINE(sensor_pose_6dof, struct sensor_pose_6dof,
           sensor_pose_6dof_format);
