/****************************************************************************
 * apps/system/lm75/lm75.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
 *   Authors: Gregory Nutt <gnutt@nuttx.org>
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <fixedmath.h>

#include <nuttx/sensors/lm75.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_SYSTEM_LM75_DEVNAME
#  warning CONFIG_SYSTEM_LM75_DEVNAME is not defined
#  define CONFIG_SYSTEM_LM75_DEVNAME "/dev/temp"
#endif

#if !defined(CONFIG_SYSTEM_LM75_FAHRENHEIT) && !defined(CONFIG_SYSTEM_LM75_CENTIGRADE)
#  warning one of CONFIG_SYSTEM_LM75_FAHRENHEIT or CONFIG_SYSTEM_LM75_CENTIGRADE must be defined
#  defeind CONFIG_SYSTEM_LM75_FAHRENHEIT 1
#endif

#if defined(CONFIG_SYSTEM_LM75_FAHRENHEIT) && defined(CONFIG_SYSTEM_LM75_CENTIGRADE)
#  warning both of CONFIG_SYSTEM_LM75_FAHRENHEIT and CONFIG_SYSTEM_LM75_CENTIGRADE defined
#  undef CONFIG_SYSTEM_LM75_CENTIGRADE
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int lm75_main(int argc, char *argv[])
#endif
{
#ifdef CONFIG_LIBC_FLOATINGPOINT
  double temp;
#endif
  b16_t temp16;
  ssize_t nbytes;
  int fd;
  int ret;

  /* Open the temperature sensor device */

  fd  = open(CONFIG_SYSTEM_LM75_DEVNAME, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_SYSTEM_LM75_DEVNAME, errno);
      return EXIT_FAILURE;
    }

#ifdef CONFIG_SYSTEM_LM75_FAHRENHEIT
  /* Select Fahrenheit scaling */

  ret = ioctl(fd, SNIOC_FAHRENHEIT, 0);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(SNIOC_FAHRENHEIT) failed: %d\n",
              errno);
      return EXIT_FAILURE;
    }
#else
  /* Select Fahrenheit scaling */

  ret = ioctl(fd, SNIOC_CENTIGRADE, 0);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(SNIOC_CENTIGRADE) failed: %d\n",
              errno);
      return EXIT_FAILURE;
    }
#endif

  /* Read the current temperature as a fixed precision number */

  nbytes = read(fd, &temp16, sizeof(b16_t));
  close(fd);

  if (nbytes < 0)
    {
      fprintf(stderr, "ERROR: read(%d) failed: %d\n",
              sizeof(b16_t), errno);
      return EXIT_FAILURE;
    }

  if (nbytes != sizeof(b16_t))
    {
      fprintf(stderr, "ERROR: Unexpected read size: %Ld vs %d\n",
              (long)nbytes, sizeof(b16_t));
      return EXIT_FAILURE;
    }

  /* Print the current temperature on stdout */

#ifdef CONFIG_LIBC_FLOATINGPOINT
  temp = (double)temp16 / 65536.0;
#ifdef CONFIG_SYSTEM_LM75_FAHRENHEIT
  printf("%3.2f degrees Fahrenheit\n", temp);
#else
  printf("%3.2f degrees Centigrade\n", temp);
#endif

#else
#ifdef CONFIG_SYSTEM_LM75_FAHRENHEIT
  printf("0x%04x.%04x degrees Fahrenheit\n", temp16 >> 16, temp16 & 0xffff);
#else
  printf("0x%04x.%04x degrees Centigrade\n", temp16 >> 16, temp16 & 0xffff);
#endif
#endif

  return EXIT_SUCCESS;
}
