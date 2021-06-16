/****************************************************************************
 * apps/examples/rgbled/rgbled.c
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_RGBLED_DEVNAME
#  define CONFIG_EXAMPLES_RGBLED_DEVNAME "/dev/rgbled0"
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * rgbled_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int red = 255;
  int green = 0;
  int blue = 0;
  int sred = -1;
  int sgreen = 1;
  int sblue = 0;
  int fd;
  char buffer[8];

  fd = open(CONFIG_EXAMPLES_RGBLED_DEVNAME, O_WRONLY);

  if (fd < 0)
    {
      printf("Error opening %s!\n", CONFIG_EXAMPLES_RGBLED_DEVNAME);
      return -1;
    }

  while(1)
  {
    red   += sred;
    green += sgreen;
    blue  += sblue;

    if (green == 255)
      {
        sred   = 0;
        sgreen = -1;
        sblue  = 1;
      }

    if (blue == 255)
      {
        sred   = 1;
        sgreen = 0;
        sblue  = -1;
      }

    if (red == 255)
      {
        sred   = -1;
        sblue  = 0;
        sgreen = 1;
      }

    sprintf(buffer, "#%02X%02X%02X", red, green, blue);
    write(fd, buffer, 8);
    usleep(5000);
  }

  return 0;
}
