/****************************************************************************
 * apps/examples/lsm303_reader/lsm303_reader_main.c
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

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <debug.h>

#include <nuttx/sensors/lsm303agr.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * lsm303_reader_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FILE *sensor;
  struct lsm303agr_sensor_data_s sensor_data;
  int ret;

  sensor = fopen("/dev/lsm303agr0", "r");
  if (sensor == NULL)
    {
      printf("Unable to create file\n");
      return -ENOENT;
    }

  ret = ioctl(fileno(sensor), SNIOC_START, 0);
  if (ret < 0)
    {
      printf("IOCTL SNIOC_START failed %d\n", ret);
    }

  for (; ; )
    {
      ret = ioctl(fileno(sensor), SNIOC_LSM303AGRSENSORREAD,
                  (unsigned long)&sensor_data);
      if (ret < 0)
        {
          printf("IOCTL READ failed %d\n", ret);
        }

      printf("time:%d x:%d y:%d z:%d m_x:%d m_y:%d m_z:%d temp:%d\n",
             sensor_data.timestamp,
             sensor_data.x_data,
             sensor_data.y_data,
             sensor_data.z_data,
             sensor_data.m_x_data,
             sensor_data.m_y_data,
             sensor_data.m_z_data,
             sensor_data.temperature);

      sleep(2);
    }

  fclose(sensor);
  return EXIT_SUCCESS;
}
