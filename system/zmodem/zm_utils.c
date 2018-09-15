/****************************************************************************
 * system/zmodem/zm_utils.c
 *
 *   Copyright (C) 2013, 2018 Gregory Nutt. All rights reserved.
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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <assert.h>
#include <errno.h>
#include <crc32.h>

#include "zm.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

const uint8_t g_zeroes[4] = { 0, 0, 0, 0 };

/****************************************************************************
 * Public Function Protypes
 ****************************************************************************/

/****************************************************************************
 * Name: zm_bytobe32
 *
 * Description:
 *   Convert a sequence of four bytes into a 32-bit value.  The byte
 *   sequence is assumed to be big-endian.
 *
 ****************************************************************************/

uint32_t zm_bytobe32(FAR const uint8_t *val8)
{
  return
    (uint32_t)val8[3] << 24 |
    (uint32_t)val8[2] << 16 |
    (uint32_t)val8[1] <<  8 |
    (uint32_t)val8[0];
}

/****************************************************************************
 * Name: zm_be32toby
 *
 * Description:
 *   Convert a 32-bit value in a sequence of four bytes in big-endian byte
 *   order.
 *
 ****************************************************************************/

void zm_be32toby(uint32_t val32, FAR uint8_t *val8)
{
  val8[0] = (uint8_t)( val32        & 0xff);
  val8[1] = (uint8_t)((val32 >> 8)  & 0xff);
  val8[2] = (uint8_t)((val32 >> 16) & 0xff);
  val8[3] = (uint8_t)((val32 >> 24) & 0xff);
}

/****************************************************************************
 * Name: zm_encnibble
 *
 * Description:
 *   Encode an 4-bit binary value to a single hex "digit".
 *
 ****************************************************************************/

char zm_encnibble(uint8_t nibble)
{
  if (nibble < 10)
    {
      return nibble + '0';
    }
  else
    {
      return nibble + 'a' - 10;
    }
}

/****************************************************************************
 * Name: zm_encnibble
 *
 * Description:
 *   Decode an 4-bit binary value from a single hex "digit".
 *
 ****************************************************************************/

uint8_t zm_decnibble(char hex)
{
  if (hex <= '9')
    {
      return hex - '0';
    }
  else if (hex <= 'F')
    {
      return hex - 'A' + 10;
    }
  else
    {
      return hex - 'a' + 10;
    }
}

/****************************************************************************
 * Name: zm_puthex8
 *
 * Description:
 *   Convert an 8-bit binary value to 2 hex "digits".
 *
 ****************************************************************************/

FAR uint8_t *zm_puthex8(FAR uint8_t *ptr, uint8_t ch)
{
  *ptr++ = zm_encnibble((ch >> 4) & 0xf);
  *ptr++ = zm_encnibble(ch & 0xf);
  return ptr;
}

/****************************************************************************
 * Name: zm_read
 *
 * Description:
 *   Read a buffer of data from a read-able stream.
 *
 ****************************************************************************/

ssize_t zm_read(int fd, FAR uint8_t *buffer, size_t buflen)
{
  ssize_t nread;

  /* Read reading as necessary until the requested buffer data is successfully
   * read or until an end of file indication or irrecoverable error is
   * encountered.
   *
   * This loop will only execute if the read is interrupted by a signal.
   */

  nread = 0;
  do
    {
      /* Get the next gulp of data from the file.  On success, read will return
       * (1) nread > 0 and nread <= buflen, (2) nread == 0 on end of file, or
       * (3) nread < 0 on a read error.
       */

      nread = read(fd, buffer, buflen);

      /* Did some error occur? */

      if (nread < 0)
        {
          int errorcode = errno;

          /* EINTR is not an error... it simply means that this read was
           * interrupted by an signal before it obtained in data.
           */

          if (errorcode != EINTR)
            {
              /* But anything else is bad and we will return the failure
               * in those cases.
               */

              zmdbg("ERROR: read failed: %d\n", errorcode);
              DEBUGASSERT(errorcode != 0);
              return -errorcode;
            }
        }
    }
  while (nread < 0);

  return (int)nread;
}

/****************************************************************************
 * Name: zm_getc
 *
 * Description:
 *   Read a one byte of data from a read-able stream.
 *
 ****************************************************************************/

int zm_getc(int fd)
{
  ssize_t nread;
  uint8_t ch;

  nread = zm_read(fd, &ch, 1);
  if (nread <= 0)
    {
      return EOF;
    }

  return ch;
}

/****************************************************************************
 * Name: zm_write
 *
 * Description:
 *   Write a buffer of data to a write-able stream.
 *
 ****************************************************************************/

ssize_t zm_write(int fd, FAR const uint8_t *buffer, size_t buflen)
{
  ssize_t nwritten;
  size_t wrsize;
  size_t remaining;

  /* Read reading as necessary until the requested buffer is filled or until
   * an end of file indication or irrecoverable error is encountered.
   */

  for (remaining = buflen; remaining > 0; )
    {
#if CONFIG_SYSTEM_ZMODEM_WRITESIZE > 0
       if (remaining > CONFIG_SYSTEM_ZMODEM_WRITESIZE)
         {
           wrsize = CONFIG_SYSTEM_ZMODEM_WRITESIZE;
         }
       else
#endif
         {
           wrsize = remaining;
         }

      /* Get the next gulp of data from the file */

      nwritten = write(fd, buffer, wrsize);
      if (nwritten < 0)
        {
          int errorcode = errno;

          /* EINTR is not an error... it simply means that this read was
           * interrupted by an signal before it obtained in data.
           */

          if (errorcode != EINTR)
            {
              zmdbg("ERROR: write failed: %d\n", errorcode);
              DEBUGASSERT(errorcode != 0);
              return -errorcode;
            }
        }
      else
        {
          /* Updates counts and pointers for the next read */

          buffer    += nwritten;
          remaining -= nwritten;
        }
    }

  return (int)(buflen - remaining);
}

/****************************************************************************
 * Name: zm_remwrite
 *
 * Description:
 *   Write a buffer of data to the remote peer.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_ZMODEM_DUMPBUFFER
ssize_t zm_remwrite(int fd, FAR const uint8_t *buffer, size_t buflen)
{
  zm_dumpbuffer("Sending", buffer, buflen);
  return zm_write(fd, buffer, buflen);
}
#endif

/****************************************************************************
 * Name: zm_putc
 *
 * Description:
 *   Write a one byte of data to a write-able stream.
 *
 ****************************************************************************/

#if 0 /* Not used */
int zm_putc(int fd, uint8_t ch)
{
  ssize_t nwritten;

  nwritten = zm_write(fd, &ch, 1);
  if (nwritten <= 0)
    {
      return EOF
    }

  return ch;
}
#endif

/****************************************************************************
 * Name: zm_writefile
 *
 * Description:
 *   Write a buffer of data to file, performing newline conversions as
 *   necessary.
 *
 * NOTE:  Not re-entrant.  CR-LF sequences that span buffer boundaries are
 * not guaranteed to to be handled correctly.
 *
 ****************************************************************************/

int zm_writefile(int fd, FAR const uint8_t *buffer, size_t buflen, bool zcnl)
{
  int ret;

  /* If zcnl set, convert newlines to Unix convention */

  if (zcnl)
    {
      static bool newline = false;
      FAR const uint8_t *start;
      uint8_t ch;
      int nbytes;

      start  = buffer;
      nbytes = 0;
      ret    = OK;

      /* Loop for each character in the buffer */

      for (; buflen > 0 && ret == OK; buflen--)
        {
          /* Get the next character in the buffer */

          ch = *buffer++;

          /* Convert CR-LF, LF-CR, CR, and LF to LF */

          if (ch == '\n' || ch == '\r')
            {
              if (nbytes > 0)
                {
                  ret     = zm_write(fd, start, nbytes);
                  start   = buffer;
                  nbytes  = 0;
                }

              if (ret == OK)
                {
                  /* Skip one char of \r\n? */

                  if (newline)
                    {
                      /* Yes.. But don't skip if there is another */

                      newline = false;
                    }
                  else
                    {
                      /* Write one newline and skip the follow \r or \n */

                      ret     = zm_write(fd, (FAR uint8_t *)"\n", 1);
                      newline = true;
                    }
               }
            }
          else
            {
              /* Increment the number of bytes we need to write beginning at
               * start.  We want to write as many contiguous bytes as possible
               * for performance reasons.
               */

              nbytes++;
              newline = false;
            }
        }

      /* Write any trailing data that does not end with a newline */

      if (ret == OK && nbytes > 0)
        {
          ret = zm_write(fd, start, nbytes);
        }
    }
  else
    {
      /* We are not modifying newlines, let zm_write() do the whole job */

      ret = zm_write(fd, buffer, buflen);
    }

  return ret;
}

/****************************************************************************
 * Name: zm_filecrc
 *
 * Description:
 *   Perform CRC32 calculation on a file.
 *
 * Assumptions:
 *   The allocated I/O buffer is available to buffer file data.
 *
 ****************************************************************************/

uint32_t zm_filecrc(FAR struct zm_state_s *pzm, FAR const char *filename)
{
  uint32_t crc;
  ssize_t nread;
  int fd;

  /* Open the file for reading */

  fd = open(filename, O_RDONLY);
  if (fd < 0)
    {
      /* This should not happen */

      zmdbg("ERROR: Failed to open %s: %d\n", filename, errno);
      return 0;
    }

  /* Calculate the file CRC */

  crc = 0xffffffff;
  while ((nread = zm_read(fd, pzm->scratch, CONFIG_SYSTEM_ZMODEM_SNDBUFSIZE)) > 0)
    {
      crc = crc32part(pzm->scratch, nread, crc);
    }

  /* Close the file and return the CRC */

  close(fd);
  return ~crc;
}

/****************************************************************************
 * Name: zm_flowc
 *
 * Description:
 *   Enable hardware Rx/Tx flow control.
 *
 *   REVISIT:  Consider returning the original termios settings so that they
 *   could be restored with rx/sz exits.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_ZMODEM_FLOWC
void zm_flowc(int fd)
{
  struct termios term;

  /* Get the termios */

  tcgetattr(fd, &term);

#ifdef CONFIG_SYSTEM_ZMODEM_IFLOW
  /* Set input flow control */

#ifdef CRTS_IFLOW
  term.c_cflag |= CRTS_IFLOW;
#else
  term.c_cflag |= CRTSCTS;
#endif
#else /* CONFIG_SYSTEM_ZMODEM_IFLOW */
  /* Clear input flow control */

#ifdef CRTS_IFLOW
  term.c_cflag &= ~CRTS_IFLOW;
#else
  term.c_cflag &= ~CRTSCTS;
#endif
#endif /* CONFIG_SYSTEM_ZMODEM_IFLOW */

#ifdef CONFIG_SYSTEM_ZMODEM_OFLOW
  /* Set output flow control */

#ifdef CCTS_OFLOW
  term.c_cflag |= CCTS_OFLOW;
#else
  term.c_cflag |= CRTSCTS;
#endif
#else /* CONFIG_SYSTEM_ZMODEM_OFLOW */
  /* Clear output flow control */

#ifdef CCTS_OFLOW
  term.c_cflag &= ~CCTS_OFLOW;
#else
  term.c_cflag &= ~CRTSCTS;
#endif
#endif /* CONFIG_SYSTEM_ZMODEM_OFLOW */

  /* Save the modified termios */

  tcsetattr(fd, TCSANOW, &term);
}
#endif
