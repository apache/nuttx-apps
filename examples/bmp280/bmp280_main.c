/****************************************************************************
 * apps/examples/bmp280/bmp280_main.c
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

#include <nuttx/config.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <nuttx/sensors/sensor.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * bmp280_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int ret;
  struct sensor_baro sensor_data;

  fd = open("/dev/uorb/sensor_baro0", O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      printf("Failed to open BMP280 sensor");
      return EXIT_FAILURE;
    }

  ret = read(fd, &sensor_data, sizeof(sensor_data));
  if (ret != sizeof(sensor_data))
    {
      perror("Could not read");
      return EXIT_FAILURE;
    }

  printf("Absolute pressure [hPa] = %f\n", sensor_data.pressure);
  printf("Temperature [C] = %f\n", sensor_data.temperature);

  return EXIT_SUCCESS;
}
