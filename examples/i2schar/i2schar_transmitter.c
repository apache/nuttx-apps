/****************************************************************************
 * apps/examples/i2schar/i2schar_transmitter.c
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

#ifdef CONFIG_EXAMPLES_I2SCHAR_TX

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
 * Name: i2schar_transmitter()
 *
 * Description:
 *   This is the entry point for the transmitter thread.
 *
 ****************************************************************************/

pthread_addr_t i2schar_transmitter(pthread_addr_t arg)
{
  FAR struct ap_buffer_s *apb;
  struct audio_buf_desc_s desc;
  uint8_t crap;
  uint8_t *ptr;
  int bufsize;
  int nwritten;
  int ret;
  int fd;
  int i;
  int j;

  /* Open the I2C character device */

  fd = open(g_i2schar.devpath, O_WRONLY);
  if (fd < 0)
    {
      int errcode = errno;
      printf("i2schar_transmitter: ERROR: failed to open %s: %d\n",
              g_i2schar.devpath, errcode);
      pthread_exit(NULL);
    }

  /* Loop for the requested number of times */

  for (i = 0, crap = 0; i < CONFIG_EXAMPLES_I2SCHAR_TXBUFFERS; i++)
    {
      /* Allocate an audio buffer of the configured size */

      desc.numbytes   = CONFIG_EXAMPLES_I2SCHAR_BUFSIZE;
      desc.u.pbuffer = &apb;

      ret = apb_alloc(&desc);
      if (ret < 0)
        {
           printf("
             i2schar_transmitter: ERROR: failed to allocate buffer %d: %d\n",
             i + 1, ret);
           close(fd);
           pthread_exit(NULL);
        }

      bufsize = sizeof(struct ap_buffer_s) + CONFIG_EXAMPLES_I2SCHAR_BUFSIZE;

      /* Fill the audio buffer with crap */

      for (j = 0, ptr = apb->samp; j < CONFIG_EXAMPLES_I2SCHAR_BUFSIZE; j++)
        {
          *ptr++ = crap++;
        }

      apb->nbytes = CONFIG_EXAMPLES_I2SCHAR_BUFSIZE;

      /* Then send the buffer */

      do
        {
          /* Flush any output before writing */

          fflush(stdout);

          /* Write the buffer to the I2S character driver */

          nwritten = write(fd, apb, bufsize);
          if (nwritten < 0)
            {
              int errcode = errno;
              if (errcode != EINTR)
                {
                  printf("i2schar_transmitter: ERROR: write failed: %d\n",
                          errcode);
                  close(fd);
                  pthread_exit(NULL);
                }
            }
          else if (nwritten != bufsize)
            {
              printf("i2schar_transmitter: ERROR: partial write: %d\n",
                      nwritten);
              close(fd);
              pthread_exit(NULL);
            }
          else
            {
              printf("i2schar_transmitter: Send buffer %d\n", i + 1);
            }
        }
      while (nwritten != bufsize);

      /* Make sure that the receiver thread has a chance to run */

      pthread_yield();
    }

  close(fd);
  return NULL;
}

#endif /* CONFIG_EXAMPLES_I2SCHAR_TX */
