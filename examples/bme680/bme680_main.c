/****************************************************************************
 * apps/examples/bme680/bme680_main.c
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
#include <nuttx/sensors/sensor.h>
#include <nuttx/sensors/bme680.h>
#include <stdio.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>

#define NB_LOWERHALFS 3

/* Structure used when polling the sub-sensors */

struct data
{
  void *data_struct;
  uint16_t data_size;
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * bme680_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int baro_fd;
  int hum_fd;
  int gas_fd;
  uint16_t seconds;
  int ret;

  /* This example works when all of the sub-sensors of
   * the BME680 are enabled.
   */

  struct sensor_baro baro_data;
  struct sensor_humi humi_data;
  struct sensor_gas gas_data;

  /* Open each lowerhalf file to be able to read the data.
   * When the pressure measurement is deactivated, sensor_temp0 should
   * be opened instead (to get the temperature measurement).
   */

  baro_fd = open("/dev/uorb/sensor_baro0", O_RDONLY | O_NONBLOCK);
  if (baro_fd < 0)
    {
      printf("Failed to open barometer lowerhalf.\n");
      return -1;
    }

  hum_fd = open("/dev/uorb/sensor_humi0", O_RDONLY | O_NONBLOCK);
  if (hum_fd < 0)
    {
      printf("Failed to open humidity sensor lowerhalf.\n");
      return -1;
    }

  gas_fd = open("/dev/uorb/sensor_gas0", O_RDONLY | O_NONBLOCK);
  if (gas_fd < 0)
    {
      printf("Failed to open gas lowerhalf.\n");
      return -1;
    }

  /* Configure the sensor */

  struct bme680_config_s config;

  /* Set oversampling */

  config.temp_os = BME680_OS_2X;
  config.press_os = BME680_OS_16X;
  config.filter_coef = BME680_FILTER_COEF3;
  config.hum_os = BME680_OS_1X;

  /* Set heater parameters */

  config.target_temp = 300;     /* degrees Celsius */
  config.amb_temp = 30;         /* degrees Celsius */
  config.heater_duration = 100; /* milliseconds */

  config.nb_conv = 0;

  ret = ioctl(baro_fd, SNIOC_CALIBRATE, &config);

  struct pollfd pfds[] = {
    {.fd = baro_fd, .events = POLLIN},
    {.fd = hum_fd, .events = POLLIN},
    {.fd = gas_fd, .events = POLLIN}
  };

  struct data sensor_data[] = {
    {.data_struct = &baro_data, .data_size = sizeof(struct sensor_baro)},
    {.data_struct = &humi_data, .data_size = sizeof(struct sensor_humi)},
    {.data_struct = &gas_data, .data_size = sizeof(struct sensor_gas)}
  };

  seconds = 5 * 60;

  /* Wait ~5 minutes for the sensor to accomodate to the surroundings.
   * The first measurements are not accurate. The longer the wait, the more
   * accurate the results are.
   */

  while (seconds > 0)
    {
      ret = poll(pfds, NB_LOWERHALFS, -1);
      if (ret < 0)
        {
          perror("Could not poll sensor.");
          return ret;
        }

      /* Go through lowerhalfs and read the data */

      for (int i = 0; i < NB_LOWERHALFS; i++)
        {
          if (pfds[i].revents & POLLIN)
            {
              ret = read(pfds[i].fd, sensor_data[i].data_struct,
                                     sensor_data[i].data_size);

              if (ret != sensor_data[i].data_size)
                {
                  perror("Could not read from sub-sensor.");
                  return ret;
                }
            }
        }

      seconds -= 3;
    }

  printf("Temperature [C] = %f\n"
         "Pressure [hPa] = %f\n"
         "Humidity [rH] = %f\n"
         "Gas resistance [kOhm] = %f\n",
         baro_data.temperature, baro_data.pressure,
         humi_data.humidity, gas_data.gas_resistance);

  close(baro_fd);
  close(hum_fd);
  close(gas_fd);

  return 0;
}
