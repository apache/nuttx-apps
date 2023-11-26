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
static const char sensor_gps_format[] =
  "timestamp:%" PRIu64 ",time_utc:%" PRIu64 ",latitude:%hf,longitude:%hf,"
  "altitude:%hf,altitude_ellipsoid:%hf,eph:%hf,epv:%hf,hdop:%hf,pdop:%hf,"
  "vdop:%hf,ground_speed:%hf,course:%hf,satellites_used:%" PRIu32 "";

static const char sensor_gps_satellite_format[] =
  "timestamp:%" PRIu64 ",count:%" PRIu32 ",satellites:%" PRIu32 ","
  "svid0:%" PRIu32 ",elevation0:%" PRIu32 ",azimuth0:%" PRIu32 ","
  "snr0:%" PRIu32 ",svid1:%" PRIu32 ",elevation1:%" PRIu32 ","
  "azimuth1:%" PRIu32 ",snr1:%" PRIu32 ",svid2:%" PRIu32 ","
  "elevation2:%" PRIu32 ",azimuth2:%" PRIu32 ",snr2:%" PRIu32 ","
  "svid3:%" PRIu32 ",elevation3:%" PRIu32 ",azimuth3:%" PRIu32 ","
  "snr3:%" PRIu32 "";
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

ORB_DEFINE(sensor_gps, struct sensor_gps, sensor_gps_format);
ORB_DEFINE(sensor_gps_satellite, struct sensor_gps_satellite,
           sensor_gps_satellite_format);
