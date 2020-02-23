/****************************************************************************
 * apps/graphics/ft80x/ft80x_touch.c
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
#include <signal.h>
#include <assert.h>
#include <errno.h>

#include <nuttx/lcd/ft80x.h>

#include "graphics/ft80x.h"
#include "ft80x.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_touch_gettransform
 *
 * Description:
 *   Read the touch transform matrix
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   matrix - The location to return the transform matrix
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_touch_gettransform(int fd, FAR uint32_t matrix[6])
{
  int ret;

  /* Read FT80X_REG_TOUCH_TRANSFORM_A through FT80X_REG_TOUCH_TRANSFORM_F */

#ifdef CONFIG_LCD_FT800
  ret = ft80x_getregs(fd, FT80X_REG_TOUCH_TRANSFORM_A, 6, matrix);
#else
  ret = ft80x_getregs(fd, FT80X_REG_CTOUCH_TRANSFORM_A, 6, matrix);
#endif

  if (ret < 0)
    {
      ft80x_err("ERROR:  ft80x_getregs failed: %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_touch_tag
 *
 * Description:
 *   Read the current touch tag.  The touch tag is an 8-bit value
 *   identifying the specific graphics object on the screen that is being
 *   touched. The value zero indicates that there is no graphic object being
 *   touched.
 *
 *   Only a single touch can be queried.  For the FT801 in "extended",
 *   multi-touch mode, this value indicates only the tag associated with
 *   touch 0.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *
 * Returned Value:
 *   A value of 1-255 is returned if a graphics object is touched.  Zero is
 *   returned if no graphics object is touched.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_touch_tag(int fd)
{
  uint8_t tag;
  int ret;

#if defined(CONFIG_LCD_FT800)
  /* Read REG_TOUCH_TAG */

  ret = ft80x_getreg8(fd, FT80X_REG_TOUCH_TAG, &tag);

#elif defined(CONFIG_LCD_FT801)
  /* Read REG_CTOUCH_TAG */

  ret = ft80x_getreg8(fd, FT80X_REG_CTOUCH_TAG, &tag);

#endif

  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg8 failed: %d\n", ret);
      return ret;
    }

  return (int)tag;
}

/****************************************************************************
 * Name: ft80x_touch_waittag
 *
 * Description:
 *   Wait until there is a change in the touch tag.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   oldtag - The previous tag value.  This function will return when the
 *            current touch tag differs from this value.
 *
 * Returned Value:
 *   A value of 1-255 is returned if a graphics object is touched.  Zero is
 *   returned if no graphics object is touched.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_touch_waittag(int fd, uint8_t oldtag)
{
  struct ft80x_notify_s notify;
  struct timespec timeout;
  sigset_t set;
  uint8_t tag;
  int ret;

  /* Block the signal so that it will pend if received asynchronously */

  sigemptyset(&set);
  sigaddset(&set, CONFIG_GRAPHICS_FT80X_TAG_SIGNAL);

  ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
  if (ret < 0)
    {
      ret = -errno;
      ft80x_err("ERROR: pthread_sigmask for signal %d failed: %d\n",
                CONFIG_GRAPHICS_FT80X_TAG_SIGNAL, ret);
      return ret;
    }

  notify.event.sigev_notify = SIGEV_SIGNAL;
  notify.event.sigev_signo  = CONFIG_GRAPHICS_FT80X_TAG_SIGNAL;

  notify.pid    = getpid();
  notify.id     = FT80X_NOTIFY_TAG;
  notify.enable = false;

  for (; ; )
    {
      /* Read the current touch tag. */

      ret = ft80x_touch_tag(fd);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_touch_tag failed: %d\n", ret);
          return ret;
        }

      /* Check if the touch tag has already changed */

      tag = (uint8_t)(ret & 0xff);
      ft80x_info("oldtag=%u tag=%u\n", oldtag, tag);

      if (tag == oldtag)
        {
          /* The tag has changed... return success */

          ret = OK;
          break;
        }

      /* Set up to receive a notification when the touch tag changes.
       * NOTE that there is a race condition here between the preceding test
       * and the time when the event is registered.  We catch this with a
       * timeout below.
       */

      notify.enable = true;
      ret           = ioctl(fd, FT80X_IOC_EVENTNOTIFY,
                            (unsigned long)((uintptr_t)&notify));
      if (ret < 0)
        {
          ret = -errno;
          ft80x_err("ERROR: ioctl(FT80X_IOC_EVENTNOTIFY) failed: %d\n", ret);
          break;
        }

      /* Now wait for the signal event (or a timeout) */

      timeout.tv_sec  = 0;
      timeout.tv_nsec = 400 * 1000 * 1000;

      ret = sigtimedwait(&set, NULL, &timeout);

      /* Make sure that the event notification is again disabled */

      notify.enable = false;
      ioctl(fd, FT80X_IOC_EVENTNOTIFY,
            (unsigned long)((uintptr_t)&notify));

      /* Check if the signal was received correctly or if the timeout occurred. */

      if (ret < 0)
        {
          int errcode = errno;
          if (errcode != EAGAIN)
            {
              ft80x_err("ERROR: ioctl(FT80X_IOC_EVENTNOTIFY) failed: %d\n",
                        errcode);
              ret = -errcode;
              break;
            }
        }
    }

  pthread_sigmask(SIG_UNBLOCK, &set, NULL);
  return tag;
}

/****************************************************************************
 * Name: ft80x_touch_info
 *
 * Description:
 *   Return the current touch tag and touch position information.
 *
 *   For the FT801 in "extended", multi-touch mode, the tag value indicates
 *   only the tag associated with touch 0.
 *
 *   Touch positions of -32768 indicate that no touch is detected.
 *
 * Input Parameters:
 *   fd   - The file descriptor of the FT80x device.  Opened by the caller
 *          with write access.
 *   info - Location in which to return the touch information
 *
 * Returned Value:
 *   A value of 1-255 is returned if a graphics object is touched.  Zero is
 *   returned if no graphics object is touched.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_touch_info(int fd, FAR struct ft80x_touchinfo_s *info)
{
  int ret;

  DEBUGASSERT(info != NULL);

#if defined(CONFIG_LCD_FT800)
  /* Read REG_TOUCH_TAG */

  ret = ft80x_getreg8(fd, FT80X_REG_TOUCH_TAG, &info->tag);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg8 failed: %d\n", ret);
      return ret;
    }

  /* Read the FT80X_REG_TOUCH_TAG_XY register */

  ret = ft80x_getreg32(fd, FT80X_REG_TOUCH_TAG_XY, &info->tagpos.xy);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg32 failed: %d\n", ret);
      return ret;
    }

  /* Read the FT80X_REG_TOUCH_RZ register */

  ret = ft80x_getreg16(fd, FT80X_REG_TOUCH_RZ,
                       (FAR uint16_t *)&info->pressure);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg16 failed: %d\n", ret);
      return ret;
    }

  /* Read the FT80X_REG_TOUCH_SCREEN_XY register */

  ret = ft80x_getreg32(fd, FT80X_REG_TOUCH_SCREEN_XY, &info->pos.xy);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg32 failed: %d\n", ret);
      return ret;
    }

#elif defined(CONFIG_LCD_FT801)
  /* Read REG_CTOUCH_TAG */

  ret = ft80x_getreg8(fd, FT80X_REG_CTOUCH_TAG, &info->tag);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg8 failed: %d\n", ret);
      return ret;
    }

  /* Read the FT80X_REG_CTOUCH_TAG_XY register */

  ret = ft80x_getreg32(fd, FT80X_REG_CTOUCH_TAG_XY, &info->tagpos.xy);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg32 failed: %d\n", ret);
      return ret;
    }

#ifndef CONFIG_LCD_FT801_MULTITOUCH
  /* Read the FT80X_REG_CTOUCH_TOUCH0_XY register */

  ret = ft80x_getreg32(fd, FT80X_REG_CTOUCH_TOUCH0_XY, &info->pos.xy);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg32 failed: %d\n", ret);
      return ret;
    }

#else
  /* Read the FT80X_REG_CTOUCH_TOUCH0_XY register */

  ret = ft80x_getreg32(fd, FT80X_REG_CTOUCH_TOUCH0_XY, &info->pos[0].xy);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg32 failed: %d\n", ret);
      return ret;
    }

  /* Read the FT80X_REG_CTOUCH_TOUCH1_XY register */

  ret = ft80x_getreg32(fd, FT80X_REG_CTOUCH_TOUCH1_XY, &info->pos[1].xy);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg32 failed: %d\n", ret);
      return ret;
    }

  /* Read the FT80X_REG_CTOUCH_TOUCH2_XY register */

  ret = ft80x_getreg32(fd, FT80X_REG_CTOUCH_TOUCH2_XY, &info->pos[2].xy);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg32 failed: %d\n", ret);
      return ret;
    }

  /* Read the FT80X_REG_CTOUCH_TOUCH3_XY register */

  ret = ft80x_getreg32(fd, FT80X_REG_CTOUCH_TOUCH3_XY, &info->pos[3].xy);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg32 failed: %d\n", ret);
      return ret;
    }

  /* Read the FT80X_REG_CTOUCH_TOUCH4_X and Y registers */

  ret = ft80x_getreg16(fd, FT80X_REG_CTOUCH_TOUCH4_X,
                       (FAR uint16_t *)&info->pos[4].u.x);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg16 failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_getreg16(fd, FT80X_REG_CTOUCH_TOUCH4_Y,
                       (FAR uint16_t *)&info->pos[4].u.y);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg16 failed: %d\n", ret);
      return ret;
    }

#endif /* CONFIG_LCD_FT801_MULTITOUCH */
#endif /* CONFIG_LCD_FT800/1 */

  return OK;
}
