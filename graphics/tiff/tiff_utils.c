/****************************************************************************
 * apps/graphics/tiff/tiff_utils.c
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

#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include "graphics/tiff.h"

#include "tiff_internal.h"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tiff_put/get16/32
 *
 * Description:
 *   Put and get 16 and 32 values in the correct byte order at the specified
 *   position.
 *
 * Input Parameters:
 *   dest - The location to store the multi-byte data (put only)
 *   src - The location to get the multi-byte data (get only)
 *   value - The value to be stored (put only)
 *
 * Returned Value:
 *   None (put)
 *   The extracted value (get)
 *
 ****************************************************************************/

void tiff_put16(FAR uint8_t *dest, uint16_t value)
{
#ifdef CONFIG_ENDIAN_BIG
  *dest++ = (uint8_t)(value >> 8);
  *dest   = (uint8_t)(value & 0xff);
#else
  *dest++ = (uint8_t)(value & 0xff);
  *dest   = (uint8_t)(value >> 8);
#endif
}

void tiff_put32(FAR uint8_t *dest, uint32_t value)
{
#ifdef CONFIG_ENDIAN_BIG
  tiff_put16(dest,   (uint16_t)(value >> 16));
  tiff_put16(dest+2, (uint16_t)(value & 0xffff));
#else
  tiff_put16(dest,   (uint16_t)(value & 0xffff));
  tiff_put16(dest+2, (uint16_t)(value >> 16));
#endif
}

uint16_t tiff_get16(FAR uint8_t *src)
{
#ifdef CONFIG_ENDIAN_BIG
  return (uint16_t)src[0] << 8 | (uint16_t)src[1];
#else
  return (uint16_t)src[1] << 8 | (uint16_t)src[0];
#endif
}

uint32_t tiff_get32(FAR uint8_t *src)
{
#ifdef CONFIG_ENDIAN_BIG
  return (uint32_t)tiff_get16(src) << 16 | (uint32_t)tiff_get16(src+2);
#else
  return (uint32_t)tiff_get16(src+2) << 16 | (uint32_t)tiff_get16(src);
#endif

}

/****************************************************************************
 * Name: tiff_read
 *
 * Description:
 *   Read TIFF data from the specified file
 *
 * Input Parameters:
 *   fd - Open file descriptor to read from
 *   buffer - Read-only buffer containing the data to be written
 *   count - The number of bytes to write
 *
 * Returned Value:
 *   On success, then number of bytes read; Zero is returned on EOF.
 *   Otherwise, a negated errno value on failure.
 *
 ****************************************************************************/

ssize_t tiff_read(int fd, FAR void *buffer, size_t count)
{
  size_t  ntotal;
  ssize_t nbytes;
  int errval;

  /* This loop retries the write until either: (1) it completes successfully,
   * or (2) until an irrecoverble error occurs.
   */

  for (ntotal = 0; ntotal < count; )
    {
      /* Do the read.  The number of bytes left to read is the total
       * requested size (count) minus the amount that we have already read
       * (ntotal).
       */

      nbytes = read(fd, buffer, count-ntotal);

      /* Check for an error */

      if (nbytes < 0)
        {
          /* EINTR is not an error.. this just means that the write was
           * interrupted by a signal.
           */

          errval = errno;
          if (errval != EINTR)
            {
              /* Other errors are bad news and we will break out with an error */

              return -errval;
            }
        }

      /* Zero is a special case and means that the end of file was encountered. */

      else if (nbytes == 0)
        {
          break;
        }

      /* What if read returns some number of bytes other than the requested number?
       * This probably means that the end-of-file will be encountered the next time
       * that we call read().
       */

      else
        {
          buffer += nbytes;
          ntotal += nbytes;
        }
    }

  return ntotal;
}

/****************************************************************************
 * Name: tiff_write
 *
 * Description:
 *   Write TIFF data to the specified file
 *
 * Input Parameters:
 *   fd - Open file descriptor to write to
 *   buffer - Read-only buffer containing the data to be written
 *   count - The number of bytes to write
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int tiff_write(int fd, FAR const void *buffer, size_t count)
{
  ssize_t nbytes;
  int errval;

  /* This loop retries the write until either: (1) it completes successfully,
   * or (2) until an irrecoverble error occurs.
   */

  while (count > 0)
    {
      /* Do the write */

      nbytes = write(fd, buffer, count);

      /* Check for an error */

      if (nbytes < 0)
        {
          /* EINTR is not an error.. this just means that the write was
           * interrupted by a signal.
           */

          errval = errno;
          if (errval != EINTR)
            {
              /* Other errors are bad news and we will break out with an error */

              return -errval;
            }
        }

      /* What if write returns some number of bytes other than the requested number? */

      else
        {
          DEBUGASSERT(nbytes == count);
          buffer += nbytes;
          count  -= nbytes;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: tiff_putint16
 *
 * Description:
 *   Write two bytes to the outfile.
 *
 * Input Parameters:
 *   fd - File descriptor to be used.
 *   value - The 2-byte, uint16_t value to write
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int tiff_putint16(int fd, uint16_t value)
{
  uint8_t bytes[2];

  /* Write the two bytes to the output file */

  tiff_put16(bytes, value);
  return tiff_write(fd, bytes, 2);
}

/****************************************************************************
 * Name: tiff_putint32
 *
 * Description:
 *   Write four bytes to the outfile.
 *
 * Input Parameters:
 *   fd - File descriptor to be used.
 *   value - The 4-byte, uint32_t value to write
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int tiff_putint32(int fd, uint32_t value)
{
  uint8_t bytes[4];

  /* Write the four bytes to the output file */

  tiff_put32(bytes, value);
  return tiff_write(fd, bytes, 4);
}

/****************************************************************************
 * Name: tiff_putstring
 *
 * Description:
 *  Write a string of fixed length to the outfile.
 *
 * Input Parameters:
 *   fd - File descriptor to be used.
 *   string - A pointer to the memory containing the string
 *   len - The length of the string (including the NUL terminator)
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int tiff_putstring(int fd, FAR const char *string, int len)
{
#ifdef CONFIG_DEBUG_GRAPHICS
  int actual = strlen(string);

  DEBUGASSERT(len = actual+1);
#endif
  return tiff_write(fd, string, len);
}

/****************************************************************************
 * Name: tiff_wordalign
 *
 * Description:
 *  Pad a file with zeros as necessary to achieve word alignament.
 *
 * Input Parameters:
 *   fd - File descriptor to be used.
 *   size - The current size of the file
 *
 * Returned Value:
 *   The new size of the file on success.  A negated errno value on failure.
 *
 ****************************************************************************/

ssize_t tiff_wordalign(int fd, size_t size)
{
  unsigned int remainder;
  int ret;

  remainder = size & 3;
  if (remainder > 0)
    {
      unsigned int nbytes = 4 - remainder;
      uint32_t value      = 0;

      ret = tiff_write(fd, &value, nbytes);
      if (ret < 0)
        {
          return (ssize_t)ret;
        }
      size += nbytes;
    }
  return size;
}
