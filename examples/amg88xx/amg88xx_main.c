/****************************************************************************
 * apps/examples/amg88xx/amg88xx_main.c
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
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <float.h>

#include <nuttx/sensors/amg88xx.h>

#define READ_DELAY 2
#define ROWS 8
#define COLS 8

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * amg88xx_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  amg88xx_pixels_temp_t data;
  float temps[AMG88XX_PIXELS];
  int fd;
  int ret;
  int i;
  int j;

  fd = open("/dev/irm0", O_RDWR);

  ioctl(fd, SNIOC_SET_MOVING_AVG, true);

  while (1)
    {
      sleep(READ_DELAY);
      ret = read(fd, data, AMG88XX_PIXELS_ARRAY_LENGTH);
      printf("\n");
      if (ret == 0)
        {
          for (i = 0; i < AMG88XX_PIXELS_ARRAY_LENGTH; i += 2)
            {
              uint16_t pixel = (data[i + 1] << 8) | data[i];
              pixel &= 0x0fff;
              temps[i / 2] = pixel * AMG88XX_PIXEL_RESOLUTION;
            }

          for (i = 0; i < ROWS; i++)
            {
              for (j = 0; j < COLS; j++)
                {
                  printf(" %04.2f ", temps[i * COLS + j]);
                }

              printf("\n");
            }
        }
    }

  close(fd);
  return 0;
}
