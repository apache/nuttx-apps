/****************************************************************************
 * apps/graphics/ft80x/ft80x_ramcmd.c
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
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

#include <nuttx/lcd/ft80x.h>

#include "graphics/ft80x.h"
#include "ft80x.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FT80X_CMDFIFO_MASK    (FT80X_CMDFIFO_SIZE - 1)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_ramcmd_offset
 *
 * Description:
 *   Return the offset to next unused location in RAM CMD
 *
 * Input Parameters:
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *
 * Returned Value:
 *   The offset to the next unused location in RAM CMD.
 *
 ****************************************************************************/

static inline uint16_t ft80x_ramcmd_offset(FAR struct ft80x_dlbuffer_s *buffer)
{
  register uint32_t tmp = buffer->dlsize + buffer->hwoffset;
  return  (uint16_t)(tmp & FT80X_CMDFIFO_MASK);
}

/****************************************************************************
 * Name: ft80x_ramcmd_fifopos
 *
 * Description:
 *   Return the current FIFO position RAM CMD
 *
 * Input Parameters:
 *   fd  - The file descriptor of the FT80x device.  Opened by the caller
 *         with write access.
 *   pos - Pointer to location to return the FIFO position
 *
 * Returned Value:
 *   The current FIFO position in RAM CMD.
 *
 ****************************************************************************/

static inline int ft80x_ramcmd_fifopos(int fd, FAR uint16_t *pos)
{
  int ret = ft80x_getreg16(fd, FT80X_REG_CMD_READ, pos);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg16 failed: %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_ramcmd_freespace
 *
 * Description:
 *   Return the free space in RAM CMD memory
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *   avail  - Pointer to location to return the FIFO free space
 *
 * Returned Value:
 *   The (positive) number of free bytes in RAM CMD on success.  A negated
 *   errno value is returned on any failure.
 *
 ****************************************************************************/

static uint16_t ft80x_ramcmd_freespace(int fd,
                                       FAR struct ft80x_dlbuffer_s *buffer,
                                       FAR uint16_t *avail)
{
  uint16_t head;
  uint16_t tail;
  int ret;

  /* Index to the next available location in RAM CMD */

  head = ft80x_ramcmd_offset(buffer);

  /* The index to next unconsumed value in RAM CMD */

  ret = ft80x_ramcmd_fifopos(fd, &tail);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_ramcmd_fifopos failed: %d\n", ret);
      return ret;
    }

  /* Return the free space in the FIFO.  NOTE that 4 bytes are not available */

  *avail = (FT80X_CMDFIFO_SIZE - 4) - ((head - tail) & FT80X_CMDFIFO_MASK);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_ramcmd_append
 *
 * Description:
 *   Append new display list data to RAM CMD
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *   data   - A pointer to the start of the data to be written.
 *   len    - The number of bytes to be written.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_ramcmd_append(int fd, FAR struct ft80x_dlbuffer_s *buffer,
                        FAR const void *data, size_t len)
{
  struct ft80x_relmem_s wrdesc;
  FAR const uint8_t *src;
  ssize_t remaining = 0;
  size_t wrsize = 0;
  uint16_t maxsize;
  int ret;

  /* Loop until all of the display list commands have been transferred to
   * FIFO.
   */

  src       = data;
  remaining = len;

  do
    {
      /* Write the number of bytes remaining to be transferred */

      wrsize = remaining;

      /* Get the amount of free space in the FIFO. */

      maxsize = 0;
      ret     = ft80x_ramcmd_freespace(fd, buffer, &maxsize);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_ramcmd_freespace() failed: %d\n", ret);
          return ret;
        }

      /* If the FIFO full?
       *
       * REVISIT:  Will the FIFO be consumed as we generate the display
       * list?  Is it fatal if it becomes full?  Or could we wait for space
       * to free up.
       */

      if (maxsize == 0)
        {
          ft80x_err("ERROR: ft80x_ramcmd_freespace() failed: %d\n", ret);
          return -ENOSPC;
        }

      /* Limit the write size to the size of the available FIFO memory */

      if (wrsize > (size_t)maxsize)
        {
          wrsize = (size_t)maxsize;
        }

      /* Perform the transfer */

      wrdesc.offset = ft80x_ramcmd_offset(buffer);
      wrdesc.nbytes = wrsize;
      wrdesc.value  = (FAR void *)src;  /* Discards 'const' qualifier */

      ret = ioctl(fd, FT80X_IOC_PUTRAMCMD,
                  (unsigned long)((uintptr_t)&wrdesc));
      if (ret < 0)
        {
          int errcode = errno;
          ft80x_err("ERROR: ioctl() FT80X_IOC_PUTRAMCMD failed: %d\n",
                    errcode)
          return -errcode;
        }

      /* Update the command FIFO */

      buffer->dlsize += wrsize;
      ret = ft80x_putreg16(fd, FT80X_REG_CMD_WRITE,
                           ft80x_ramcmd_offset(buffer));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_putreg16() failed: %d\n", ret);
          return ret;
        }

      /* Wait for the FIFO to empty */

      ret = ft80x_ramcmd_waitfifoempty(fd, buffer);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_ramcmd_waitfifoempty() failed: %d\n", ret);
          return ret;
        }

      /* Set up for the next time through the loop */

      remaining -= wrsize;
    }
  while (remaining > 0);

  return OK;
}

/****************************************************************************
 * Name: ft80x_ramcmd_waitfifoempty
 *
 * Description:
 *   Wait until co processor completes the operation and the FIFO is again
 *   empty.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_ramcmd_waitfifoempty(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  struct ft80x_notify_s notify;
  struct timespec timeout;
  sigset_t set;
  uint16_t head;
  uint16_t tail;
  int ret;

  /* Block the signal so that it will pend if received asynchronously */

  (void)sigemptyset(&set);
  (void)sigaddset(&set, CONFIG_GRAPHICS_FT80X_CMDEMPTY_SIGNAL);

  ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
  if (ret < 0)
    {
      ret = -errno;
      ft80x_err("ERROR: pthread_sigmask for signal %d failed: %d\n",
                CONFIG_GRAPHICS_FT80X_CMDEMPTY_SIGNAL, ret);
      return ret;
    }

  notify.signo  = CONFIG_GRAPHICS_FT80X_CMDEMPTY_SIGNAL;
  notify.pid    = getpid();
  notify.event  = FT80X_NOTIFY_CMDEMPTY;
  notify.enable = false;

  for (; ; )
    {
      /* Check if the FIFO is already empty */

      ret = ft80x_getreg16(fd, FT80X_REG_CMD_WRITE, &head);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_getreg16 failed: %d\n", ret);
          break;
        }

      ret = ft80x_getreg16(fd, FT80X_REG_CMD_READ, &tail);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_getreg16 failed: %d\n", ret);
          break;
        }

      ft80x_info("head=%u tail=%u\n", head, tail);

      if (head == tail)
        {
          /* The FIFO is empty... return success */

          ret = OK;
          break;
        }

      /* Set up to receive a notification when the CMD FIFO becomes empty
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
          ft80x_err("ERROR: ioctl(FT80X_IOC_EVENTNOTIFY) failed: %d\n", errcode);
          break;
        }

      /* Now wait for the signal event (or a timeout) */

      timeout.tv_sec  = 0;
      timeout.tv_nsec = 400 * 1000 * 1000;

      ret = sigtimedwait(&set, NULL, &timeout);

      /* Make sure that the event notification is again disabled */

      notify.enable = false;
      (void)ioctl(fd, FT80X_IOC_EVENTNOTIFY,
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

  (void)pthread_sigmask(SIG_UNBLOCK, &set, NULL);
  return ret;
}
