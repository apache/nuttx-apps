/****************************************************************************
 * apps/examples/dhtxx/dhtxx_main.c
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

#include <nuttx/sensors/dhtxx.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * dhtxx_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct dhtxx_sensor_data_s data;
  int fd;
  int ret;
  int i;

  printf("Dhtxx app is running.\n");

  fd = open(CONFIG_EXAMPLES_DHTXX_DEVPATH, O_RDWR);

  for (i = 0; i < 20; i++)
    {
      ret = read(fd, &data, sizeof(struct dhtxx_sensor_data_s));
      if (ret < 0)
        {
          printf("Read error.\n");
          printf("Sensor reported error %d\n", data.status);
        }
      else
        {
          printf("Read successful.\n");
          printf("Humidity = %2.2f %%, temperature = %2.2f C\n",
                  data.hum, data.temp);
        }

      sleep(1);
    }

  close(fd);
  return 0;
}
