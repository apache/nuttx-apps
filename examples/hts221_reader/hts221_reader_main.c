/****************************************************************************
 * apps/examples/hts221_reader/hts221_reader_main.c
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

#include <nuttx/sensors/hts221.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * lsm303_reader_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FILE *sensor;
  hts221_conv_data_t sensor_data;
  int ret;

  sensor = fopen("/dev/hts2210", "r");
  if (sensor == NULL)
    {
      printf("Unable to create file\n");
      return -ENOENT;
    }

  ret = ioctl(fileno(sensor), SNIOC_START_CONVERSION, 0);
  if (ret < 0)
    {
      printf("IOCTL SNIOC_START failed %d\n", ret);
    }

  for (; ; )
    {
      ret = ioctl(fileno(sensor), SNIOC_READ_CONVERT_DATA,
                  (unsigned long)&sensor_data);
      if (ret < 0)
        {
          printf("IOCTL READ failed %d\n", ret);
        }

      printf("temp: %d, hum: %d\n",
             sensor_data.temperature,
             sensor_data.humidity);

      sleep(2);
    }

  fclose(sensor);
  return EXIT_SUCCESS;
}
