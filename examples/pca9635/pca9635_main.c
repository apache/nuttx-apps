/****************************************************************************
 * apps/examples/pca9635/pca9635_main.c
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include <nuttx/leds/pca9635pw.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONFIG_EXAMPLES_PCA9635_DEVNAME "/dev/leddrv0"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * pca9635_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct pca9635pw_brightness_s ledbright;
  int led;
  int bright;
  int fd;
  int ret;

  fd = open(CONFIG_EXAMPLES_PCA9635_DEVNAME, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_EXAMPLES_PCA9635_DEVNAME, errno);
      return -1;
    }

  for (; ; )
    {
      for (bright = 0; bright <= 255; bright++)
        {
          for (led = LED_0; led <= LED_15; led++)
            {
              ledbright.led = led;
              ledbright.brightness = bright;

              ret = ioctl(fd, PWMIOC_SETLED_BRIGHTNESS,
                          (unsigned long)&ledbright);
              if (ret < 0)
                {
                  _err("ERROR: ioctl(PWMIOC_SETLED_BRIGHTNESS) failed: %d\n",
                        errno);
                }
            }

          usleep(100);
        }
    }

  close(fd);
  return 0;
}
