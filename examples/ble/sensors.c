/****************************************************************************
 * apps/examples/ble/sensors.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements. See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/sensors/sensor.h>
#include <nuttx/sensors/bme680.h>
#include "sensors.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NB_LOWERHALFS 3

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct pollfd pfds[NB_LOWERHALFS];
static int baro_fd;
static int hum_fd;
static int gas_fd;

static struct sensor_baro baro_data;
static struct sensor_humi humi_data;
static struct sensor_gas gas_data;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Returns the current temperature value */

float get_temperature(void)
{
  return baro_data.temperature;
}

/* Returns the current pressure value */

float get_pressure(void)
{
  return baro_data.pressure;
}

/* Returns the current gas resistance value */

float get_gas_resistance(void)
{
  return gas_data.gas_resistance;
}

/* Returns the current humidity value */

float get_humidity(void)
{
  return humi_data.humidity;
}

/* Reads the barometer sensor data */

static void read_barometer(void)
{
  int ret = read(baro_fd, &baro_data, sizeof(struct sensor_baro));
  if (ret != sizeof(struct sensor_baro))
    {
      perror("Could not read temperature");
    }
}

/* Reads the humidity sensor data */

static void read_humidity(void)
{
  int ret = read(hum_fd, &humi_data, sizeof(struct sensor_humi));
  if (ret != sizeof(struct sensor_humi))
    {
      perror("Could not read humidity");
    }
}

/* Reads the gas sensor data */

static void read_gas(void)
{
  int ret = read(gas_fd, &gas_data, sizeof(struct sensor_gas));
  if (ret != sizeof(struct sensor_gas))
    {
      perror("Could not read gas resistance");
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Monitors sensor data and outputs periodically */

void *monitor_sensors(void *arg)
{
  while (1)
    {
      int ret = poll(pfds, NB_LOWERHALFS, -1);
      if (ret < 0)
        {
          perror("Could not poll sensor.");
          return NULL;
        }

      read_barometer();
      read_humidity();
      read_gas();

      printf("Temperature [C] = %f\n"
             "Pressure [hPa] = %f\n"
             "Humidity [rH] = %f\n"
             "Gas resistance [kOhm] = %f\n",
             baro_data.temperature, baro_data.pressure,
             humi_data.humidity, gas_data.gas_resistance);

      sleep(1);
    }

  return NULL;
}

/* Initializes sensors and configures them */

int init_sensors(void)
{
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

  pfds[0].fd = baro_fd;
  pfds[1].fd = hum_fd;
  pfds[2].fd = gas_fd;
  pfds[0].events = pfds[1].events = pfds[2].events = POLLIN;

  struct bme680_config_s config;
  config.temp_os = BME680_OS_2X;
  config.press_os = BME680_OS_16X;
  config.filter_coef = BME680_FILTER_COEF3;
  config.hum_os = BME680_OS_1X;
  config.target_temp = 300;
  config.amb_temp = 30;
  config.heater_duration = 100;
  config.nb_conv = 0;

  int ret = ioctl(baro_fd, SNIOC_CALIBRATE, &config);
  if (ret < 0)
    {
      perror("Failed to configure sensor");
      return -1;
    }

  return 0;
}

/* Closes sensor file descriptors */

void close_sensors_fd(void)
{
  close(baro_fd);
  close(hum_fd);
  close(gas_fd);
}
