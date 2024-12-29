/****************************************************************************
 * apps/examples/rmtchar/rmtchar_receiver.c
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

#include "rmtchar.h"

#ifdef CONFIG_EXAMPLES_RMTCHAR_RX

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
 * Name: rmtchar_receiver()
 *
 * Description:
 *   This is the entry point for the receiver thread.
 *
 ****************************************************************************/

pthread_addr_t rmtchar_receiver(pthread_addr_t arg)
{
  FAR struct rmtchar_state_s *rmtchar = (FAR struct rmtchar_state_s *)arg;
  struct rmt_item32_s buf[rmtchar->rmtchar_items];
  int nread;
  int fd;

  /* Open the RMT character device */

  fd = open(rmtchar->rxdevpath, O_RDONLY);
  if (fd < 0)
    {
      int errcode = errno;
      printf("rmtchar_receiver: ERROR: failed to open %s: %d\n",
              rmtchar->rxdevpath, errcode);
      pthread_exit(NULL);
    }

  /* Flush any output before reading */

  fflush(stdout);

  /* Read the buffer to the RMT character driver */

  nread = read(fd, (char *)buf, rmtchar->rmtchar_items *
               sizeof(struct rmt_item32_s));
  if (nread < 0)
    {
      int errcode = errno;
      if (errcode != EINTR)
        {
          printf("rmtchar_receiver: ERROR: read failed: %d\n",
                  errcode);
          close(fd);
          pthread_exit(NULL);
        }
    }
  else
    {
      if (nread != (rmtchar->rmtchar_items * sizeof(struct rmt_item32_s)))
        {
          printf("rmtchar_receiver: ERROR: partial read: %d\n",
                  nread);
          printf("rmtchar_receiver: Received buffer:\n");
          print_items(buf, nread / sizeof(struct rmt_item32_s));
          close(fd);
          pthread_exit(NULL);
        }
      else
        {
          printf("rmtchar_receiver: Received buffer:\n");
          print_items(buf, nread / sizeof(struct rmt_item32_s));
        }
    }

  /* Make sure that the transmitter thread has a chance to run */

  pthread_yield();

  close(fd);
  return NULL;
}

#endif /* CONFIG_EXAMPLES_RMTCHAR_RX */
