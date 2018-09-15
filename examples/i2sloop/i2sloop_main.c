/****************************************************************************
 * examples/i2sloop/i2sloop_main.c
 *
 *   Copyright 2018 Sony Video & Sound Products Inc.
 *   Author: Masayuki Ishikawa <Masayuki.Ishikawa@jp.sony.com>
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
#include <nuttx/audio/audio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_i2sloop_running;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void _signal_handler(int number)
{
  g_i2sloop_running = false;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2sloop_main
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int i2sloop_main(int argc, char *argv[])
#endif
{
  FAR struct ap_buffer_s *apb;
  struct audio_buf_desc_s desc;
  ssize_t bufsize;
  int ret;
  int fd;
  int opt;
  int ch;
  int freq;

  ch = 1;
  freq = 16000;

  while ((opt = getopt(argc, argv, "c:f:")) != -1)
     {
       switch (opt)
         {
           case 'c':
             ch = atoi(optarg);
             break;

           case 'f':
             freq = atoi(optarg);
             break;
         }
   }

  /* Open the I2S character device */

  fd = open("/dev/i2schar0", O_RDWR);
  DEBUGASSERT(0 < fd);

  /* Setup sample freq for i2s */

  struct audio_caps_desc_s  cap_desc;
  cap_desc.caps.ac_len        = sizeof(struct audio_caps_s);
  cap_desc.caps.ac_type       = AUDIO_TYPE_OUTPUT | AUDIO_TYPE_INPUT;
  cap_desc.caps.ac_controls.w = freq;
  cap_desc.caps.ac_channels   = ch;
  cap_desc.caps.ac_format.hw  = AUDIO_FMT_PCM;

  ioctl(fd, AUDIOIOC_CONFIGURE, (unsigned long)&cap_desc);

  /* Allocate an audio buffer */

  desc.numbytes   = 1024;
  desc.u.ppBuffer = &apb;

  ret = apb_alloc(&desc);
  DEBUGASSERT(ret == sizeof(desc));

  signal(1, _signal_handler);
  g_i2sloop_running = true;

  while (g_i2sloop_running)
    {
      bufsize = sizeof(struct ap_buffer_s) + apb->nmaxbytes;
      ret = read(fd, apb, bufsize);

      if (0 < ret)
        {
          apb->nbytes = apb->nmaxbytes;
          bufsize = sizeof(struct ap_buffer_s) + apb->nbytes;
          ret = write(fd, apb, bufsize);
        }

    }

  apb_free(apb);
  close(fd);

  return EXIT_SUCCESS;
}
