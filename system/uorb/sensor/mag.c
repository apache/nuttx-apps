/****************************************************************************
 * apps/system/uorb/sensor/mag.c
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

#include <sensor/mag.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_UORB
static void print_sensor_mag_message(FAR const struct orb_metadata *meta,
                                     FAR const void *buffer)
{
  FAR const struct sensor_mag *message = buffer;
  const orb_abstime now = orb_absolute_time();

  uorbinfo_raw("%s:\ttimestamp: %" PRIu64 " (%" PRIu64 " us ago) "
               "temperature: %.2f x: %.2f y: %.2f z: %.2f",
               meta->o_name, message->timestamp, now - message->timestamp,
               message->temperature, message->x, message->y, message->z);
}
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

ORB_DEFINE(sensor_mag, struct sensor_mag, print_sensor_mag_message);
ORB_DEFINE(sensor_mag_uncal, struct sensor_mag,
           print_sensor_mag_message);
