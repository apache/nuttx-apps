/****************************************************************************
 * examples/max31855/max31855_main.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Tiago Almeida <tiagojbalmeida@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
  while(1)
    {
      printf("Channel SSP0/SPI1 Device 0: ");
      if (fd0 < 0)
        {
          /* The file could not be open, probably the device is not registered */

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

              printf("Temperature = %d!\n",temp/4);
            }
        }

      printf("Channel SSP0/SPI1 Device 1: ");
      if (fd1 < 0)
        {
          /* The file could not be open, probably the device is not registered */

          printf("Not enabled!\n");
        }
      else
        {
          ret = read(fd1, &temp, 2);
          if (ret < 0)
            {
              /* The file could not be read, probably some max31855 pin is not
               * connected to the channel.
               */

              printf("Disconnected!\n");
            }
          else
            {
              /* Print temperature value of target device */

              printf("Temperature = %d!\n",temp/4);
            }
        }

      printf("Channel SSP1/SPI2 Device 0: ");
      if (fd2 < 0)
        {
          /* The file could not be open, probably the device is not registered */

          printf("Not enabled!\n");
        }
      else
        {
          ret = read(fd2, &temp, 2);
          if (ret < 0)
            {
              /* The file could not be read, probably some max31855 pin is not
               * connected to the channel.
               */

              printf("Disconnected!\n");
            }
          else
            {
              /* Print temperature value of target device */

              printf("Temperature = %d!\n",temp/4);
            }
        }

      printf("Channel SSP1/SPI2 Device 1: ");
      if (fd3 < 0)
        {
          /* The file could not be open, probably the device is not registered */

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

              printf("Temperature = %d!\n",temp/4);
            }
        }

      printf("\n");

      /* One second sample rate */

      usleep(1000000);
    }

  return 0;
}
