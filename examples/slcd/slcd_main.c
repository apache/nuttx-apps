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

#include <nuttx/lcd/slcd_codec.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef EXAMPLES_SLCD_DEVNAME
#  define EXAMPLES_SLCD_DEVNAME "/dev/slcd"
#endif

/****************************************************************************
 * Private Typs
 ****************************************************************************/
/* All state information for this test is kept within the following structure
 * in order create a namespace and to minimize the possibility of name
 * collisions.
 */

struct slcd_test_s
{
  struct lib_outstream_s stream;  /* Stream to use for all output */
  struct slcd_geometry_s geo;     /* Size of the SLCD (rows x columns) */
  int                    fd;      /* File descriptor or the open SLCD device */

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

  printf("%s (%p):\n", msg, buffer);
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

  /* Initialize the output stream */

  memset(priv, 0, sizeof(struct slcd_test_s));
  priv->stream.put   = slcd_putc;
#ifdef CONFIG_STDIO_LINEBUFFER
  priv->stream.flush = lib_noflush;
#endif

  /* Open the SLCD device */

  printf("Opening %s for read/write access\n", EXAMPLES_SLCD_DEVNAME);

  priv->fd = open(EXAMPLES_SLCD_DEVNAME, O_RDWR);
  if (priv->fd < 0)
    {
      printf("Failed to open %s: %d\n", EXAMPLES_SLCD_DEVNAME, errno);
      goto errout;
    }

  /* Get the geometry of the SCLD device */

  ret = ioctl(priv->fd, SLCDIOC_GEOMETRY, (unsigned long)&priv->geo);
  if (ret < 0)
    {
      printf("ioctl(SLCDIOC_GEOMETRY) failed: %d\n", errno);
      goto errout_with_fd;
    }

  printf("Geometry rows: %d columns: %d nbars: %d\n",
         priv->geo.nrows, priv->geo.ncolumns, priv->geo.nbars);

  /* Home the cursor and clear the display */

  printf("Clear screen\n");
  slcd_encode(SLCDCODE_CLEAR, 0, &priv->stream);

  /* Say hello */

  printf("Print [%s]\n", str);
  slcd_puts(&priv->stream, str);
  slcd_flush(&priv->stream);

  /* Normal exit */

  printf("Test complete\n");
  close(priv->fd);
  return 0;

errout_with_fd:
   close(priv->fd);
errout:
   exit(EXIT_FAILURE);
}
