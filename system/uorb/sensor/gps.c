/****************************************************************************
 * apps/system/uorb/sensor/gps.c
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

#include <sensor/gps.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_UORB
static void print_sensor_gps_message(FAR const struct orb_metadata *meta,
                                     FAR const void *buffer)
{
  FAR const struct sensor_gps *message = buffer;
  const orb_abstime now = orb_absolute_time();

  uorbinfo_raw("%s:\ttimestamp: %" PRIu64 " (%" PRIu64 " us ago) "
               "time_utc: %" PRIu64 " latitude: %.4f longitude: %.4f",
               meta->o_name, message->timestamp, now - message->timestamp,
               message->time_utc, message->latitude, message->longitude);

  uorbinfo_raw("%s:\taltitude: %.4f altitude_ellipsoid: %.4f "
               "ground_speed: %.4f course: %.4f",
               meta->o_name, message->altitude, message->altitude_ellipsoid,
               message->ground_speed, message->course);

  uorbinfo_raw("%s:\teph: %.4f epv: %.4f hdop: %.4f vdop: %.4f",
               meta->o_name, message->eph, message->epv,
               message->hdop, message->vdop);
}

static void
print_sensor_gps_satellite_message(FAR const struct orb_metadata *meta,
                                   FAR const void *buffer)
{
  FAR const struct sensor_gps_satellite *message = buffer;
  const orb_abstime now = orb_absolute_time();
  int i;

  uorbinfo_raw("%s:\ttimestamp: %" PRIu64 " (%" PRIu64 " us ago)",
               meta->o_name, message->timestamp, now - message->timestamp);

  for (i = 0; i < message->count; i++)
    {
      uorbinfo_raw("%s:\tnumber:%d svid: %" PRIu32
                   "elevation: %" PRIu32 "azimuth: %" PRIu32
                   " snr: %" PRIu32 "",
                   meta->o_name, i, message->info[i].svid,
                   message->info[i].elevation, message->info[i].azimuth,
                   message->info[i].snr);
    }
}
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

ORB_DEFINE(sensor_gps, struct sensor_gps, print_sensor_gps_message);
ORB_DEFINE(sensor_gps_satellite, struct sensor_gps_satellite,
           print_sensor_gps_satellite_message);
