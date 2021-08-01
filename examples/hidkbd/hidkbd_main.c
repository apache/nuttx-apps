/****************************************************************************
 * apps/examples/hidkbd/hidkbd_main.c
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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
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

#ifdef CONFIG_USBHOST_INT_DISABLE
#  error "Interrupt endpoints are disabled (CONFIG_USBHOST_INT_DISABLE)"
#endif

/* Provide some default values for other configuration settings */

#ifndef CONFIG_EXAMPLES_HIDKBD_DEVNAME
#  define CONFIG_EXAMPLES_HIDKBD_DEVNAME "/dev/kbda"
#endif

#if !defined(CONFIG_HIDKBD_ENCODED) || !defined(CONFIG_LIBC_KBDCODEC)
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
  FAR struct hidbkd_instream_s *kbdstream = \
    (FAR struct hidbkd_instream_s *)this;

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

  for (; ; )
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

  for (; ; )
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

      printf("Closing device %s: %d\n", CONFIG_EXAMPLES_HIDKBD_DEVNAME,
             (int)nbytes);
      fflush(stdout);
      close(fd);
    }

  return 0;
}
