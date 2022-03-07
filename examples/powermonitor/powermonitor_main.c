/****************************************************************************
 * apps/examples/powermonitor/powermonitor_main.c
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

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DECIMAL_PLACES3(x) abs(((int)(((x)-((int)x))*1000)))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

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
