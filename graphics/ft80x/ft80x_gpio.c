/****************************************************************************
 * apps/graphics/ft80x/ft80x_gpio.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include <nuttx/lcd/ft80x.h>

#include "graphics/ft80x.h"
#include "ft80x.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_gpio_configure
 *
 * Description:
 *   Configure an FT80x GPIO pin
 *
 * Input Parameters:
 *   fd    - The file descriptor of the FT80x device.  Opened by the caller
 *           with write access.
 *   gpio  - Identifies the GPIO pin {0,1}
 *   dir   - Direction:  0=input, 1=output
 *   drive - Common output drive strength for GPIO 0 and 1 (see
 *           FT80X_GPIO_DRIVE_* definitions).  Default is 4mA.
 *   value - Initial value for output pins
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_gpio_configure(int fd, uint8_t gpio, uint8_t dir, uint8_t drive,
                         bool value)
{
  uint8_t regval8;
  int ret;

  DEBUGASSERT(gpio == 0 || gpio == 1 || gpio == 7);
  DEBUGASSERT(dir == 0 || dir == 1);
  DEBUGASSERT((drive & ~FT80X_GPIO_DRIVE_MASK) == 0);

  /* Set the pin drive strength and output value */

  ret = ft80x_getreg8(fd, FT80X_REG_GPIO, &regval8);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg8 failed; %d\n", ret);
      return ret;
    }

  regval8 &= ~(FT80X_GPIO_DRIVE_MASK | (1 << gpio));
  regval8 |= drive;

  if (value)
    {
      regval8 |= (1 << gpio);
    }

  ret = ft80x_putreg8(fd, FT80X_REG_GPIO, regval8);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_putreg8 failed; %d\n", ret);
      return ret;
    }

  /* Set the pin direction */


  ret = ft80x_getreg8(fd, FT80X_REG_GPIO_DIR, &regval8);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg8 failed; %d\n", ret);
      return ret;
    }

  if (dir == 0)
    {
      regval8 &= ~(1 << gpio);
    }
  else
    {
      regval8 |= (1 << gpio);
    }

  ret = ft80x_putreg8(fd, FT80X_REG_GPIO_DIR, regval8);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_putreg8 failed; %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_gpio_write
 *
 * Description:
 *   Write a value to a pin configured for output
 *
 * Input Parameters:
 *   fd    - The file descriptor of the FT80x device.  Opened by the caller
 *           with write access.
 *   gpio  - Identifies the GPIO pin {0,1}
 *   value - True: high, false: low
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_gpio_write(int fd, uint8_t gpio, bool value)
{
  uint8_t regval8;
  int ret;

  DEBUGASSERT(gpio == 0 || gpio == 1 || gpio == 7);

  /* Set the output value */

  ret = ft80x_getreg8(fd, FT80X_REG_GPIO, &regval8);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg8 failed; %d\n", ret);
      return ret;
    }

  if (value)
    {
      regval8 |= (1 << gpio);
    }
  else
    {
      regval8 &= ~(1 << gpio);
    }

  ret = ft80x_putreg8(fd, FT80X_REG_GPIO, regval8);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_putreg8 failed; %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_gpio_read
 *
 * Description:
 *   Read the value from a pin configured for input
 *
 * Input Parameters:
 *   fd   - The file descriptor of the FT80x device.  Opened by the caller
 *          with write access.
 *   gpio - Identifies the GPIO pin {0,1}
 *
 * Returned Value:
 *   True: high, false: low
 *
 ****************************************************************************/

bool ft80x_gpio_read(int fd, uint8_t gpio)
{
  uint8_t regval8;
  int ret;

  DEBUGASSERT(gpio == 0 || gpio == 1 || gpio == 7);

  /* Return the input value */

  ret = ft80x_getreg8(fd, FT80X_REG_GPIO, &regval8);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg8 failed; %d\n", ret);
      return false;
    }

  return (regval8 & (1 << gpio)) != 0;
}
