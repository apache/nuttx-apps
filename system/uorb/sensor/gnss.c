/****************************************************************************
 * apps/system/uorb/sensor/gnss.c
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

#include <sensor/gnss.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_UORB
#define UORB_DEBUG_FORMAT_SENSOR_GNSS     \
  "timestamp:%" PRIu64                    \
  ",time_utc:%" PRIu64                    \
  ",latitude:%hf"                         \
  ",longitude:%hf"                        \
  ",altitude:%hf"                         \
  ",altitude_ellipsoid:%hf"               \
  ",eph:%hf"                              \
  ",epv:%hf"                              \
  ",hdop:%hf"                             \
  ",pdop:%hf"                             \
  ",vdop:%hf"                             \
  ",ground_speed:%hf"                     \
  ",course:%hf"                           \
  ",satellites_used:%" PRIu32 ""

#define SENSOR_GNSS_SATELLITE_INFO_FORMAT(idx) \
  ",svid" #idx ":%" PRIu32 \
  ",elevation" #idx ":%" PRIu32 \
  ",azimuth" #idx ":%" PRIu32 \
  ",snr" #idx ":%" PRIu32 ""

static const char sensor_gnss_format[] =
  UORB_DEBUG_FORMAT_SENSOR_GNSS ",firmware_version:%" PRIu32 "";

static const char sensor_gnss_clock_format[] =
  "timestamp:%" PRIu64 ","
  "flags:%" PRIx32 ",leap_second:%" PRId32 ",time_ns:%" PRId64 ","
  "time_uncertainty_ns:%hf,hw_clock_discontinuity_count:%" PRIu32 ","
  "full_bias_ns:%" PRId64 ",bias_ns:%hf,bias_uncertainty_ns:%hf,"
  "drift_nsps:%hf,drift_uncertainty_nsps:%hf";

static const char sensor_gnss_geofence_event_format[] =
  "type:%" PRId32 ",geofence_id:%" PRId32 ","
  UORB_DEBUG_FORMAT_SENSOR_GNSS ","
  "timestamp:%" PRId64 ",status:%" PRId32 ",transition:%" PRId32 "";

static const char sensor_gnss_measurement_format[] =
  "timestamp:%" PRIu64 ","
  "flags:%" PRIx32 ",svid:%" PRId32 ",constellation:%" PRIu32 ","
  "time_offset_ns:%hf,received_sv_time_in_ns:%" PRId64 ","
  "received_sv_time_uncertainty_in_ns:%" PRId64 ",state:%" PRIu32 ","
  "c_n0_dbhz:%hf,pseudorange_rate_mps:%hf,"
  "pseudorange_rate_uncertainty_mps:%hf,"
  "accumulated_delta_range_state:%" PRIu32 ",accumulated_delta_range_m:%hf,"
  "accumulated_delta_range_uncertainty_m:%hf,"
  "carrier_frequency_hz:%hf,carrier_cycles:%" PRId64 ",carrier_phase:%hf,"
  "carrier_phase_uncertainty:%hf,multipath_indicator:%" PRIu32 ","
  "snr:%" PRIu32 "";

static const char sensor_gnss_satellite_format[] =
  "timestamp:%" PRIu64 ",count:%" PRIu32 ",satellites:%" PRIu32 ","
  "constellation:%" PRIu32 ""
  ",cf:%hf"
  SENSOR_GNSS_SATELLITE_INFO_FORMAT(0)
  SENSOR_GNSS_SATELLITE_INFO_FORMAT(1)
  SENSOR_GNSS_SATELLITE_INFO_FORMAT(2)
  SENSOR_GNSS_SATELLITE_INFO_FORMAT(3)
  ;
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

ORB_DEFINE(sensor_gnss, struct sensor_gnss, sensor_gnss_format);
ORB_DEFINE(sensor_gnss_clock, struct sensor_gnss_clock,
           sensor_gnss_clock_format);
ORB_DEFINE(sensor_gnss_geofence_event, struct sensor_gnss_geofence_event,
           sensor_gnss_geofence_event_format);
ORB_DEFINE(sensor_gnss_measurement, struct sensor_gnss_measurement,
           sensor_gnss_measurement_format);
ORB_DEFINE(sensor_gnss_satellite, struct sensor_gnss_satellite,
           sensor_gnss_satellite_format);
