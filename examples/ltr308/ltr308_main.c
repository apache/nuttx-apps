/****************************************************************************
 * apps/examples/ltr308/ltr308_main.c
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
#include <nuttx/sensors/ioctl.h>
#include <nuttx/sensors/sensor.h>
#include <nuttx/sensors/ltr308.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <unistd.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ltr308_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ltr308_calibval calibval;
  struct sensor_light light;
  struct pollfd pfd;
  int ret;
  int fd;

  fd = open("/dev/uorb/sensor_light0", O_RDONLY);
  if (fd < 0)
    {
      perror("Could not open file");
      return fd;
    }

  calibval.integration_time = 1;
  calibval.measurement_rate = 2;
  calibval.gain = 1;
  ret = ioctl(fd, SNIOC_CALIBRATE, &calibval);
  if (ret < 0)
    {
      perror("Could not calibrate sensor");
      goto err_out;
    }

  memset(&pfd, 0, sizeof(struct pollfd));
  pfd.fd = fd;
  pfd.events = POLLIN;
  ret = poll(&pfd, 1, -1);
  if (ret < 0)
    {
      perror("Could not poll sensor");
      goto err_out;
    }

  ret = read(fd, &light, sizeof(struct sensor_light));
  if (ret < 0)
    {
      perror("Could not read from sensor");
      goto err_out;
    }

  printf("timestamp: %llu, lux: %f\n", light.timestamp, light.light);
  return OK;

err_out:
  close(fd);
  return ret;
}
