/****************************************************************************
 * apps/graphics/ft80x/ft80x_backlight.c
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
 * Name: ft80x_backlight_set
 *
 * Description:
 *   Set the backlight intensity via the PWM duty.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   duty   - The new backlight duty (as a percentage 0..100)
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_backlight_set(int fd, uint8_t duty)
{
  uint16_t duty128;
  int ret;

  DEBUGASSERT(duty <= 100);

  /* 0% corresponds to the value 0, but 100% corresponds to the value 128 */

  duty128 = ((uint16_t)duty << 7) / 100;

  /* Perform the IOCTL to set the backlight duty */

  ret = ft80x_putreg8(fd, FT80X_REG_PWM_DUTY, (uint8_t)duty128);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_putreg8 failed: %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_backlight_fade
 *
 * Description:
 *   Change the backlight intensity with a controllable fade.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   duty   - The terminal duty (as a percentage 0..100)
 *   delay  - The duration of the fade in milliseconds (10..16700)
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_backlight_fade(int fd, uint8_t duty, uint16_t delay)
{
  struct ft80x_fade_s fade;
  int ret;

  DEBUGASSERT(duty <= 100);

  /* Perform the IOCTL to perform the fade */

  fade.duty  = duty;
  fade.delay = delay;

  ret = ioctl(fd, FT80X_IOC_FADE, (unsigned long)((uintptr_t)&fade));
  if (ret < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: ioctl(FT80X_IOC_FADE) failed: %d\n", errcode);
      ret = -errcode;
    }

  return ret;
}
