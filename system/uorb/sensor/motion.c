/****************************************************************************
 * apps/system/uorb/sensor/motion.c
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

#include <sensor/motion.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_UORB
static const char sensor_event_format[] =
  "timestamp:%" PRIu64 ",event:%" PRIu32 "";
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

ORB_DEFINE(sensor_motion_detect, struct sensor_event, sensor_event_format);
ORB_DEFINE(sensor_significant_motion, struct sensor_event,
           sensor_event_format);
ORB_DEFINE(sensor_step_detector, struct sensor_event, sensor_event_format);
ORB_DEFINE(sensor_tilt_detector, struct sensor_event, sensor_event_format);
ORB_DEFINE(sensor_tilt_detector_uncal, struct sensor_event,
           sensor_event_format);
