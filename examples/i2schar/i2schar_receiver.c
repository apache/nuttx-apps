/****************************************************************************
 * apps/examples/i2schar/i2schar_receiver.c
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

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/audio/audio.h>

#include "i2schar.h"

#ifdef CONFIG_EXAMPLES_I2SCHAR_RX

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
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
 * Name: i2schar_receiver()
 *
 * Description:
 *   This is the entry point for the receiver thread.
 *
 ****************************************************************************/

pthread_addr_t i2schar_receiver(pthread_addr_t arg)
{
  FAR struct ap_buffer_s *apb;
  struct audio_buf_desc_s desc;
  int bufsize;
  int nread;
  int ret;
  int fd;
  int i;

  /* Open the I2C character device */

  fd = open(g_i2schar.devpath, O_RDONLY);
  if (fd < 0)
    {
      int errcode = errno;
      printf("i2schar_receiver: ERROR: failed to open %s: %d\n",
              g_i2schar.devpath, errcode);
      pthread_exit(NULL);
    }

  /* Loop for the requested number of times */

  for (i = 0; i < CONFIG_EXAMPLES_I2SCHAR_TXBUFFERS; i++)
    {
      /* Allocate an audio buffer of the configured size */

      desc.numbytes   = CONFIG_EXAMPLES_I2SCHAR_BUFSIZE;
      desc.u.pbuffer = &apb;

      ret = apb_alloc(&desc);
      if (ret < 0)
        {
           printf(
             "i2schar_receiver: ERROR: failed to allocate buffer %d: %d\n",
                i + 1, ret);
           close(fd);
           pthread_exit(NULL);
        }

      bufsize = sizeof(struct ap_buffer_s) + CONFIG_EXAMPLES_I2SCHAR_BUFSIZE;

      /* Then receifve into the buffer */

      do
        {
          /* Flush any output before reading */

          fflush(stdout);

          /* Read the buffer to the I2S character driver */

          nread = read(fd, apb, bufsize);
          if (nread < 0)
            {
              int errcode = errno;
              if (errcode != EINTR)
                {
                  printf("i2schar_receiver: ERROR: read failed: %d\n",
                          errcode);
                  close(fd);
                  pthread_exit(NULL);
                }
            }
          else if (nread != bufsize)
            {
              printf("i2schar_receiver: ERROR: partial read: %d\n",
                      nread);
              close(fd);
              pthread_exit(NULL);
            }
          else
            {
              printf("i2schar_receiver: Received buffer %d\n", i + 1);
            }
        }
      while (nread != bufsize);

      /* Make sure that the transmitter thread has a chance to run */

      pthread_yield();
    }

  close(fd);
  return NULL;
}

#endif /* CONFIG_EXAMPLES_I2SCHAR_RX */
