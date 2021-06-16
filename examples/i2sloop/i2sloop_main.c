/****************************************************************************
 * apps/examples/i2sloop/i2sloop_main.c
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

int main(int argc, FAR char *argv[])
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
  desc.u.pbuffer = &apb;

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
