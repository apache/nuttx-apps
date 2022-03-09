/****************************************************************************
 * apps/graphics/ft80x/ft80x_ramcmd.c
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
#include <pthread.h>
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
 * Name: ft80x_ramcmd_append
 *
 * Description:
 *   Append new display list data to RAM CMD
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   data   - A pointer to the start of the data to be append to RAM CMD.
 *   len    - The number of bytes to be appended.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_ramcmd_append(int fd, FAR const void *data, size_t len)
{
  struct ft80x_relmem_s wrdesc;
  FAR const uint8_t *src;
  ssize_t remaining;
  size_t wrsize;
  uint16_t offset;
  uint16_t maxsize;
  int ret;

  DEBUGASSERT(data != NULL && ((uintptr_t)data & 3) == 0 &&
              len > 0 && (len & 3) == 0);

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
      ret     = ft80x_ramcmd_freespace(fd, &offset, &maxsize);
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
#if 1
          /* Yes.. wait for the FIFO to empty, then try again.
           *
           * REVISIT:  Would it not be sufficient to wait only until the
           * FIFO is not full?
           */

          ft80x_warn("WARNING: FIFO is full: %d\n", ret);
          ret = ft80x_ramcmd_waitfifoempty(fd);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_ramcmd_waitfifoempty() failed: %d\n",
                        ret);
              return ret;
            }

          continue;
#else
          ft80x_err("ERROR: FIFO is full: %d\n", ret);
          return -ENOSPC;
#endif
        }

      /* Limit the write size to the size of the available FIFO memory */

      if (wrsize > (size_t)maxsize)
        {
          wrsize = (size_t)maxsize;
        }

      /* Perform the transfer */

      wrdesc.offset = offset;
      wrdesc.nbytes = wrsize;
      wrdesc.value  = (FAR void *)src;  /* Discards 'const' qualifier */

      ret = ioctl(fd, FT80X_IOC_PUTRAMCMD,
                  (unsigned long)((uintptr_t)&wrdesc));
      if (ret < 0)
        {
          int errcode = errno;
          ft80x_err("ERROR: ioctl() FT80X_IOC_PUTRAMCMD failed: %d\n",
                    errcode);
          return -errcode;
        }

      /* Update the command FIFO */

      ret = ft80x_putreg16(fd, FT80X_REG_CMD_WRITE, offset + wrsize);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_putreg16() failed: %d\n", ret);
          return ret;
        }

      /* Wait for the FIFO to empty */

      if (remaining > wrsize)
        {
          ret = ft80x_ramcmd_waitfifoempty(fd);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_ramcmd_waitfifoempty() failed: %d\n",
                        ret);
              return ret;
            }
        }

      /* Set up for the next time through the loop. */

      remaining -= wrsize;
      src       += wrsize;
    }
  while (remaining > 0);

  return OK;
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
 *   offset - Pointer to location to return the write offset to use if the
 *            FIFO is not full.
 *   avail  - Pointer to location to return the FIFO free space
 *
 * Returned Value:
 *   The (positive) number of free bytes in RAM CMD on success.  A negated
 *   errno value is returned on any failure.
 *
 ****************************************************************************/

uint16_t ft80x_ramcmd_freespace(int fd, FAR uint16_t *offset,
                                FAR uint16_t *avail)
{
  uint32_t regs[2];
  int ret;

  /* Read both the FT80X_REG_CMD_WRITE and FT80X_REG_CMD_READ registers. */

  ret = ft80x_getregs(fd, FT80X_REG_CMD_READ, 2, regs);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getregs failed: %d\n", ret);
      return ret;
    }

  /* Return the free space in the FIFO.  NOTE that 4 bytes are not available */

  *offset = regs[1] & FT80X_CMDFIFO_MASK;
  *avail  = (FT80X_CMDFIFO_SIZE - 4) - ((regs[1] - regs[0]) & FT80X_CMDFIFO_MASK);
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
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_ramcmd_waitfifoempty(int fd)
{
  struct ft80x_notify_s notify;
  struct timespec timeout;
  sigset_t set;
  uint32_t regs[2];
  uint16_t head;
  uint16_t tail;
  int ret;

  /* Block the signal so that it will pend if received asynchronously */

  sigemptyset(&set);
  sigaddset(&set, CONFIG_GRAPHICS_FT80X_CMDEMPTY_SIGNAL);

  ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
  if (ret < 0)
    {
      ret = -errno;
      ft80x_err("ERROR: pthread_sigmask for signal %d failed: %d\n",
                CONFIG_GRAPHICS_FT80X_CMDEMPTY_SIGNAL, ret);
      return ret;
    }

  notify.event.sigev_notify = SIGEV_SIGNAL;
  notify.event.sigev_signo  = CONFIG_GRAPHICS_FT80X_CMDEMPTY_SIGNAL;

  notify.pid    = getpid();
  notify.id     = FT80X_NOTIFY_CMDEMPTY;
  notify.enable = false;

  for (; ; )
    {
      /* Read both the FT80X_REG_CMD_WRITE and FT80X_REG_CMD_READ registers. */

      ret = ft80x_getregs(fd, FT80X_REG_CMD_READ, 2, regs);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_getregs failed: %d\n", ret);
          return ret;
        }

      /* Check if the FIFO is already empty */

      head = regs[1] & FT80X_CMDFIFO_MASK;
      tail = regs[0] & FT80X_CMDFIFO_MASK;

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
  return ret;
}
