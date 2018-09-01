/****************************************************************************
 * examples/dhtxx/dhtxx_main.c
 *
 *   Copyright (C) 2018 Abdelatif GUETTOUCHE. All rights reserved.
 *   Author: Abdelatif GUETTOUCHE <abdelatif.guettouche@gmail.com>
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

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int dhtxx_main(int argc, char *argv[])
#endif
{
  struct dhtxx_sensor_data_s data;
  int fd;
  int ret;
  int i;

  printf("Dhtxx app is running.\n");

  fd = open("/dev/dht0", O_RDWR);

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
