/****************************************************************************
 * apps/examples/bmi160/sixaxis_main.c
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
#include <inttypes.h>
#include <fcntl.h>
#include <stdio.h>

#include <nuttx/sensors/bmi160.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ACC_DEVPATH      "/dev/accel0"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * sixaxis_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  struct accel_gyro_st_s data;
  uint32_t prev;

  fd = open(ACC_DEVPATH, O_RDONLY);
  if (fd < 0)
    {
      printf("Device %s open failure. %d\n", ACC_DEVPATH, fd);
      return -1;
    }

  prev = 0;
  for (; ; )
    {
      int ret;

      ret = read(fd, &data, sizeof(struct accel_gyro_st_s));
      if (ret != sizeof(struct accel_gyro_st_s))
        {
          fprintf(stderr, "Read failed.\n");
          break;
        }

      /* If sensing time has been changed, show 6 axis data. */

      if (prev != data.sensor_time)
        {
          printf("[%" PRIu32 "] %d, %d, %d / %d, %d, %d\n",
                 data.sensor_time,
                 data.gyro.x, data.gyro.y, data.gyro.z,
                 data.accel.x, data.accel.y, data.accel.z);
          fflush(stdout);
          prev = data.sensor_time;
        }
    }

  close(fd);

  return 0;
}
