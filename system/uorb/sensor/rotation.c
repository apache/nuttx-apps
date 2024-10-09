/****************************************************************************
 * apps/system/uorb/sensor/rotation.c
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

#include <sensor/rotation.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_UORB
static const char sensor_rotation_format[] =
  "timestamp:%" PRIu64 ",x:%hf,y:%hf,z:%hf";
static const char sensor_orientation_format[] =
  "timestamp:%" PRIu64 ",x:%hf,y:%hf,z:%hf,w:%hf";
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

ORB_DEFINE(sensor_rotation, struct sensor_rotation, sensor_rotation_format);
ORB_DEFINE(sensor_orientation, struct sensor_orientation,
           sensor_orientation_format);
ORB_DEFINE(sensor_device_orientation, struct sensor_orientation,
           sensor_orientation_format);
