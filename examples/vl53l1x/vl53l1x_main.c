/****************************************************************************
 * examples/vl53l1x/vl53l1x_main.c
 *
 *   Copyright (C) 2019 Acutronics Robotics. All rights reserved.
 *   Author: Acutronics Robotics (Juan Flores Muñoz) <juan@erlerobotics.com>
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
#include <stdlib.h>
#include <fcntl.h>

#include <nuttx/sensors/vl53l1x.h>
#include <nuttx/sensors/ioctl.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void error_message()
{
  printf("VL53L1X help usage:\n");
  printf("vl53l1x -m <measure_mode> -c <calibration_distance>\n ");
  printf("measure_mode:\n");
  printf("  short-> Up to 1.3 mtr and higher precision\n");
  printf("  long-> Up to 4 mtr and lower precision\n");
  printf("calibration_distance:\n");
  printf("  Set a object to a known distance. "
         "Valid value between 10mm and 400mm or none\n");
}

/****************************************************************************
 * hello_main
 ****************************************************************************/

#if defined(BUILD_MODULE)
int main(int argc, FAR char *argv[])
#else
int vl53l1x_main(int argc, char *argv[])
#endif
{
  uint16_t calib_value = 0;     /* Variable to save the calibration value */
  int fd;                       /* VL53L1X file descriptor */
  uint16_t result;              /* Variable to save the return data of the
                                 * read.
                                 */

  if (argc == 1 || (argc == 2) && !strcmp(argv[1], "-h") || argc > 5 ||
      argc < 4)
    {
      error_message();          /* Just show the instructions of how to set-up
                                 * this App
                                 */
      return 0;
    }

  /* Opening the sensor with read only permission */

  fd = open("/dev/tof0", O_RDONLY);

  /* Setting the measure mode. */

  if (!strcmp(argv[2], "long"))
    {
      ioctl(fd, SNIOC_DISTANCELONG);
    }
  else if (!strcmp(argv[2], "short"))
    {
      ioctl(fd, SNIOC_DISTANCESHORT);
    }
  else
    {
      error_message();          /* Just show the instructions of how to set-up
                                 * this App
                                 */
      close(fd);
      return 0;
    }

  /* Set the calibration value */

  if (argc == 5)
    {
      calib_value = atoi(argv[4]);
      if (calib_value > 10 || calib_value < 4000)
        {
          ioctl(fd, SNIOC_CALIBRATE, calib_value);
        }
      else
        {
          printf("Error: Calibration not set, value out of range\n");
        }
    }

  while (1)
    {
      /* Read the distance every 100 mS. */

      read(fd, &result, sizeof(result));
      printf("Distance %i mm \n", result);
      usleep(100000);
    }
}
