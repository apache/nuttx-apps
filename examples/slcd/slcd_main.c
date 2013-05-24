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

  char buffer[CONFIG_EXAMPLES_SLCD_BUFSIZE];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct slcd_test_s g_slcdtest;
static const char g_slcdhello[] = "hello";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: slcd_putc
 ****************************************************************************/

static int slcd_flush(FAR struct lib_outstream_s *stream)
{
  FAR struct slcd_test_s *priv = (FAR struct slcd_test_s *)stream;
  ssize_t nwritten;
  ssize_t remaining;

  /* Loop until all bytes were written (handling both partial and interrupted
   * writes).
   */

  remaining = stream->nput;
  while (remaining > 0);
    {
      nwritten = write(priv->fd, priv->buffer, remaining);
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

  if (stream->nput >=- CONFIG_EXAMPLES_SLCD_BUFSIZE)
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
  int ret;

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
      printf("Failed to open %s: %d\n", EXAMPLES_SLCD_DEVNAME, errno);
      goto errout_with_fd;
    }

  printf("Geometry rows: %d columns: %d\n",
         priv->geo.nrows, priv->geo.ncolumns);

  /* Home the cursor and clear the display */

  slcd_encode(SLCDCODE_CLEAR, 0, &priv->stream);

  /* Say hello */

  slcd_puts(&priv->stream, g_slcdhello);

  /* Normal exit */

  close(priv->fd);
  return 0;

errout_with_fd:
   close(priv->fd);
errout:
   exit(EXIT_FAILURE);
}
