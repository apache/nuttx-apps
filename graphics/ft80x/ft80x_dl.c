/****************************************************************************
 * apps/graphics/ft80x/ft80x_dl.c
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
 *   data   - Data being written.
 *   len    - Number of bytes being written
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_FT80X_DEBUG_INFO
static void ft80x_dl_dump(FAR struct ft80x_dlbuffer_s *buffer,
                          FAR const void *data, size_t len)
{
  size_t nwords;
  int max;
  int i;
  int j;

  printf("Writing display list:\n");
  printf("  buffer: dlsize=%lu dloffset=%lu coproc=%u\n",
         (unsigned long)buffer->dlsize, (unsigned long)buffer->dloffset,
         buffer->coproc);
  printf("  write:  data=%p length=%lu\n",
         data, (unsigned long)len);

  nwords = len >> 2;
  for (i = 0; i < nwords; i += 8)
    {
      printf("  %04x: ", i << 2);

      max = i + 4;
      if (max >= nwords)
        {
          max = nwords;
        }

      for (j = i; j < max; j++)
        {
          printf(" %08x", buffer->dlbuffer[j]);
        }

      putchar(' ');

      max = i + 8;
      if (max >= nwords)
        {
          max = nwords;
        }

      for (j = i + 4; j < max; j++)
        {
          printf(" %08x", buffer->dlbuffer[j]);
        }

      putchar('\n');
    }
}
#else
#  define ft80x_dl_dump(b,d,l)
#endif

/****************************************************************************
 * Name: ft80x_dl_append
 *
 * Description:
 *   Append the display list data to the appropriate memory region.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *   data   - A pointer to the start of the data to be written.
 *   len    - The number of bytes to be written.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static int ft80x_dl_append(int fd, FAR struct ft80x_dlbuffer_s *buffer,
                           FAR const void *data, size_t len)
{
  int ret;

  ft80x_dl_dump(buffer, data, len);
  if (buffer->coproc)
    {
      /* Append data to RAM CMD */

      ret = ft80x_ramcmd_append(fd, data, len);
    }
  else
    {
      /* Append data to RAM DL */

      ret = ft80x_ramdl_append(fd, data, len);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_dl_start
 *
 * Description:
 *   Start a new display list.  This function will:
 *
 *   1) Set the total display list size to zero
 *   2) Set the display list buffer offset to zero
 *   3) Reposition the VFS so that subsequent writes will be to the
 *      beginning of the hardware display list.
 *      (Only for DL memory commands)
 *   4) Write the CMD_DLSTART command into the local display list buffer
 *      (Only for co-processor commands)
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *   coproc - True: Use co-processor FIFO; false: Use DL memory.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_start(int fd, FAR struct ft80x_dlbuffer_s *buffer, bool coproc)
{
  struct ft80x_cmd_dlstart_s dlstart;
  int ret;

  ft80x_info("fd=%d buffer=%p\n", fd, buffer);
  DEBUGASSERT(fd >= 0 && buffer != NULL);

  /* 1) Set the total display list size to zero
   * 2) Set the display list buffer offset to zero
   */

  buffer->coproc   = coproc;
  buffer->dlsize   = 0;
  buffer->dloffset = 0;

  if (!coproc)
    {
      /* 3) Reposition the VFS so that subsequent writes will be to the
       *    beginning of the hardware display list.
       */

      ret = ft80x_ramdl_rewind(fd);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_ramdl_rewind failed: %d\n", ret);
        }
    }
  else
    {
      /* 4) Write the CMD_DLSTART command into the local display list
       *    buffer. (Only for co-processor commands)
       */

      dlstart.cmd = FT80X_CMD_DLSTART;
      ret = ft80x_dl_data(fd, buffer, &dlstart,
                          sizeof(struct ft80x_cmd_dlstart_s));
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
        }
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_dl_end
 *
 * Description:
 *   Terminate the display list.  This function will:
 *
 *   1) Add the DISPLAY command to the local display list buffer to finish
 *      the last display
 *   2) If using co-processor RAM CMD, add the CMD_SWAP to the DL command
 *      list
 *   3) Flush the local display buffer to hardware and set the display list
 *      buffer offset to zero.
 *   4) Swap to the newly created display list (DL memory case only).
 *   5) For the case of the co-processor RAM CMD, it will also wait for the
 *      FIFO to be emptied.
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
  struct
  {
    struct ft80x_dlcmd_s display;
    struct ft80x_cmd32_s swap;
  } s;

  size_t size;
  int ret;

  ft80x_info("fd=%d buffer=%p\n", fd, buffer);
  DEBUGASSERT(fd >= 0 && buffer != NULL);

  /* 1) Add the DISPLAY command to the local display list buffer to finish
   *    the last display
   */

  s.display.cmd = FT80X_DISPLAY();
  size          = sizeof(struct ft80x_dlcmd_s);

  /* 2) If using co-processor RAM CMD, add the CMD_SWAP to the DL command
   *    list
   */

  if (buffer->coproc)
    {
      s.swap.cmd = FT80X_CMD_SWAP;
      size       = sizeof(s);
    };

  ret = ft80x_dl_data(fd, buffer, &s, size);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* 3) Flush the local display buffer to hardware and set the display list
   *    buffer offset to zero.
   */

  ret = ft80x_dl_flush(fd, buffer, false);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_flush failed: %d\n", ret);
    }

  /* 4) Swap to the newly created display list (DL memory case only). */

  if (!buffer->coproc)
    {
      ret = ft80x_dl_swap(fd);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_swap failed: %d\n", ret);
          return ret;
        }
    }

  /* 5) For the case of the co-processor RAM CMD, it will also wait for the
   *    FIFO to be emptied.
   */

  if (buffer->coproc)
    {
      ret = ft80x_ramcmd_waitfifoempty(fd);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_ramcmd_waitfifoempty failed: %d\n", ret);
          return ret;
        }
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
          ft80x_info("Length padded to %u->%u\n", datlen, padlen);
        }

      /* Is there enough space in the local display list buffer to hold the
       * new data?
       */

      if ((size_t)buffer->dloffset + padlen > FT80X_DL_BUFSIZE)
        {
          /* No... flush the local buffer */

          ret = ft80x_dl_flush(fd, buffer, false);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_dl_flush failed: %d\n", ret);
              return ret;
            }
        }

      /* Special case:  The new data won't fit into our local display list
       * buffer.  Here we can assume the flush above occurred.  We can then
       * work around by writing directly, unbuffered from the caller's
       * buffer.
       */

      if (padlen > FT80X_DL_BUFSIZE)
        {
          size_t writelen;

          /* Write the aligned portion of the data directly to the FT80x
           * hardware display list.
           */

          writelen = datlen & ~3;
          ret = ft80x_dl_append(fd, buffer, data, writelen);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_dl_append failed: %d\n", ret);
              return ret;
            }

          buffer->dlsize += writelen;

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

      buffer->dlsize   += padlen;
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
      ft80x_info("Length padded to %u->%u\n", datlen, padlen);
    }

  /* Is there enough space in the local display list buffer to hold the new
   * string?
   */

  if ((size_t)buffer->dloffset + padlen > FT80X_DL_BUFSIZE)
    {
      /* No... flush the local buffer */

      ret = ft80x_dl_flush(fd, buffer, false);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_flush failed: %d\n", ret);
          return ret;
        }
    }

  /* Special case:  The new string won't fit into our local display list
   * buffer.  Here we can assume the flush above occurred.  We can then
   * work around by writing directly, unbuffered from the caller's
   * buffer.
   */

  if (padlen > FT80X_DL_BUFSIZE)
    {
      size_t writelen;

      /* Write the aligned portion of the string directly to the FT80x
       * hardware display list.
       */

      writelen = datlen & ~3;
      ret = ft80x_dl_append(fd, buffer, str, writelen);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_dl_append failed: %d\n", ret);
          return ret;
        }

      buffer->dlsize += writelen;

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

  buffer->dlsize   += padlen;
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
 *   wait   - True: wait until data has been consumed by the co-processor
 *            (only for co-processor destination); false:  Send to hardware
 *            and return immediately.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_flush(int fd, FAR struct ft80x_dlbuffer_s *buffer, bool wait)
{
  int ret;

  ft80x_info("fd=%d buffer=%p dloffset=%u\n", fd, buffer, buffer->dloffset);
  DEBUGASSERT(fd >= 0 && buffer != NULL);

  /* Write the content of the local display buffer to hardware. */

  ret = ft80x_dl_append(fd, buffer, buffer->dlbuffer, buffer->dloffset);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_append failed: %d\n", ret);
      return ret;
    }

  buffer->dloffset = 0;

  /* For the case of the co-processor RAM CMD, it will also wait for the
   * FIFO to be emptied if wait == true.
   */

  if (wait && buffer->coproc)
    {
      ret = ft80x_ramcmd_waitfifoempty(fd);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_ramcmd_waitfifoempty failed: %d\n", ret);
          return ret;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: ft80x_dl_create
 *
 * Description:
 *   For simple display lists, this function combines all functionality into
 *   a single combined.  This function does the following:
 *
 *   1) Calls ft80x_dl_dlstart() to initialize the display list.
 *   2) Calls ft80x_dl_data() to transfer the simple display list
 *   3) Calls ft80x_dl_end() to complete the display list
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *   data   - Pointer to a uint32_t array containing the simple display list
 *   nwords - The number of 32-bit words in the array.
 *   coproc - True: Use co-processor FIFO; false: Use DL memory.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_create(int fd, FAR struct ft80x_dlbuffer_s *buffer,
                    FAR const uint32_t *cmds, unsigned int nwords,
                    bool coproc)
{
  int ret;

  ft80x_info("fd=%d buffer=%p cmds=%p nwords=%u coproc=%u\n",
             fd, buffer, cmds, nwords, coproc);
  DEBUGASSERT(fd >= 0 && buffer != NULL && cmds != NULL && nwords > 0);

  /* Create the hardware display list */

  ret = ft80x_dl_start(fd, buffer, coproc);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  /* Copy the rectangle data into the display list */

  ret = ft80x_dl_data(fd, buffer, cmds, nwords << 2);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  /* And terminate the display list */

  ret = ft80x_dl_end(fd, buffer);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_end failed: %d\n", ret);
      return ret;
    }

  return OK;
}
