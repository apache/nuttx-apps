/****************************************************************************
 * apps/examples/sht3x/sht3x_main.c
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <nuttx/sensors/ioctl.h>
#include <nuttx/sensors/sht3x.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * sht3x_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct sht3x_meas_data_s data;
  int fd;
  int ret;

  fd = open("/dev/temp0", O_RDWR);
  if (fd < 0)
    {
      printf("Failed to open /dev/temp0\n");
      return -1;
    }

  ret = ioctl(fd, SNIOC_START, NULL);
  if (ret < 0)
    {
      printf("Error to run ioctl SNIOC_START\n");
      return -1;
    }

  while (1)
    {
      printf("Converting...\n");
      usleep(1000000);

      ret = ioctl(fd, SNIOC_READ_CONVERT_DATA,
                  (unsigned long)((uintptr_t)&data));
      printf("Temperature = %f\n", data.temperature);
      printf("Humidity    = %f\n", data.humidity);
    }

  return 0;
}
