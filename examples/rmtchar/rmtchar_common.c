/****************************************************************************
 * apps/examples/rmtchar/rmtchar_common.c
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
 * Name: print_items
 *
 * Description:
 *   This function prints the level and duration of RMT items stored in a
 *   buffer. It iterates over the buffer, printing the level and duration of
 *   each item in a formatted manner.
 *
 * Input Parameters:
 *   buf  - Pointer to the buffer containing the RMT items.
 *   len  - The number of RMT items in the buffer.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void print_items(struct rmt_item32_s *buf, int len)
{
  int i;

  for (i = 0; i < len; i++, buf++)
    {
      printf("\t[%d]:\tL %d\tD %d\t", i, buf->level0, buf->duration0);
      printf("L %d\t D %d\n", buf->level1, buf->duration1);
    }
}
