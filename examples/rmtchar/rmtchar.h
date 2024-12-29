/****************************************************************************
 * apps/examples/rmtchar/rmtchar.h
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

#ifndef __APPS_EXAMPLES_RMTCHAR_RMTCHAR_H
#define __APPS_EXAMPLES_RMTCHAR_RMTCHAR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <pthread.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration */

#ifndef CONFIG_EXAMPLES_RMTCHAR_TX_DEVPATH
#  define CONFIG_EXAMPLES_RMTCHAR_TX_DEVPATH "/dev/rmt0"
#endif

#ifndef CONFIG_EXAMPLES_RMTCHAR_RX_DEVPATH
#  define CONFIG_EXAMPLES_RMTCHAR_RX_DEVPATH "/dev/rmt1"
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct rmtchar_state_s
{
  bool initialized;
  int rmtchar_items;
#ifdef CONFIG_EXAMPLES_RMTCHAR_TX
  FAR char *txdevpath;
#endif
#ifdef CONFIG_EXAMPLES_RMTCHAR_RX
  FAR char *rxdevpath;
#endif
};

struct rmt_item32_s
{
  union
    {
      struct
        {
          uint32_t duration0 : 15; /* Duration of level0 */
          uint32_t level0 : 1;     /* Level of the first part */
          uint32_t duration1 : 15; /* Duration of level1 */
          uint32_t level1 : 1;     /* Level of the second part */
        };
      uint32_t val; /* Equivalent unsigned value for the RMT item */
    };
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: rmtchar_transmitter()
 *
 * Description:
 *   This is the entry point for the transmitter thread.
 *
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_RMTCHAR_TX
pthread_addr_t rmtchar_transmitter(pthread_addr_t arg);
#endif

/****************************************************************************
 * Name: rmtchar_receiver()
 *
 * Description:
 *   This is the entry point for the receiver thread.
 *
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_RMTCHAR_RX
pthread_addr_t rmtchar_receiver(pthread_addr_t arg);
#endif

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

void print_items(struct rmt_item32_s *buf, int len);

#endif /* __APPS_EXAMPLES_RMTCHAR_RMTCHAR_H */
