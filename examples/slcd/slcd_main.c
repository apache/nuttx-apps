/****************************************************************************
 * examples/clcd/slcd_main.c
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/lcd/slcd_ioctl.h>
#include <nuttx/lcd/slcd_codec.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_SLCD_DEVNAME
#  define CONFIG_EXAMPLES_SLCD_DEVNAME "/dev/slcd"
#endif

/****************************************************************************
 * Private Typs
 ****************************************************************************/
/* All state information for this test is kept within the following structure
 * in order create a namespace and to minimize the possibility of name
 * collisions.
 *
 * NOTE: stream must be the first element of struct slcd_test_s to support
 * casting between these two types.
 */

struct slcd_test_s
{
  struct lib_outstream_s   stream;      /* Stream to use for all output */
  struct slcd_attributes_s attr;        /* Size of the SLCD (rows x columns) */
  int                      fd;          /* File descriptor or the open SLCD device */
  bool                     initialized; /* TRUE:  Initialized */
  uint8_t                  currow;      /* Next row to display */

  /* The I/O buffer */

  uint8_t buffer[CONFIG_EXAMPLES_SLCD_BUFSIZE];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct slcd_test_s g_slcdtest;
static const char g_slcdhello[] = "Hello";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: slcd_dumpbuffer
 *
 * Description:
 *  Do a pretty buffer dump
 *
 ****************************************************************************/

void slcd_dumpbuffer(FAR const char *msg, FAR const uint8_t *buffer, unsigned int buflen)
{
  int i;
  int j;
  int k;

  printf("%s:\n", msg);
  for (i = 0; i < buflen; i += 32)
    {
      printf("%04x: ", i);
      for (j = 0; j < 32; j++)
        {
          k = i + j;

          if (j == 16)
            {
              printf(" ");
            }

          if (k < buflen)
            {
              printf("%02x", buffer[k]);
            }
          else
            {
              printf("  ");
            }
        }

      printf(" ");
      for (j = 0; j < 32; j++)
        {
         k = i + j;

          if (j == 16)
            {
              printf(" ");
            }

          if (k < buflen)
            {
              if (buffer[k] >= 0x20 && buffer[k] < 0x7f)
                {
                  printf("%c", buffer[k]);
                }
              else
                {
                  printf(".");
                }
            }
        }

      printf("\n");
   }
}

/****************************************************************************
 * Name: slcd_flush
 ****************************************************************************/

static int slcd_flush(FAR struct lib_outstream_s *stream)
{
  FAR struct slcd_test_s *priv = (FAR struct slcd_test_s *)stream;
  FAR const uint8_t *buffer;
  ssize_t nwritten;
  ssize_t remaining;

  /* Loop until all bytes were written (handling both partial and interrupted
   * writes).
   */

  remaining = stream->nput;
  buffer    = priv->buffer;

  slcd_dumpbuffer("WRITING", buffer, remaining);

  while (remaining > 0)
    {
      nwritten = write(priv->fd, buffer, remaining);
      if (nwritten < 0)
        {
          int errcode = errno;
          printf("write failed: %d\n", errcode);

          if (errcode != EINTR)
            {
              exit(EXIT_FAILURE);
            }
        }
      else
        {
          remaining -= nwritten;
          buffer    += nwritten;
        }
    }

  /* Reset the stream */

  stream->nput = 0;
  return OK;
}

/****************************************************************************
 * Name: slcd_putc
 ****************************************************************************/

static void slcd_putc(FAR struct lib_outstream_s *stream, int ch)
{
  FAR struct slcd_test_s *priv = (FAR struct slcd_test_s *)stream;

  /* Write the character to the buffer */

  priv->buffer[stream->nput] = ch;
  stream->nput++;
  priv->buffer[stream->nput] = '\0';

  /* If the buffer is full, flush it */

  if (stream->nput >= CONFIG_EXAMPLES_SLCD_BUFSIZE)
    {
      (void)slcd_flush(stream);
    }
}

/****************************************************************************
 * Name: slcd_puts
 ****************************************************************************/

static void slcd_puts(FAR struct lib_outstream_s *outstream,
                      FAR const char *str)
{
  for (; *str; str++)
    {
      slcd_put((int)*str, outstream);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * slcd_main
 ****************************************************************************/

int slcd_main(int argc, char *argv[])
{
  FAR struct slcd_test_s *priv = &g_slcdtest;
  FAR const char *str = g_slcdhello;
  int fd;
  int ret;

  /* Parse the command line.  For now, only a single optional string argument
   * is supported.
   */

#if defined(CONFIG_NSH_BUILTIN_APPS)
  if (argc > 1)
    {
      str = argv[1];
    }
#endif

  /* Open the SLCD device */

  printf("Opening %s for read/write access\n", CONFIG_EXAMPLES_SLCD_DEVNAME);

  fd = open(CONFIG_EXAMPLES_SLCD_DEVNAME, O_RDWR);
  if (priv->fd < 0)
    {
      printf("Failed to open %s: %d\n", CONFIG_EXAMPLES_SLCD_DEVNAME, errno);
      goto errout;
    }

  /* Are we already initialized? */

  if (!priv->initialized)
    {
      unsigned long brightness;

      /* Initialize the output stream */

      memset(priv, 0, sizeof(struct slcd_test_s));
      priv->stream.put   = slcd_putc;
#ifdef CONFIG_STDIO_LINEBUFFER
      priv->stream.flush = slcd_flush;
#endif

      /* Get the attributes of the SCLD device */

      ret = ioctl(fd, SLCDIOC_GETATTRIBUTES, (unsigned long)&priv->attr);
      if (ret < 0)
        {
          printf("ioctl(SLCDIOC_GETATTRIBUTES) failed: %d\n", errno);
          goto errout_with_fd;
        }

      printf("Attributes:\n");
      printf("  rows: %d columns: %d nbars: %d\n",
             priv->attr.nrows, priv->attr.ncolumns, priv->attr.nbars);
      printf("  max contrast: %d max brightness: %d\n",
             priv->attr.maxcontrast, priv->attr.maxbrightness);

      /* Home the cursor and clear the display */

      printf("Clear screen\n");
      slcd_encode(SLCDCODE_CLEAR, 0, &priv->stream);
      slcd_flush(&priv->stream);

      /* Set the brightness to the mid value */

      brightness = ((unsigned long)priv->attr.maxbrightness + 1) >> 1;
      printf("Set brightness to %ld\n", brightness);

      ret = ioctl(fd, SLCDIOC_SETBRIGHTNESS, brightness);
      if (ret < 0)
        {
          /* Report the ioctl failure, but do not error out.  Some SLCDs
           * do not support brightness settings.
           */

          printf("ioctl(SLCDIOC_GETATTRIBUTES) failed: %d\n", errno);
        }

      priv->initialized = true;
    }

  /* Save the file descriptor in a place where slcd_flush can find it */

  priv->fd = fd;

  /* Set the cursor to the beginning of the current row by homing the cursor
   * then going down as necessary, and erase to the end of the line.
   */

  slcd_encode(SLCDCODE_HOME, 0, &priv->stream);
  slcd_encode(SLCDCODE_DOWN, priv->currow, &priv->stream);
  slcd_encode(SLCDCODE_ERASEEOL, 0, &priv->stream);

  /* Increment to the next row, wrapping back to first if necessary. */

  if (++priv->currow >= priv->attr.nrows)
    {
      priv->currow = 0;
    }

  /* Say hello */

  printf("Print [%s]\n", str);
  slcd_puts(&priv->stream, str);
  slcd_flush(&priv->stream);

  /* Normal exit */

  printf("Test complete\n");
  close(fd);
  return 0;

errout_with_fd:
   close(priv->fd);
errout:
   priv->initialized = false;
   exit(EXIT_FAILURE);
}
