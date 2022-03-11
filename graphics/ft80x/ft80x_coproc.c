/****************************************************************************
 * apps/graphics/ft80x/ft80x_coproc.c
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
 * Name: ft80x_coproc_send
 *
 * Description:
 *   Send commands to the co-processor via the CMD RAM FIFO.  This function
 *   will not return until the command has been consumed by the co-processor.
 *
 *   NOTE:  This command is not appropriate use while a display is being
 *   formed.  It is will mess up the CMD RAM FIFO offsets managed by the
 *   display list logic.
 *
 * Input Parameters:
 *   fd    - The file descriptor of the FT80x device.  Opened by the caller
 *           with write access.
 *   cmds  - A list of 32-bit commands to be sent.
 *   ncmds - The number of commands in the list.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_coproc_send(int fd, FAR const uint32_t *cmds, size_t ncmds)
{
  return ft80x_ramcmd_append(fd, cmds, ncmds << 2);
}

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
 *   fd - The file descriptor of the FT80x device.  Opened by the caller
 *        with write access.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_coproc_waitlogo(int fd)
{
  uint32_t regs[2];
  uint16_t head;
  uint16_t tail;
  int elapsed;
  int ret;

  /* Loop until both REG_CMD_READ and REG_CMD_WRITE are zero or until 5
   * seconds elapses.
   */

  for (elapsed = 0; elapsed < FIVE_SECONDS; elapsed++)
    {
      /* Read both the FT80X_REG_CMD_WRITE and FT80X_REG_CMD_READ registers. */

      ret = ft80x_getregs(fd, FT80X_REG_CMD_READ, 2, regs);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_getregs failed: %d\n", ret);
          return ret;
        }

      /* Check if both have been reset to zero. */

      head = regs[1] & FT80X_CMDFIFO_MASK;
      tail = regs[0] & FT80X_CMDFIFO_MASK;

      if (head == 0 && tail == 0)
        {
          /* The animation is done! */

          return OK;
        }

      /* Wait for a half of a second */

      usleep(HALF_SECOND);
    }

  ft80x_err("ERROR:  Timed out!  Last head/tail = %u/%u\n", head, tail);
  return -ETIMEDOUT;
}
