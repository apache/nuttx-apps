/****************************************************************************
 * apps/examples/fxos8700cq/fxos8700cq_main.c
 * fxos8700cq motion sensor sample application
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
#include <fcntl.h>
#include <stdio.h>

#include <nuttx/sensors/fxos8700cq.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef ACC_DEVPATH
#define ACC_DEVPATH "/dev/accel0"
#endif
/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * fxos8700cq_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;

  fxos8700cq_data data;

  fd = open(ACC_DEVPATH, O_RDONLY);
  if (fd < 0)
    {
      printf("Device %s open failure. %d\n", ACC_DEVPATH, fd);

      return -1;
    }

  while (true)
    {
      int ret;

      ret = read(fd, &data, sizeof(fxos8700cq_data));
      if (ret != sizeof(fxos8700cq_data))
        {
           fprintf(stderr, "Read failed : %d/%d@%d\n",
                   ret, sizeof(fxos8700cq_data), fd);
           break;
        }
      printf("{\"accel\":[%d, %d, %d],\"magn\":[%d, %d, %d]}\n",
             data.accel.x, data.accel.y, data.accel.z,
             data.magn.x, data.magn.y, data.magn.z
          );
      fflush(stdout);
      usleep(1000 * 1000 / 5);
    }

  close(fd);

  return 0;
}
