/****************************************************************************
 * examples/apa102/apa102_main.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Copyright (c) 2015-2017 Pololu Corporation.
 *   Author: Alan Carvalho de Assis <acassis@gmail.com>
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
