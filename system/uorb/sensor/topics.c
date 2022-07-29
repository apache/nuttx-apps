/****************************************************************************
 * apps/system/uorb/sensor/topics.c
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

#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include <sensor/accel.h>
#include <sensor/baro.h>
#include <sensor/cap.h>
#include <sensor/co2.h>
#include <sensor/dust.h>
#include <sensor/ecg.h>
#include <sensor/gps.h>
#include <sensor/gyro.h>
#include <sensor/gesture.h>
#include <sensor/hall.h>
#include <sensor/hbeat.h>
#include <sensor/hcho.h>
#include <sensor/hrate.h>
#include <sensor/humi.h>
#include <sensor/impd.h>
#include <sensor/ir.h>
#include <sensor/light.h>
#include <sensor/mag.h>
#include <sensor/noise.h>
#include <sensor/ots.h>
#include <sensor/ph.h>
#include <sensor/pm25.h>
#include <sensor/pm1p0.h>
#include <sensor/pm10.h>
#include <sensor/ppgd.h>
#include <sensor/ppgq.h>
#include <sensor/prox.h>
#include <sensor/rgb.h>
#include <sensor/temp.h>
#include <sensor/tvoc.h>
#include <sensor/uv.h>

#include <uORB/uORB.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR const struct orb_metadata *g_sensor_list[] =
{
  ORB_ID(sensor_accel),
  ORB_ID(sensor_accel_uncal),
  ORB_ID(sensor_baro),
  ORB_ID(sensor_cap),
  ORB_ID(sensor_co2),
  ORB_ID(sensor_dust),
  ORB_ID(sensor_ecg),
  ORB_ID(sensor_gps),
  ORB_ID(sensor_gps_satellite),
  ORB_ID(sensor_gyro),
  ORB_ID(sensor_gyro_uncal),
  ORB_ID(sensor_hall),
  ORB_ID(sensor_hbeat),
  ORB_ID(sensor_hcho),
  ORB_ID(sensor_humi),
  ORB_ID(sensor_hrate),
  ORB_ID(sensor_impd),
  ORB_ID(sensor_ir),
  ORB_ID(sensor_light),
  ORB_ID(sensor_light_uncal),
  ORB_ID(sensor_mag),
  ORB_ID(sensor_mag_uncal),
  ORB_ID(sensor_noise),
  ORB_ID(sensor_ots),
  ORB_ID(sensor_ph),
  ORB_ID(sensor_pm10),
  ORB_ID(sensor_pm1p0),
  ORB_ID(sensor_pm25),
  ORB_ID(sensor_ppgd),
  ORB_ID(sensor_ppgq),
  ORB_ID(sensor_prox),
  ORB_ID(sensor_rgb),
  ORB_ID(sensor_temp),
  ORB_ID(sensor_tvoc),
  ORB_ID(sensor_uv),
  ORB_ID(sensor_wake_gesture),
  ORB_ID(sensor_wake_gesture_uncal),
  NULL,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

FAR const struct orb_metadata *orb_get_meta(FAR const char *name)
{
  struct sensor_state_s state;
  char path[ORB_PATH_MAX];
  int idx = -1;
  int ret;
  int fd;
  int i;

  /* Fisrt search built-in topics */

  for (i = 0; g_sensor_list[i]; i++)
    {
      if (!strncmp(g_sensor_list[i]->o_name, name,
          strlen(g_sensor_list[i]->o_name)))
        {
          if (idx == -1 || strlen(g_sensor_list[idx]->o_name) <
                           strlen(g_sensor_list[i]->o_name))
            {
              idx = i;
            }
        }
    }

  if (idx != -1)
    {
      return g_sensor_list[idx];
    }

  /* Then open node to get meta */

  snprintf(path, ORB_PATH_MAX, ORB_SENSOR_PATH"%s", name);
  fd = open(path, O_RDONLY);
  if (fd < 0)
    {
      snprintf(path, ORB_PATH_MAX, ORB_SENSOR_PATH"%s%d", name, 0);
      fd = open(path, O_RDONLY);
      if (fd < 0)
        {
          return NULL;
        }
    }

  ret = ioctl(fd, SNIOC_GET_STATE, (unsigned long)(uintptr_t)&state);
  close(fd);
  if (ret < 0)
    {
      return NULL;
    }

  return state.priv;
}
