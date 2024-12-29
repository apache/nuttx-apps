/****************************************************************************
 * apps/examples/rmtchar/rmtchar_transmitter.c
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <unistd.h>

#include <nuttx/audio/audio.h>

#include "rmtchar.h"

#ifdef CONFIG_EXAMPLES_RMTCHAR_TX

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
 * Name: rmtchar_transmitter()
 *
 * Description:
 *   This is the entry point for the transmitter thread.
 *
 ****************************************************************************/

pthread_addr_t rmtchar_transmitter(pthread_addr_t arg)
{
  FAR struct rmtchar_state_s *rmtchar = (FAR struct rmtchar_state_s *)arg;
  struct rmt_item32_s buf[rmtchar->rmtchar_items];
  int duration = 1000;
  int nwritten;
  int fd;
  int i;

  /* Open the RMT character device */

  fd = open(rmtchar->txdevpath, O_WRONLY);
  if (fd < 0)
    {
      int errcode = errno;
      printf("rmtchar_transmitter: ERROR: failed to open %s: %d\n",
              rmtchar->txdevpath, errcode);
      pthread_exit(NULL);
    }

  /* Fill the transmitter buffer with known items */

  for (i = 0; i < rmtchar->rmtchar_items; i++)
    {
      buf[i].level0 = 1;
      buf[i].duration0 = duration;
      buf[i].level1 = 0;
      buf[i].duration1 = duration;

      duration += 10;
    }

  /* Flush any output before writing */

  fflush(stdout);

  /* Print the buffer to be sent */

  print_items(buf, rmtchar->rmtchar_items);

  /* Then send the buffer */

  /* Write the buffer to the RMT character driver */

  nwritten = write(fd, (char *)buf, rmtchar->rmtchar_items *
                   sizeof(struct rmt_item32_s));
  if (nwritten < 0)
    {
      int errcode = errno;
      if (errcode != EINTR)
        {
          printf("rmtchar_transmitter: ERROR: write failed: %d\n",
                  errcode);
          close(fd);
          pthread_exit(NULL);
        }
    }
  else if (nwritten != (rmtchar->rmtchar_items *
                        sizeof(struct rmt_item32_s)))
    {
      printf("rmtchar_transmitter: ERROR: partial write: %d\n",
              nwritten);
      close(fd);
      pthread_exit(NULL);
    }
  else
    {
      printf("rmtchar_transmitter: Send buffer with %d bytes\n",
             rmtchar->rmtchar_items * sizeof(struct rmt_item32_s));
    }

  /* Make sure that the receiver thread has a chance to run */

  pthread_yield();

  close(fd);
  return NULL;
}

#endif /* CONFIG_EXAMPLES_RMTCHAR_TX */
