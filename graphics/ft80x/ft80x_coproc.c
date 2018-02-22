/****************************************************************************
 * apps/graphics/ft80x/ft80x_coproc.c
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
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include <nuttx/lcd/ft80x.h>

#include "graphics/ft80x.h"
#include "ft80x.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HALF_SECOND  (500 * 1000)  /* 500 milliseconds (units = microseconds) */
#define FIVE_SECONDS (5 * 2)       /* 5 seconds (units = half seconds) */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_coproc_waitlogo
 *
 * Description:
 *   Wait for the logo animation to complete.  The logo command causes the
 *   co-processor engine to play back a short animation of the FTDI logo.
 *   During logo playback the MCU should not access any FT800 resources.
 *   After 2.5 seconds have elapsed, the co-processor engine writes zero to
 *   REG_CMD_READ and REG_CMD_WRITE, and starts waiting for commands.  After
 *   this command is complete, the MCU shall write the next command to the
 *   starting address of RAM_CMD.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_coproc_waitlogo(int fd)
{
  uint16_t head;
  uint16_t tail;
  int elapsed;
  int ret;

  /* Loop until both REG_CMD_READ and REG_CMD_WRITE are zero or until 5
   * seconds elapses.
   */

  for (elapsed = 0; elapsed < FIVE_SECONDS; elapsed++)
    {
      /* Read REG_CMD_WRITE */

      ret = ft80x_getreg16(fd, FT80X_REG_CMD_WRITE, &head);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_getreg16 failed: %d\n", ret);
          return ret;
        }

      /* Check if FT80X_REG_CMD_WRITE is zero */

      if (head == 0)
        {
          /* Read REG_CMD_READ */

          ret = ft80x_getreg16(fd, FT80X_REG_CMD_READ, &tail);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_getreg16 failed: %d\n", ret);
              return ret;
            }

          /* If REG_CMD_READ is also zero, then we are done */

          if (tail == 0)
            {
              /* The animation is done! */

              return OK;
            }
        }

      /* Wait for a half a second */

      (void)usleep(HALF_SECOND);
    }

  ft80x_err("ERROR:  Timed out!  Last head/tail = %u/%u\n", head, tail);
  return -ETIMEDOUT;
}
