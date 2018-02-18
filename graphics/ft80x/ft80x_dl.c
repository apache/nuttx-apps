/****************************************************************************
 * apps/graphics/ft80x/ft80x_dl.c
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

#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <nuttx/lcd/ft80x.h>

#include "graphics/ft80x.h"
#include "ft80x.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_dl_dump
 *
 * Description:
 *   Dump the content of the display list to stdout as it is written from
 *   the local display buffer to hardware.
 *
 * Input Parameters:
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#ifdef GRAPHICS_FT80X_DEBUG_INFO
static void ft80x_dl_dump(FAR struct ft80x_dlbuffer_s *buffer)
{
  uint16_t nwords;
  int max;
  int i;
  int j;

  printf("Write display list:  dlsize=%lu dloffset=%lu\n",
         (unsigned long)buffer->dlsize, (unsigned long)buffer->dloffset);

  nwords = buffer->dloffset >> 2;
  for (i = 0; i < nwords; i += 8)
    {
      printf("  %04x: ", i << 2);

      max = i + 4;
      if (max >= nwords)
        {
          max = nwords - i;
        }

      for (j = i; j < max; j++)
        {
          printf(" %08x", buffer->dlbuffer[j]);
        }

      putchar(' ');

      max = i + 8;
      if (i + max >= nwords)
        {
          max = nwords - i;
        }

      for (j = i + 4; j < max; j++)
        {
          printf(" %08x", buffer->dlbuffer[j]);
        }

      putchar('\n');
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

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

int ft80x_dl_start(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  struct ft80x_cmd_dlstart_s dlstart;
  off_t pos;

  ft80x_info("fd=%d buffer=%p\n", fd, buffer);
  DEBUGASSERT(fd >= 0 && buffer != NULL);

  /* 1) Set the total display size to zero
   * 2) Set the display list buffer offset to zero
   */

  buffer->dlsize  = 0;
  buffer->dloffset = 0;

  /* 3) Reposition the VFS so that subsequent writes will be to the
   *    beginning of the hardware display list.
   */

  pos = lseek(fd, 0, SEEK_SET);
  if (pos < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: lseek failed: %d\n", errcode);
      return -errcode;
    }

  /* 4) Write the CMD_DLSTART command into the local display list buffer. */

  dlstart.cmd = FT80X_CMD_DLSTART;
  return ft80x_dl_data(fd, buffer, &dlstart,
                       sizeof(struct ft80x_cmd_dlstart_s));
}

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

int ft80x_dl_end(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  struct ft80x_dlcmd_s display;
  int ret;

  ft80x_info("fd=%d buffer=%p\n", fd, buffer);
  DEBUGASSERT(fd >= 0 && buffer != NULL);

  /* 1) Add the DISPLAY command to the local display list buffer to finish
   *    the last display
   */

  display.cmd = FT80X_DISPLAY();
  ret = ft80x_dl_data(fd, buffer, &display, sizeof(struct ft80x_dlcmd_s));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* 2) Flush the local display buffer to hardware and set the display list
   *    buffer offset to zero.
   */

  ret = ft80x_dl_flush(fd, buffer);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_flush failed: %d\n", ret);
    }

  /* 3) Swap to the newly created display list. */

  ret = ft80x_dl_swap(fd);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_swap failed: %d\n", ret);
      return ret;
    }

  return ret;
}

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
                  FAR const void *data, size_t datlen)
{
  FAR uint8_t *bufptr;
  size_t padlen;
  int ret;

  ft80x_info("fd=%d buffer=%p data=%p datlen=%u\n", fd, buffer, data, datlen);
  DEBUGASSERT(fd >= 0 && buffer != NULL && data != NULL && datlen > 0);

  if (datlen > 0)
    {
      /* This is the length of the data after alignment */

      padlen = (datlen + 3) & ~3;
      if (padlen != datlen)
        {
          ft80x_warn("WARNING: Length padded to %u->%u\n", datlen, padlen);
        }

      /* Is there enough space in the local display list buffer to hold the
       * new data?
       */

      if ((size_t)buffer->dloffset + padlen > FT80X_DL_BUFSIZE)
        {
          /* No... flush the local buffer */

          ret = ft80x_dl_flush(fd, buffer);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_dl_flush failed: %d\n", ret);
              return ret;
            }
        }

      /* Special case:  The new data won't fit into our local display list
       * buffer.
       */

      if (padlen > FT80X_DL_BUFSIZE)
        {
          size_t writelen;
          size_t nwritten;

          /* Write the aligned portion of the data directly to the FT80x
           * hardware display list.  NOTE:  We have inside knowledge that
           * the write will complete in a single operation so that no
           * piecewise writes will ever be necessary.
           */

          writelen = datlen & ~3;
          nwritten = write(fd, data, writelen);
          if (nwritten < 0)
            {
              int errcode = errno;
              ft80x_err("ERROR: write failed: %d\n", errcode);
              return -errcode;
            }

          DEBUGASSERT(nwritten == writelen);

          /* Is there any unaligned remainder?  If the original data length
           * was aligned, then we should have writelen == datlen == padlen.
           * If it the length was unaligned, then we should have writelen <
           * datlen < padlen
           */

          if (writelen < datlen)
            {
              /* Yes... we will handle the unaligned bytes below. */

              data    = (FAR void *)((uintptr_t)data + writelen);
              datlen -= writelen;
              padlen -= writelen;
            }
          else
            {
              /* No.. then we are finished */

              return OK;
            }
        }

     /* Copy the data into the local display list buffer */

     bufptr            = (FAR uint8_t *)buffer->dlbuffer;
     bufptr           += buffer->dloffset;
     memcpy(bufptr, data, datlen);

     bufptr           += datlen;
     buffer->dloffset += datlen;

     /* Then append zero bytes as necessary to achieve alignment */

     while (datlen < padlen)
       {
         *bufptr++     = 0;
         buffer->dloffset++;
         datlen++;
       }
    }

  return OK;
}

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
                    FAR const char *str)
{
  FAR uint8_t *bufptr;
  int datlen;
  int padlen;
  int ret;

  ft80x_info("fd=%d buffer=%p str=%p\n", fd, buffer, str);
  DEBUGASSERT(fd >= 0 && buffer != NULL);

  /* Get the length the the string (excluding the NUL terminator) */

  if (str == NULL)
    {
      str    = "";
      datlen = 0;
    }
  else
    {
      datlen = strlen(str);
    }

  /* This is the length of the string after the NUL terminator is added and
   * the length is aligned to 32-bit boundary alignment.
   */

  padlen = (datlen + 4) & ~3;
  if (padlen != (datlen + 1))
    {
      ft80x_warn("WARNING: Length padded to %u->%u\n", datlen, padlen);
    }

  /* Is there enough space in the local display list buffer to hold the new
   * string?
   */

  if ((size_t)buffer->dloffset + padlen > FT80X_DL_BUFSIZE)
    {
      /* No... flush the local buffer */

      ret = ft80x_dl_flush(fd, buffer);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_flush failed: %d\n", ret);
          return ret;
        }
    }

  /* Special case:  The new string won't fit into our local display list
   * buffer.
   */

  if (padlen > FT80X_DL_BUFSIZE)
    {
      size_t writelen;
      size_t nwritten;

      /* Write the aligned portion of the string directly to the FT80x
       * hardware display list.  NOTE:  We have inside knowledge that the
       * write will complete in a single operation so that no piecewise
       * writes will ever be necessary.
       */

      writelen = datlen & ~3;
      nwritten = write(fd, str, writelen);
      if (nwritten < 0)
        {
          int errcode = errno;
          ft80x_err("ERROR: write failed: %d\n", errcode);
          return -errcode;
        }

      DEBUGASSERT(nwritten == writelen);

      /* There should always be an unaligned remainder  If the original
       * string length was aligned, then we should have writelen == datlen <
       * padlen. If it the length was unaligned, then we should have
       * writelen < datlen < padlen
       */

      DEBUGASSERT(writelen < padlen);

      /* We will handle the unaligned remainder below. */

      str     = &str[writelen];
      datlen -= writelen;
      padlen -= writelen;
    }

  /* Copy the data into the local display list buffer */

  bufptr            = (FAR uint8_t *)buffer->dlbuffer;
  bufptr           += buffer->dloffset;
  strcpy((FAR char *)bufptr, str);

  /* NOTE: that strcpy will copy the NUL terminator too */

  datlen++;

  /* Update pointers/offsets */

  bufptr           += datlen;
  buffer->dloffset += datlen;

  /* Then append zero bytes as necessary to achieve alignment */

  while (datlen < padlen)
    {
      *bufptr++     = 0;
      buffer->dloffset++;
      datlen++;
    }

  return OK;
}

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

int ft80x_dl_flush(int fd, FAR struct ft80x_dlbuffer_s *buffer)
{
  size_t nwritten;

  ft80x_info("fd=%d buffer=%p\n", fd, buffer);
  DEBUGASSERT(fd >= 0 && buffer != NULL);

  /* Write the content of the local display buffer to hardware.  NOTE:  We
   * have inside knowledge that the write will complete in a single
   * operation so that no piecewise writes will ever be necessary.
   */

  nwritten = write(fd, buffer->dlbuffer, buffer->dloffset);
  if (nwritten < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: write failed: %d\n", errcode);
      return -errcode;
    }

  DEBUGASSERT(nwritten == buffer->dloffset);
  buffer->dloffset = 0;
  return OK;
}
