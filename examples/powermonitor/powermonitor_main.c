/****************************************************************************
 * apps/examples/powermonitor/powermonitor_main.c
 *
 *   Copyright (C) 2017 Giorgi Groß. All rights reserved.
 *   Author: Giorgio Groß <giorgio.gross@robodev.eu>
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

/******************************************************************************
 * Included Files
 ******************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <debug.h>
#include <stdio.h>
#include <fixedmath.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#include <nuttx/sensors/ltc4151.h>

/******************************************************************************
 * Pre-processor Definitions
 ******************************************************************************/

#define DECIMAL_PLACES3(x) abs(((int)(((x)-((int)x))*1000)))

/******************************************************************************
 * Public Functions
 ******************************************************************************/

int main(int argc, char *argv[])
{
  static FAR const char *pwrmntr_dev = "/dev/pwrmntr0";
  float float_current;
  float float_voltage;
  ltc4151_t ltc;
  int pwrmntr_fd;
  int samples;
  int sample;

  samples = 1;
  if (argc > 1)
    {
      samples = atoi(argv[1]);
    }

  pwrmntr_fd = open(pwrmntr_dev, O_RDONLY);
  if (pwrmntr_fd < 0)
    {
      printf("Failed to open %s: %d\n", pwrmntr_dev, errno);
      return 1;
    }

  for (sample = 0; sample < samples; ++sample)
    {
      if (read(pwrmntr_fd, &ltc, sizeof(ltc)) < 0)
        {
          printf("Failed to read from %s: %d\n", pwrmntr_dev, errno);
          return 1;
        }

      float_current = b16tof(ltc.current);
      float_voltage = b16tof(ltc.voltage);

      printf("Current: %d.%03dmA - Voltage: %d.%03dV\n",
             (int)float_current, DECIMAL_PLACES3(float_current),
             (int)float_voltage, DECIMAL_PLACES3(float_voltage));
    }

  close(pwrmntr_fd);
  return 0;
}
