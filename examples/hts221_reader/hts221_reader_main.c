/****************************************************************************
 * examples/hts221_reader/hts221_reader_main.c
 *
 *   Copyright (C) 2020 Gregory Nutt. All rights reserved.
 *   Author: Mateusz Szafoni <raiden00@railab.me>
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

  ret = ioctl(fileno(sensor), SNIOC_START, 0);
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
