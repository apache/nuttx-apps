/****************************************************************************
 * apps/include/graphics/ft80x.h
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

#ifndef __APPS_INCLUDE_GRAPHICS_FT80X_H
#define __APPS_INCLUDE_GRAPHICS_FT80X_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>

#ifdef CONFIG_GRAPHICS_FT80X

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Buffer size in units of words (truncating any unaligned bytes) */

#define FT80X_DL_BUFSIZE  (CONFIG_GRAPHICS_FT80X_BUFSIZE & ~3)
#define FT80X_DL_BUFWORDS (CONFIG_GRAPHICS_FT80X_BUFSIZE >> 2)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure defines the local display list buffer */

struct ft80x_dlbuffer_s
{
  uint16_t dlsize;   /* Total sizeof the display list written to hardware */
  uint16_t dloffset; /* The number display list bytes buffered locally */
  uint32_t dlbuffer[FT80X_DL_BUFWORDS];
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: ft80x_dl_start
 *
 * Description:
 *   Start a new display list.  This function will:
 *
 *   1) Set the total display size to zero
 *   2) Set the display list buffer offset to zero
 *   3) Reposition the VFS so that subsequent writes will be to the
 *      beginning of the hardware display list.
 *   4) Write the CMD_DLSTART command into the local display list buffer.
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

int ft80x_dl_start(int fd, FAR struct ft80x_dlbuffer_s *buffer);

/****************************************************************************
 * Name: ft80x_dl_end
 *
 * Description:
 *   Terminate the display list.  This function will:
 *
 *   1) Add the DISPLAY command to the local display list buffer to finish
 *      the last display
 *   2) Flush the local display buffer to hardware and set the display list
 *      buffer offset to zero.
 *   3) Swap to the newly created display list.
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

int ft80x_dl_end(int fd, FAR struct ft80x_dlbuffer_s *buffer);

/****************************************************************************
 * Name: ft80x_dl_data
 *
 * Description:
 *   Add data to the display list and increment the display list buffer
 *   offset.  If the data will not fit into the local display buffer, then
 *   the local display buffer will first be flushed to hardware in order to
 *   free up space.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *   data   - The data to be added to the display list
 *   datlen - The length of the data to be added to the display list.  If
 *            this is not an even multiple of 4 bytes, then the actual length
 *            will be padded with zero bytes to achieve alignment.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_data(int fd, FAR struct ft80x_dlbuffer_s *buffer,
                  FAR const void *data, size_t datlen);

/****************************************************************************
 * Name: ft80x_dl_string
 *
 * Description:
 *   Add the string along with its NUL terminator to the display list and
 *   increment the display list buffer offset.  If the length of the string
 *   with its NUL terminator is not an even multiple of 4 bytes, then the
 *   actual length will be padded with zero bytes to achieve alignment.
 *
 *   If the data will not fit into the local display buffer, then the local
 *   display buffer will first be flushed to hardware in order to free up
 *   space.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *   str    - The string to be added to the display list.  If NUL, then a
 *            NUL string will be added to the display list.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_string(int fd, FAR struct ft80x_dlbuffer_s *buffer,
                    FAR const char *str);

/****************************************************************************
 * Name: ft80x_dl_flush
 *
 * Description:
 *   Flush the current contents of the local local display list buffer to
 *   hardware and reset the local display list buffer offset to zero.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller with
 *            write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_flush(int fd, FAR struct ft80x_dlbuffer_s *buffer);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_GRAPHICS_FT80X */
#endif /* __APPS_INCLUDE_GRAPHICS_FT80X_H */
