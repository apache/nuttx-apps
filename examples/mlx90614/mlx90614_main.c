/****************************************************************************
 * apps/examples/mlx90614/mlx90614_main.c
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
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include <nuttx/sensors/ioctl.h>
#include <nuttx/sensors/mlx90614.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONFIG_EXAMPLES_MLX90614_DEVNAME "/dev/therm0"

/* If MLX90614 has two internal thermopile change it to 1 */

#define MLX90614_DUALTHERMOPILE  0

/****************************************************************************
 * Private Data
 ****************************************************************************/

#define MILISEC_DELAY  1000

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * mlx90614_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct mlx90614_temp_s temp;
  float ta;
  float tobj1;
#if MLX90614_DUALTHERMOPILE
  float tobj2;
#endif
  int fd;
  int ret;

  fd = open(CONFIG_EXAMPLES_MLX90614_DEVNAME, O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_EXAMPLES_MLX90614_DEVNAME, errno);
      return -1;
    }

  /* If user passed a new address */

  if (argc == 2)
    {
      FAR char *buffer;
      uint8_t newaddr;

      buffer = argv[1];

      if (buffer[0] != '0' && buffer[1] != 'x')
        {
          fprintf(stderr, "You need to pass the I2C address in hexa: 0xNN\n");
          goto out;
        }

      newaddr = strtol(buffer, NULL, 16);

      if (newaddr == 0 || newaddr > 0x7f)
        {
          fprintf(stderr, "Inform a valid I2C address: 0x01 to 0x7f\n");
          goto out;
        }

      ret = ioctl(fd, SNIOC_CHANGE_SMBUSADDR,  (unsigned long)((uintptr_t)&newaddr));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: ioctl(SNIOC_CHANGE_SMBUSADDR) failed: %d\n", errno);
          goto out;
        }

      printf("Address 0x%02x stored on your device!\n", newaddr);
      printf("Please do a power cycle and update the I2C board address!\n");

      goto out;
    }

  for (; ; )
    {
      /* Read new temperature from sensor */

      ret = read(fd, &temp, sizeof(struct mlx90614_temp_s));
      if (ret < 0)
        {
          fprintf(stderr, "Failed to read MLX90614, ERROR: %d\n", errno);
        }

      ta    = (temp.ta * 0.02) - 273.15;
      tobj1 = (temp.tobj1 * 0.02) - 273.15;
#if MLX90614_DUALTHERMOPILE
      tobj2 = (temp.tobj2 * 0.02) - 273.15;
#endif

      printf("Ambient Temperature = %.2f ", ta);
      printf("| Object 1 Temperature = %.2f ", tobj1);
#if MLX90614_DUALTHERMOPILE
      printf("| Object 2 Temperature = %.2f", tobj2);
#endif
      printf("\n");

      usleep(MILISEC_DELAY * 1000);
    }

out:

  close(fd);
  return 0;
}
