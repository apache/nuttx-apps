/****************************************************************************
 * apps/examples/apa102/apa102_main.c
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

#include <nuttx/leds/apa102.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONFIG_EXAMPLES_APA102_DEVNAME "/dev/leddrv0"

/****************************************************************************
 * Private Data
 ****************************************************************************/

#define NUM_LEDS  30
#define US_DELAY  1000

struct apa102_ledstrip_s ledstrip[NUM_LEDS];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hsvtorgb
 *
 * Description:
 *   Converts a color from HSV to RGB.
 *
 *   Note: This function is based on Pololu example:
 *   https://github.com/pololu/apa102-arduino/blob/master/examples/Rainbow/Rainbow.ino
 *
 * Input Parameters:
 *   h is hue, as a number between 0 and 360.
 *   s is the saturation, as a number between 0 and 255.
 *   v is the value, as a number between 0 and 255.
 *
 ****************************************************************************/

static struct apa102_ledstrip_s hsvtorgb(uint16_t h, uint8_t s, uint8_t v)
{
  struct apa102_ledstrip_s led;
  uint8_t f = (h % 60) * 255 / 60;
  uint8_t p = (255 - s) * (uint16_t)v / 255;
  uint8_t q = (255 - f * (uint16_t)s / 255) * (uint16_t)v / 255;
  uint8_t t = (255 - (255 - f) * (uint16_t)s / 255) * (uint16_t)v / 255;
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  switch((h / 60) % 6)
    {
      case 0:
        r = v;
        g = t;
        b = p;
        break;

      case 1:
        r = q;
        g = v;
        b = p;
        break;

      case 2:
        r = p;
        g = v;
        b = t;
        break;

      case 3:
        r = p;
        g = q;
        b = v;
        break;

      case 4:
        r = t;
        g = p;
        b = v;
        break;

      case 5:
        r = v;
        g = p;
        b = q;
        break;
    }

  led.red    = r;
  led.green  = g;
  led.blue   = b;
  led.bright = 0;

  return led;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * apa102_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int ret;
  int led;
  int miles = 10000;

  fd = open(CONFIG_EXAMPLES_APA102_DEVNAME, O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_EXAMPLES_APA102_DEVNAME, errno);
      return -1;
    }

  for (; ; )
    {
      /* Fill the LED Strip */

      for (led = 0; led < NUM_LEDS; led++)
        {
          uint8_t p = miles - led * 8;
          ledstrip[led] = hsvtorgb((uint32_t)p * 359 / 256, 255, 255);
        }

      /* Write the new colors in all the LEDs */

      ret = write(fd, ledstrip, 4 * NUM_LEDS);
      if (ret < 0)
        {
          _err("ERROR: write LED Strip failed: %d\n", errno);
        }

      usleep(US_DELAY);
      miles += (US_DELAY / 1000);
    }

  close(fd);
  return 0;
}
