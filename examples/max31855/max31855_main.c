/****************************************************************************
 * apps/examples/max31855/max31855_main.c
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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* In this app, only two devices are registered to each SSP channel */

  int16_t temp;
  int fd0;
  int fd1;
  int fd2;
  int fd3;
  int ret;

  /* Open files for each registered device to the respective SSP channel */

  /* Channel SSP0/SPI1 Device 0 */

  fd0 = open("/dev/temp0", O_RDONLY);
  if (fd0 < 0)
    {
      printf("Unable to open file /dev/temp0\n");
    }

  /* Channel SSP0/SPI1 Device 1 */

  fd1 = open("/dev/temp1", O_RDONLY);
  if (fd1 < 0)
    {
      printf("Unable to open file /dev/temp1\n");
    }

  /* Channel SSP1/SPI2 Device 0 */

  fd2 = open("/dev/temp2", O_RDONLY);
  if (fd2 < 0)
    {
      printf("Unable to open file /dev/temp2\n");
    }

  /* Channel SSP1/SPI2 Device 1 */

  fd3 = open("/dev/temp3", O_RDONLY);
  if (fd3 < 0)
    {
      printf("Unable to open file /dev/temp3\n");
    }

  /* Start reading each file and print sensor temperature if acquired */

  printf("Starting...\n");
  while (1)
    {
      printf("Channel SSP0/SPI1 Device 0: ");
      if (fd0 < 0)
        {
          /* The file could not be open,
           * probably the device is not registered
           */

          printf("Not enabled!\n");
        }
      else
        {
          ret = read(fd0, &temp, 2);
          if (ret < 0)
            {
              /* The file could not be read, probably some max31855 pin is
               * not connected to the channel.
               */

              printf("Disconnected!\n");
            }
          else
            {
              /* Print temperature value of target device */

              printf("Temperature = %d!\n", temp / 4);
            }
        }

      printf("Channel SSP0/SPI1 Device 1: ");
      if (fd1 < 0)
        {
          /* The file could not be open,
           * probably the device is not registered
           */

          printf("Not enabled!\n");
        }
      else
        {
          ret = read(fd1, &temp, 2);
          if (ret < 0)
            {
              /* The file could not be read, probably some max31855 pin is
               * not connected to the channel.
               */

              printf("Disconnected!\n");
            }
          else
            {
              /* Print temperature value of target device */

              printf("Temperature = %d!\n", temp / 4);
            }
        }

      printf("Channel SSP1/SPI2 Device 0: ");
      if (fd2 < 0)
        {
          /* The file could not be open,
           * probably the device is not registered
           */

          printf("Not enabled!\n");
        }
      else
        {
          ret = read(fd2, &temp, 2);
          if (ret < 0)
            {
              /* The file could not be read, probably some max31855 pin is
               * not connected to the channel.
               */

              printf("Disconnected!\n");
            }
          else
            {
              /* Print temperature value of target device */

              printf("Temperature = %d!\n", temp / 4);
            }
        }

      printf("Channel SSP1/SPI2 Device 1: ");
      if (fd3 < 0)
        {
          /* The file could not be open,
           * probably the device is not registered
           */

          printf("Not enabled!\n");
        }
      else
        {
          ret = read(fd3, &temp, 2);
          if (ret < 0)
            {
              /* The file could not be read,
               * probably some max31855 pin is not connected to the channel.
               */

              printf("Disconnected!\n");
            }
          else
            {
              /* Print temperature value of target device */

              printf("Temperature = %d!\n", temp / 4);
            }
        }

      printf("\n");

      /* One second sample rate */

      usleep(1000000);
    }

  return 0;
}
