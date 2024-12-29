/****************************************************************************
 * apps/system/uorb/sensor/gesture.c
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

#include <sensor/gesture.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_UORB
static const char sensor_gesture_format[] =
  "timestamp:%" PRIu64 ",event:%" PRIu32 "";
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

ORB_DEFINE(sensor_glance_gesture, struct sensor_event,
           sensor_gesture_format);
ORB_DEFINE(sensor_glance_gesture_uncal, struct sensor_event,
           sensor_gesture_format);
ORB_DEFINE(sensor_offbody_detector, struct sensor_event,
           sensor_gesture_format);
ORB_DEFINE(sensor_offbody_detector_uncal, struct sensor_event,
           sensor_gesture_format);
ORB_DEFINE(sensor_pickup_gesture, struct sensor_event,
           sensor_gesture_format);
ORB_DEFINE(sensor_pickup_gesture_uncal, struct sensor_event,
           sensor_gesture_format);
ORB_DEFINE(sensor_wrist_tilt, struct sensor_event, sensor_gesture_format);
ORB_DEFINE(sensor_wrist_tilt_uncal, struct sensor_event,
           sensor_gesture_format);
ORB_DEFINE(sensor_wake_gesture, struct sensor_event, sensor_gesture_format);
ORB_DEFINE(sensor_wake_gesture_uncal, struct sensor_event,
           sensor_gesture_format);
