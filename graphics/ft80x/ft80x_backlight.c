/****************************************************************************
 * apps/graphics/ft80x/ft80x_backlight.c
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
