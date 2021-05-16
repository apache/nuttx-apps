/****************************************************************************
 * apps/examples/hdc1008_demo/hdc1008_main.c
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
#include <nuttx/sensors/hdc1008.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <stdio.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEV_PATH  "/dev/hdc1008"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * hdc1008_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct hdc1008_conv_data_s data;
  int fd;
  int ret;
  int i;
  uint8_t res[3];
  char buf[16];

  /* Available resolutions: 8/11/14 bits */

  res[0] = 8;
  res[1] = 11;
  res[2] = 14;

  fd = open(DEV_PATH, O_RDONLY);
  if (fd < 0)
    {
      printf("Failed to open device driver at '%s'\n", DEV_PATH);
    }

  /* Read both t and rh */

  ret = ioctl(fd, SNIOC_SET_OPERATIONAL_MODE, HDC1008_MEAS_T_AND_RH);
  if (ret < 0)
    {
      printf("Failed to set temperature and humidity measurement mode: %d\n",
             errno);
      goto out;
    }

  ret = read(fd, buf, sizeof(buf));
  if (ret < 0)
    {
      printf("FAILED to read temperature and rh: %d\n", errno);
      goto out;
    }

  printf("Temperature and humidity\n"
         "========================\n");
  printf("data=%s\n\n", buf);

  /* Measure using ioctl */

  ret = ioctl(fd, SNIOC_MEASURE, (unsigned long)&data);
  if (ret < 0)
    {
      printf("Failed to measure using ioctl: %d\n", errno);
      goto out;
    }

  printf("Temperature and humidity (ioctl)\n"
         "================================\n");
  printf("t=%d.%d, h=%d.%d\n\n",
          data.temperature / 100, data.temperature % 100,
          data.humidity / 10, data.humidity % 10);

  /* Enable heater */

  ret = ioctl(fd, SNIOC_SET_HEATER_MODE, 1);
  if (ret < 0)
    {
      printf("Failed to enable heater: %d\n", errno);
      goto out;
    }

  /* Read only t */

  ret = ioctl(fd, SNIOC_SET_OPERATIONAL_MODE, HDC1008_MEAS_TEMPERATURE);
  if (ret < 0)
    {
      printf("Failed to set temperature measurement only: %d\n", errno);
      goto out;
    }

  for (i = 1; i < 3; ++i)
    {
      ret = ioctl(fd, SNIOC_SET_RESOLUTION_T, res[i]);
      if (ret < 0)
        {
          printf("Failed to set temperature resolution: %d\n", errno);
          goto out;
        }

      ret = read(fd, buf, sizeof(buf));
      printf("Temperature, %d bit resolution\n"
             "==============================\n", res[i]);
      printf("data=%s\n\n", buf);
    }

  /* Disable heater */

  ret = ioctl(fd, SNIOC_SET_HEATER_MODE, 0);
  if (ret < 0)
    {
      printf("Failed to disable heater: %d\n", errno);
      goto out;
    }

  /* Read only rh */

  ret = ioctl(fd, SNIOC_SET_OPERATIONAL_MODE, HDC1008_MEAS_HUMIDITY);
  if (ret < 0)
    {
      printf("Failed to set humidity measurement only: %d\n", errno);
      goto out;
    }

  for (i = 0; i < 3; ++i)
    {
      ret = ioctl(fd, SNIOC_SET_RESOLUTION_RH, res[i]);
      if (ret < 0)
        {
          printf("Failed to set humidity resolution: %d\n", errno);
          goto out;
        }

      ret = read(fd, buf, sizeof(buf));
      printf("Humidity, %d bit resolution\n"
             "===========\n", res[i]);
      printf("data=%s\n\n", buf);
    }

  ret = 0;

out:
  close(fd);
  return ret;
}
