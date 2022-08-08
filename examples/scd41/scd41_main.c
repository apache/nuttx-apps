/****************************************************************************
 * apps/examples/scd41/scd41_main.c
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
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <nuttx/sensors/scd41.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * scd41_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct scd41_conv_data_s data;
  int fd;
  int ret;
  int i;

  printf("scd41 app is running.\n");

  fd = open(CONFIG_EXAMPLES_SCD41_DEVPATH, O_RDWR);
  if (fd < 0)
    {
      printf("ERROR: open failed: %d\n", fd);
      return -1;
    }

  for (i = 0; i < 20; i++)
    {
      /* Sensor data is updated every 5 seconds. */

      sleep(5);

      ret = ioctl(fd, SNIOC_READ_CONVERT_DATA, (unsigned long)&data);
      if (ret < 0)
        {
          printf("Read error.\n");
          printf("Sensor reported error %d\n", ret);
        }
      else
        {
          printf("CO2[ppm]: %.2f, Temperature[C]: %.2f, RH[%%]: %.2f\n",
                 data.co2, data.temperature, data.humidity);
        }
    }

  ret = ioctl(fd, SNIOC_STOP, 0);
  if (ret < 0)
    {
      printf("Failed to stop: %d\n", errno);
    }

  close(fd);

  return 0;
}
