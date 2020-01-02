/****************************************************************************
 * examples/hidkbd/hidkbd_main.c
 *
 *   Copyright (C) 2011, 2013-2015, 2017 Gregory Nutt. All rights reserved.
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
 * 3. Neither the name Gregory Nutt nor the names of its contributors may be
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
#include <errno.h>

#include <nuttx/usb/usbhost.h>

#ifdef CONFIG_EXAMPLES_HIDKBD_ENCODED
#  include <nuttx/streams.h>
#  include <nuttx/input/kbd_codec.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

/* Sanity checking */

#ifndef CONFIG_USBHOST
#  error "CONFIG_USBHOST is not defined"
#endif

#ifdef CONFIG_USBHOST_INT_DISABLE
#  error "Interrupt endpoints are disabled (CONFIG_USBHOST_INT_DISABLE)"
#endif

/* Provide some default values for other configuration settings */

#ifndef CONFIG_EXAMPLES_HIDKBD_DEVNAME
#  define CONFIG_EXAMPLES_HIDKBD_DEVNAME "/dev/kbda"
#endif

#if !defined(CONFIG_HIDKBD_ENCODED) || !defined(CONFIG_LIB_KBDCODEC)
#  undef CONFIG_EXAMPLES_HIDKBD_ENCODED
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_HIDKBD_ENCODED
struct hidbkd_instream_s
{
  struct lib_instream_s stream;
  FAR char *buffer;
  ssize_t nbytes;
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hidkbd_getstream
 *
 * Description:
 *   Get one character from the keyboard.
 *
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_HIDKBD_ENCODED
static int hidkbd_getstream(FAR struct lib_instream_s *this)
{
  FAR struct hidbkd_instream_s *kbdstream = (FAR struct hidbkd_instream_s *)this;

  DEBUGASSERT(kbdstream && kbdstream->buffer);
  if (kbdstream->nbytes > 0)
    {
      kbdstream->nbytes--;
      kbdstream->stream.nget++;
      return (int)*kbdstream->buffer++;
    }

  return EOF;
}
#endif

/****************************************************************************
 * Name: hidkbd_decode
 *
 * Description:
 *   Decode encoded keyboard input
 *
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_HIDKBD_ENCODED
static void hidkbd_decode(FAR char *buffer, ssize_t nbytes)
{
  struct hidbkd_instream_s kbdstream;
  struct kbd_getstate_s state;
  uint8_t ch;
  int ret;

  /* Initialize */

  memset(&state, 0, sizeof(struct kbd_getstate_s));
  kbdstream.stream.get  = hidkbd_getstream;
  kbdstream.stream.nget = 0;
  kbdstream.buffer      = buffer;
  kbdstream.nbytes      = nbytes;

  /* Loop until all of the bytes have been consumed.  We implicitly assume
   * that the escaped sequences do not cross buffer boundaries.  That
   * might be true if the read buffer were small or the data rates high.
   */

  for (;;)
    {
      /* Decode the next thing from the buffer */

      ret = kbd_decode((FAR struct lib_instream_s *)&kbdstream, &state, &ch);
      if (ret == KBD_ERROR) /* Error or end-of-file */
        {
          /* Break out when all of the data has been processed */

          break;
        }

      /* Normal data?  Or special key?  Press?  Or release? */

      switch (ret)
        {
        case KBD_PRESS: /* Key press event */
          printf("Normal Press:    %c [%02x]\n", isprint(ch) ? ch : '.', ch);
          break;

        case KBD_RELEASE: /* Key release event */
          printf("Normal Release:  %c [%02x]\n", isprint(ch) ? ch : '.', ch);
          break;

        case KBD_SPECPRESS: /* Special key press event */
          printf("Special Press:   %d\n", ch);
          break;

        case KBD_SPECREL: /* Special key release event */
          printf("Special Release: %d\n", ch);
          break;

        case KBD_ERROR: /* Error or end-of-file, already handled */
        default:
          printf("Unexpected:      %d\n", ret);
          break;
        }
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hidkbd_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  char buffer[256];
  ssize_t nbytes;
  int fd;

  /* Eventually logic here will open the kbd device and perform the HID
   * keyboard test.
   */

  for (;;)
    {
      /* Open the keyboard device.  Loop until the device is successfully
       * opened.
       */

      do
        {
          printf("Opening device %s\n", CONFIG_EXAMPLES_HIDKBD_DEVNAME);
          fd = open(CONFIG_EXAMPLES_HIDKBD_DEVNAME, O_RDONLY);
          if (fd < 0)
            {
               printf("Failed: %d\n", errno);
               fflush(stdout);
               sleep(3);
            }
        }
      while (fd < 0);

      printf("Device %s opened\n", CONFIG_EXAMPLES_HIDKBD_DEVNAME);
      fflush(stdout);

      /* Loop until there is a read failure (or EOF?) */

      do
        {
          /* Read a buffer of data */

          nbytes = read(fd, buffer, 256);
          if (nbytes > 0)
            {
              /* On success, echo the buffer to stdout */

#ifdef CONFIG_EXAMPLES_HIDKBD_ENCODED
              hidkbd_decode(buffer, nbytes);
#else
              write(1, buffer, nbytes);
#endif
            }
        }
      while (nbytes > 0);

      printf("Closing device %s: %d\n", CONFIG_EXAMPLES_HIDKBD_DEVNAME, (int)nbytes);
      fflush(stdout);
      close(fd);
    }

  return 0;
}
