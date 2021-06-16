/****************************************************************************
 * apps/examples/clcd/slcd_main.c
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
#  define CONFIG_EXAMPLES_SLCD_DEVNAME "/dev/slcd0"
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

void slcd_dumpbuffer(FAR const char *msg, FAR const uint8_t *buffer,
                     unsigned int buflen)
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
      slcd_flush(stream);
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

int main(int argc, FAR char *argv[])
{
  FAR struct slcd_test_s *priv = &g_slcdtest;
  FAR const char *str = g_slcdhello;
  int fd;
  int ret;

  /* Parse the command line.  For now, only a single optional string argument
   * is supported.
   */

  if (argc > 1)
    {
      str = argv[1];
    }

  /* Open the SLCD device */

  printf("Opening %s for read/write access\n", CONFIG_EXAMPLES_SLCD_DEVNAME);

  fd = open(CONFIG_EXAMPLES_SLCD_DEVNAME, O_RDWR);
  if (fd < 0)
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
      priv->fd = fd;

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
      printf("Set brightness to %lu\n", brightness);

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
